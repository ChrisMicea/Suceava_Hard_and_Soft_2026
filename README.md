# Suceava H&S Fall Detection System

A crutch-mounted fall detection system for elderly care, featuring multi-stage IMU-based fall detection, vital signs monitoring, and BLE connectivity.

> **⚠️ Hardware Required**: This is an embedded application designed for specific hardware (ESP32, MPU-6050, MAX30101, MAX30205, OLED display, etc.). The firmware cannot run without the physical hardware platform. The companion apps (backend, BLE receiver, data monitor) can run independently for testing with simulated data.

## Contest Context

This project was developed for the **Hard and Soft 2026 International Competition** (Suceava, Romania).

**Competition Website**: https://www.hardandsoft.ro/

**Topic Presentation**: See `H&S_Suceava_2026_topic_presentation_v2.pdf` for the original contest topic and requirements.

### Team

**Team Name**: Politehnica University of Timișoara 1

**Institution**: Politehnica University of Timișoara

**Team Members**:
- [Member 1 Name]
- [Member 2 Name]
- [Member 3 Name]
- [Member 4 Name]

## Project Overview

This system is designed to detect falls in elderly users who rely on crutches for mobility. The device is mounted on the crutch tip and uses a combination of accelerometer and gyroscope data to distinguish between actual falls and normal crutch movements (e.g., setting the crutch down, walking).

### Key Features

- **Multi-Stage Fall Detection**: 5-stage pipeline with grip-aware thresholds
  - Gate 0: On-body detection and arming logic
  - Stage 1: Free-fall detection
  - Stage 2: Impact detection
  - Stage 3: Rotation analysis
  - Stage 4: Post-fall stillness verification
- **Grip-Aware Thresholds**: Different detection paths for held vs. unheld crutch
  - HELD mode: Lenient thresholds (user actively holding)
  - STRICT-A: Unheld with free-fall detected
  - STRICT-B: Unheld without free-fall (hardest path)
- **Vital Signs Monitoring**: Heart rate, SpO2, and body temperature
- **BLE Connectivity**: Real-time telemetry to companion app
- **OLED Display**: Local status and vital signs display
- **Touch Sensor**: Detects when user is holding the crutch
- **Panic Button**: Manual emergency alert activation

## Project Structure

```
Suceava_H&S/
├── firmware/                # ESP32 embedded firmware (PlatformIO)
│   ├── src/                 # Source files
│   ├── include/             # Header files
│   ├── lib/                 # Libraries
│   └── platformio.ini       # Build configuration
├── app-suite/               # Companion applications
│   ├── backend/             # Spring Boot REST API server
│   ├── ble_receiver/        # Flutter BLE-to-API bridge app
│   ├── data_monitor/        # Flutter caregiver dashboard app
│   └── telemetry_api.md     # API documentation
├── archive/                 # Archived old versions
│   ├── v1-early-prototype/  # Earliest version
│   ├── v2-pre-final/        # Pre-final version
│   ├── v3-alternative-approach/  # Alternative implementation
│   └── v4-duplicate-final/  # Duplicate of final version
├── experiments/             # Experimental code and prototypes
│   ├── power-management/    # Power management/hibernation experiments
│   ├── testBuzz/            # Buzzer testing
│   └── *.txt                # Serial data and experimental implementations
├── final_documentation/     # Competition deliverables
│   ├── FORCE_documentation.pdf         # Final project documentation
│   └── Timisoara1_FORCE_Design_Document.pdf  # Design document (day 3)
└── [other contest materials]
```

## Hardware Requirements

### Microcontroller
- **ESP32** (ESP32-DevKitC or similar)

### Sensors
- **MPU-6050**: 6-axis IMU (accelerometer + gyroscope)
  - I2C address: 0x68
  - Interrupt pin: GPIO 4
- **SparkFun Bio Sensor Hub (MAX30101)**: Pulse oximeter and heart rate
  - Reset pin: GPIO 13
  - MFIO pin: GPIO 14
- **MAX30205**: Medical-grade temperature sensor
  - I2C address: 0x48

### User Interface
- **SSD1306 OLED Display** (128x64)
  - I2C address: 0x3D
- **Buzzer**: GPIO 27
- **Touch Sensor**: GPIO 25 (for grip detection)
- **Panic Button**: GPIO 26
- **LED1**: GPIO 33 (Armed status indicator)
- **LED2**: GPIO 32 (BLE connection indicator)

### I2C Configuration
- SDA: GPIO 21
- SCL: GPIO 22
- Clock: 100 kHz

## Building the Firmware

### Prerequisites
- PlatformIO CLI or VS Code with PlatformIO extension
- ESP32 development board
- USB cable for programming

