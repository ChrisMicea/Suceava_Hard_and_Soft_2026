#include "sensor_vitals.h"

// ── Shared Sensor Instantiations & Variables ──────────────────
SparkFun_Bio_Sensor_Hub bioHub(RES_PIN, MFIO_PIN); 
MAX30205 tempSensor;
bioData body;  

bool tempSensorReady = false;
bool bioHubReady = false; 
float globalBodyTemp = 0.0;

// ── Task: Pulse Oximeter & Bio Hub Execution (Every 200ms) ────
void readBioHub() {
  if (!bioHubReady) {
    if (bioHub.begin() == 0) {
      bioHub.configBpm(1);
      bioHubReady = true;
      Serial.println("[Recovery] Bio Hub recovered online.");
    }
    return;
  }

  bioData freshData = bioHub.readBpm();
  if (freshData.extStatus == 0) { 
    body = freshData; 
  }
}

// ── Task: Temperature Sensor Evaluation (Every 3s) ─────────────
void readTemperature() {
  if (!tempSensorReady) {
    if (tempSensor.scanAvailableSensors()) {
      tempSensor.begin();
      tempSensorReady = true;
      Serial.println("[Recovery] Temp Sensor recovered online.");
    }
    return;
  }
  globalBodyTemp = tempSensor.getTemperature();
}

// ── Initialize Bio Hub ────────────────────────────────────────
void initializeBioHub() {
  int bioResult = bioHub.begin();
  if (bioResult == 0) {
    bioHub.configBpm(1); 
    bioHubReady = true;
    Serial.println("Bio Hub Online.");
  } 
  else {
    Serial.printf("Bio Hub Initialization Error Code: %d\n", bioResult);
  }
}

// ── Initialize Temperature Sensor ─────────────────────────────
void initializeTemperatureSensor() {
  if (tempSensor.scanAvailableSensors()) {
    tempSensor.begin();
    tempSensorReady = true;
    Serial.println("MAX30205 Temperature Sensor Online.");
  } 
  else {
    Serial.println("MAX30205 Temperature Sensor Hardware Not Found.");
  }
}
