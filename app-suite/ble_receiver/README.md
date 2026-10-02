# BLE Telemetry Receiver

A Flutter app that receives data from an ESP32 device via Bluetooth Low Energy (BLE) and forwards it to the telemetry API server.

## Features

- **BLE Device Scanning**: Discover and connect to nearby Bluetooth devices
- **Real-time Data Reception**: Receive telemetry data from ESP32
- **Automatic Transmission**: Parse and send data to the backend API
- **Transmission Logging**: Track sent/failed data transmissions
- **Multiple Data Types**: Support for vital signs, temperature, motion, and panic events

## Data Format

The ESP32 should send JSON data in the following formats:

### Vital Signs
```json
{
  "type": "vital_signs",
  "heartrate": 72,
  "oxygen": 98.5,
  "confidence": 0.95
}
```

### Temperature
```json
{
  "type": "temperature",
  "temperature": 23.5
}
```

### Motion
```json
{
  "type": "motion",
  "rotX": 10.5,
  "rotY": -5.2,
  "rotZ": 15.8,
  "accX": 0.1,
  "accY": 0.2,
  "accZ": 9.8
}
```

### Panic Event
```json
{
  "type": "panic",
  "eventType": "PANIC_BUTTON"
}
```

## API Configuration

The app sends data to:
- **Base URL**: `http://172.20.100.35:8080`
- **Auth Token**: `esp32_static_token_12345`

Update these in `lib/services/telemetry_service.dart` if needed.