### Build Instructions

1. Navigate to the firmware directory:
   ```bash
   cd firmware
   ```

2. Build the project:
   ```bash
   pio run
   ```

3. Upload to ESP32:
   ```bash
   pio run --target upload
   ```

4. Monitor serial output:
   ```bash
   pio device monitor
   ```

### Dependencies

The project uses the following PlatformIO libraries (defined in `platformio.ini`):
- `adafruit/Adafruit BusIO @ ^1.16.0`
- `adafruit/Adafruit GFX Library @ ^1.11.10`
- `adafruit/Adafruit SSD1306 @ ^2.5.11`
- `bblanchon/ArduinoJson @ ^7.4.3`

## Fall Detection Algorithm

The fall detection system uses a calibrated multi-threshold approach based on real-world data captured from crutch-tip mounted IMU.

### Threshold Calibration

Based on captured events:
- 3 dropped-crutch fall events
- 3 held-crutch fall events
- 3 hard-knock false-alarm events
- ~41 minutes of normal walking data

### Detection Paths

**HELD (touch sensor active)**:
- Impact threshold: ~1.39 g (5700 LSB)
- Rotation threshold: ~150 °/s (2460 LSB)

**STRICT-A (unheld + free-fall detected)**:
- Same as HELD (free-fall is strong indicator)

**STRICT-B (unheld, no free-fall)**:
- Impact threshold: ~1.95 g (8000 LSB)
- Rotation threshold: ~350 °/s (5740 LSB)

### Performance

- All 6 recorded fall events trigger detection
- All 3 hard-knock false alarms are rejected
- Walking data: ~1 potential false positive per 40 minutes (worst case)

## Software Architecture

### Firmware Components

The firmware is organized into modular components:

- **Main.cpp**: Entry point and task scheduler
- **hardware_config.h**: Pin definitions and hardware configuration
- **sensor_imu.cpp**: MPU-6050 IMU data acquisition
- **sensor_vitals.cpp**: Bio hub and temperature sensor reading
- **fall_detection.cpp**: Multi-stage fall detection pipeline
- **alarm_buzzer.cpp**: Alert system with buzzer control
- **display_ui.cpp**: OLED display management
- **BLE_connection.cpp**: Bluetooth Low Energy communication
- **buttons.cpp**: Button and touch sensor handling
- **task_scheduler.cpp**: Cooperative task scheduler
- **telemetry_debug.cpp**: Serial debug output

### Task Scheduler

The system uses a cooperative task scheduler with the following tasks:

| Task | Interval | Description |
|------|----------|-------------|
| readIMU | 10 ms | 100 Hz IMU data acquisition (interrupt-driven) |
| updateDisplayAndStates | 50 ms | 20 Hz UI and display updates |
| printSerialDebug | 500 ms | Debug telemetry output |
| evaluateFall | 500 ms | Fall detection evaluation |
| readBioHub | 200 ms | Pulse oximeter reading |
| readTemperature | 3000 ms | Temperature sensor reading |
| updateGate0 | 200 ms | On-body detection and arming logic |
| bluetoothTask | 100 ms | BLE communication |
| alarmTask | 100 ms | Alarm state management |

## Companion Applications

The `app-suite/` directory contains three components:

### Backend API
Spring Boot REST API server that receives, stores, and serves telemetry data.

**Documentation**: See `app-suite/backend/COMPONENT.md`

**Database**: PostgreSQL (schema in `init.sql`)

### BLE Receiver
Flutter app that receives data from ESP32 via BLE and forwards to the backend API.

**Documentation**: See `app-suite/ble_receiver/COMPONENT.md`

### Data Monitor
Flutter app for caregivers to visualize real-time telemetry data and receive emergency alerts.

**Documentation**: See `app-suite/data_monitor/COMPONENT.md`

**API Documentation**: See `app-suite/telemetry_api.md`

## Documentation

### Competition Deliverables
- `final_documentation/FORCE_documentation.pdf` - Final project documentation
- `final_documentation/Timisoara1_FORCE_Design_Document.pdf` - Design document (day 3 of competition)

### Component Documentation
- `app-suite/backend/COMPONENT.md` - Backend API documentation
- `app-suite/ble_receiver/COMPONENT.md` - BLE receiver documentation
- `app-suite/data_monitor/COMPONENT.md` - Data monitor documentation

## Archive and Experiments

- **archive/**: Contains previous versions of the firmware (v1-v4) for historical reference
- **experiments/**: Contains experimental code including power management experiments (power-management) with putting inactive processes in hibernation - scrapped because it messed with the BLE connectivity - and buzzer testing (testBuzz)

## License

This project was developed for the Hard and Soft 2026 International Competition.
