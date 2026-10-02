#include "BLE_connection.h"

BLEServer*         pServer = nullptr;
BLECharacteristic* pVitals = nullptr;
BLECharacteristic* pMotion = nullptr;
BLECharacteristic* pTemp = nullptr;
BLECharacteristic* pPanicAcknowledge = nullptr;
BLECharacteristic* pPanic    = nullptr;

bool deviceConnected    = false;
bool oldDeviceConnected = false;

uint32_t lastConnectionCheckTime = 0;

void packFloat(uint8_t* buf, int offset, float val) {
  memcpy(buf + offset, &val, 4);
}

class ServerCB : public BLEServerCallbacks {
  void onConnect(BLEServer*)    override { deviceConnected = true;  Serial.println("[BLE] Connected"); }
  void onDisconnect(BLEServer*) override { deviceConnected = false; Serial.println("[BLE] Disconnected"); }
};

void setupBLE() {

  BLEDevice::init("FORCE Crutch");
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCB());

  BLEService* svc = pServer->createService(BLEUUID(SERVICE_UUID), 40);

  auto notify  = BLECharacteristic::PROPERTY_NOTIFY;
  auto rw      = BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE;
  auto wr      = BLECharacteristic::PROPERTY_WRITE;
  auto rn      = BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY;

  auto mkChar = [&](const char* uuid, uint32_t props) -> BLECharacteristic* {
    auto* c = svc->createCharacteristic(uuid, props);
    if (props & BLECharacteristic::PROPERTY_NOTIFY)
      c->addDescriptor(new BLE2902());
    return c;
  };

  pVitals = mkChar(CHAR_VITALS_UUID,  notify);
  pMotion = mkChar(CHAR_MOTION_UUID,   notify);
  pTemp   = mkChar(CHAR_TEMP_UUID,     notify);

  pPanicAcknowledge = mkChar(CHAR_PANIC_AKNOWLEDGE_UUID, wr);
  pPanic    = mkChar(CHAR_PANIC_UUID,       notify);

  svc->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] Advertising…");
}

void sendPanic(bool isFall) {
  uint8_t val = isFall ? 1 : 0;
  pPanic->setValue(&val, 1);
  pPanic->notify();
}

void bluetoothTask() {
    uint32_t now = millis();

    if (!deviceConnected) {
        // Serial.println("[BLE] No device connected");
        if (!oldDeviceConnected) {  
            // Serial.println("[BLE] in NOT oldDeviceConnected block");
            return; 
        }

        if (now - lastConnectionCheckTime < 500) {
        return; 
        }

        pServer->startAdvertising();
        // Serial.println("[BLE] Restarted advertising");
        oldDeviceConnected = false;
        return;
    }
    // Serial.println("[BLE] Device connected, sending data...");
    oldDeviceConnected = true;
    lastConnectionCheckTime = now;

    //Send vitals
    uint8_t buf[12];
    packFloat(buf, 0, body.heartRate);
    packFloat(buf, 4, body.oxygen);
    packFloat(buf, 8, body.confidence);
    pVitals->setValue(buf, 12);
    pVitals->notify();

    //Send motion — units: accel in g, gyro in °/s
    //  Accel sensitivity at ±8 g:    4096 LSB/g
    //  Gyro  sensitivity at ±2000 °/s: 16.4 LSB/(°/s)
    uint8_t motionBuf[24];
    packFloat(motionBuf, 0, ax / 4096.0f);
    packFloat(motionBuf, 4, ay / 4096.0f);
    packFloat(motionBuf, 8, az / 4096.0f);
    packFloat(motionBuf, 12, gx / 16.4f);
    packFloat(motionBuf, 16, gy / 16.4f);
    packFloat(motionBuf, 20, gz / 16.4f);
    pMotion->setValue(motionBuf, 24);
    pMotion->notify();

    //Send temp
    uint8_t tempBuf[4];
    packFloat(tempBuf, 0, globalBodyTemp);
    pTemp->setValue(tempBuf, 4);
    pTemp->notify();

}

bool isBLEConnected() {
    return deviceConnected;
}