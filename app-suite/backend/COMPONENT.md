# Backend API Component

Spring Boot REST API server that receives, stores, and serves telemetry data from the ESP32 fall detection device.

## Overview

This backend acts as the central data hub for the entire system:
- Receives telemetry data via HTTP POST from the ESP32 (or via BLE receiver app)
- Stores data in PostgreSQL database
- Serves data to the data monitor app for visualization
- Manages panic event alerts and acknowledgment

## Technology Stack

- **Framework**: Spring Boot (Java)
- **Database**: PostgreSQL
- **Deployment**: Docker (docker-compose.yml, Dockerfile)
- **Build Tool**: Gradle

## Database Schema

The database is initialized via `init.sql` with the following tables:

### vital_signs
- Stores heart rate, SpO2, and confidence readings from the Bio Hub sensor
- Indexed by timestamp for efficient time-series queries

### temperature
- Stores body temperature readings from MAX30205 sensor
- Indexed by timestamp

### motion
- Stores IMU data (rotation + acceleration) from MPU-6050
- 6 axes: rot_x, rot_y, rot_z, acc_x, acc_y, acc_z
- Indexed by timestamp

### panic_events
- Stores emergency alerts (panic button or fall detection)
- Tracks status: ACTIVE → ACKNOWLEDGED
- Records acknowledgment timestamp

## API Endpoints

See `telemetry_api.md` for complete API documentation including:
- Vital signs endpoints (POST/GET)
- Temperature endpoints (POST/GET)
- Motion endpoints (POST/GET)
- Panic events endpoints (POST/GET/PUT acknowledge)

## Configuration

### Current Hardcoded Values
- **Base URL**: `http://172.20.100.35:8080`
- **Authentication Token**: `esp32_static_token_12345`

### Database Connection
Database configuration is in Spring Boot application properties (typically `src/main/resources/application.properties` or `application.yml`). The exact location and values need to be documented by the original developer.

## Deployment

### Using Docker Compose
```bash
docker-compose up
```

### Using Docker
```bash
docker build -t backend-api .
docker run -p 8080:8080 backend-api
```

### Using Gradle
```bash
./gradlew bootRun
```

## Development

### Build
```bash
./gradlew build
```

### Run Tests
```bash
./gradlew test
```

## Security Notes

⚠️ **Security Concerns**:
- Authentication token is hardcoded and static
- API endpoint IP is hardcoded
- No HTTPS/TLS configured
- Token should be environment-based or use proper OAuth

## Dependencies

See `build.gradle` for full dependency list. Key dependencies include:
- Spring Boot Web
- Spring Boot Data JPA
- PostgreSQL Driver
