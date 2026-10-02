# BLE Receiver Component

Flutter mobile application that receives telemetry data from the ESP32 device via Bluetooth Low Energy (BLE) and forwards it to the backend API server.

## Overview

This app serves as the bridge between the ESP32 hardware and the backend server:
- Scans for and connects to the ESP32 via BLE
- Receives JSON-formatted telemetry data from the device
- Parses and forwards data to the backend REST API
- Handles connection state and transmission errors

## Technology Stack

- **Framework**: Flutter (Android/iOS)
- **Communication**: Bluetooth Low Energy (BLE)
- **HTTP Client**: Dart http package
- **Architecture**: Monolithic single-file app (lib/main.dart - 24KB)

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

⚠️ These values are hardcoded in `lib/main.dart` and should be made configurable via environment variables or a config file.

## Build & Run

### Flutter Setup
```bash
flutter pub get
```

### Run on Device
```bash
flutter run
```

### Build APK
```bash
flutter build apk
```

### Build iOS
```bash
flutter build ios
```

## Architecture Notes

The app is currently implemented as a single large file (`lib/main.dart`). For better maintainability, consider refactoring into:
- `lib/services/ble_service.dart` - BLE connection management
- `lib/services/api_service.dart` - HTTP API calls
- `lib/models/` - Data models for each data type
- `lib/screens/` - UI screens

## Permissions

### Android
Add to `android/app/src/main/AndroidManifest.xml`:
```xml
<uses-permission android:name="android.permission.BLUETOOTH"/>
<uses-permission android:name="android.permission.BLUETOOTH_SCAN"/>
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT"/>
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"/>
```

### iOS
Add to `ios/Runner/Info.plist`:
```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>This app uses Bluetooth to connect to the fall detection device.</string>
<key>NSBluetoothPeripheralUsageDescription</key>
<string>This app uses Bluetooth to connect to the fall detection device.</string>
```
