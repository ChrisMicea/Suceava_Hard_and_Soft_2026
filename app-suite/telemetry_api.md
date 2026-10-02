# Telemetry API Documentation

Base URL: `http://172.20.100.35:8080`

All POST endpoints require token authentication via Bearer token: `esp32_static_token_12345`

## Vital Signs Endpoints

### POST /api/vital-signs
Submit vital signs data (heart rate, oxygen level, confidence)

**Headers:**
- `Authorization: Bearer esp32_static_token_12345`
- `Content-Type: application/json`

**Request Body:**
```json
{
  "heartrate": 72,
  "oxygen": 98.5,
  "confidence": 0.95
}
```

**Response:** 201 Created
```json
{
  "id": 1,
  "heartrate": 72,
  "oxygen": 98.5,
  "confidence": 0.95,
  "createdAt": "2026-05-19T10:30:45"
}
```

**curl Command:**
```bash
curl -X POST http://172.20.100.35:8080/api/vital-signs \
  -H "Authorization: Bearer esp32_static_token_12345" \
  -H "Content-Type: application/json" \
  -d '{"heartrate": 72, "oxygen": 98.5, "confidence": 0.95}'
```

### GET /api/vital-signs
Retrieve all vital signs records

**Response:** 200 OK
```json
[
  {
    "id": 1,
    "heartrate": 72,
    "oxygen": 98.5,
    "confidence": 0.95,
    "createdAt": "2026-05-19T10:30:45"
  }
]
```

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/vital-signs
```

### GET /api/vital-signs/{id}
Retrieve specific vital signs record by ID

**Response:** 200 OK or 404 Not Found

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/vital-signs/1
```

### GET /api/vital-signs/latest
Retrieve most recent vital signs record

**Response:** 200 OK or 404 Not Found

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/vital-signs/latest
```

---

## Temperature Endpoints

### POST /api/temperature
Submit temperature data

**Headers:**
- `Authorization: Bearer esp32_static_token_12345`
- `Content-Type: application/json`

**Request Body:**
```json
{
  "temperature": 23.5
}
```

**Response:** 201 Created
```json
{
  "id": 1,
  "temperature": 23.5,
  "createdAt": "2026-05-19T10:30:45"
}
```

**curl Command:**
```bash
curl -X POST http://172.20.100.35:8080/api/temperature \
  -H "Authorization: Bearer esp32_static_token_12345" \
  -H "Content-Type: application/json" \
  -d '{"temperature": 23.5}'
```

### GET /api/temperature
Retrieve all temperature records

**Response:** 200 OK

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/temperature
```

### GET /api/temperature/{id}
Retrieve specific temperature record by ID

**Response:** 200 OK or 404 Not Found

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/temperature/1
```

### GET /api/temperature/latest
Retrieve most recent temperature record

**Response:** 200 OK or 404 Not Found

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/temperature/latest
```

---

## Motion Endpoints

### POST /api/motion
Submit motion data (rotation and acceleration)

**Headers:**
- `Authorization: Bearer esp32_static_token_12345`
- `Content-Type: application/json`

**Request Body:**
```json
{
  "rotX": 10.5,
  "rotY": -5.2,
  "rotZ": 15.8,
  "accX": 0.1,
  "accY": 0.2,
  "accZ": 9.8
}
```

**Response:** 201 Created
```json
{
  "id": 1,
  "rotX": 10.5,
  "rotY": -5.2,
  "rotZ": 15.8,
  "accX": 0.1,
  "accY": 0.2,
  "accZ": 9.8,
  "createdAt": "2026-05-19T10:30:45"
}
```

**curl Command:**
```bash
curl -X POST http://172.20.100.35:8080/api/motion \
  -H "Authorization: Bearer esp32_static_token_12345" \
  -H "Content-Type: application/json" \
  -d '{"rotX": 10.5, "rotY": -5.2, "rotZ": 15.8, "accX": 0.1, "accY": 0.2, "accZ": 9.8}'
```

### GET /api/motion
Retrieve all motion records

**Response:** 200 OK

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/motion
```

### GET /api/motion/{id}
Retrieve specific motion record by ID

**Response:** 200 OK or 404 Not Found

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/motion/1
```

### GET /api/motion/latest
Retrieve most recent motion record

**Response:** 200 OK or 404 Not Found

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/motion/latest
```

---

## Error Responses

### 400 Bad Request
Missing required fields in request body

### 401 Unauthorized
Invalid or missing Bearer token

### 404 Not Found
Resource not found (for GET by ID or when no data exists for latest)

### 500 Internal Server Error
Server-side error

---

## Database Schema

### vital_signs table
- `id` (BIGSERIAL PRIMARY KEY)
- `heartrate` (INTEGER)
- `oxygen` (FLOAT)
- `confidence` (FLOAT)
- `created_at` (TIMESTAMP)

### temperature table
- `id` (BIGSERIAL PRIMARY KEY)
- `temperature` (FLOAT)
- `created_at` (TIMESTAMP)

### motion table
- `id` (BIGSERIAL PRIMARY KEY)
- `rot_x` (FLOAT)
- `rot_y` (FLOAT)
- `rot_z` (FLOAT)
- `acc_x` (FLOAT)
- `acc_y` (FLOAT)
- `acc_z` (FLOAT)
- `created_at` (TIMESTAMP)

### panic_events table
- `id` (BIGSERIAL PRIMARY KEY)
- `event_type` (VARCHAR(50))
- `status` (VARCHAR(50)) - ACTIVE or ACKNOWLEDGED
- `created_at` (TIMESTAMP)
- `acknowledged_at` (TIMESTAMP, nullable)

---

## Panic Events Endpoints

### POST /api/panic-events
Submit panic alert (triggered by panic button or fall detection)

**Headers:**
- `Authorization: Bearer esp32_static_token_12345`
- `Content-Type: application/json`

**Request Body:**
```json
{
  "eventType": "PANIC_BUTTON"
}
```

**Response:** 201 Created
```json
{
  "id": 1,
  "eventType": "PANIC_BUTTON",
  "status": "ACTIVE",
  "createdAt": "2026-05-19T10:30:45",
  "acknowledgedAt": null
}
```

**curl Command:**
```bash
curl -X POST http://172.20.100.35:8080/api/panic-events \
  -H "Authorization: Bearer esp32_static_token_12345" \
  -H "Content-Type: application/json" \
  -d '{"eventType": "PANIC_BUTTON"}'
```

### GET /api/panic-events/latest
Retrieve most recent active panic alert

**Response:** 200 OK or 404 Not Found
```json
{
  "id": 1,
  "eventType": "PANIC_BUTTON",
  "status": "ACTIVE",
  "createdAt": "2026-05-19T10:30:45",
  "acknowledgedAt": null
}
```

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/panic-events/latest
```

### GET /api/panic-events
Retrieve all panic alerts

**Response:** 200 OK
```json
[
  {
    "id": 1,
    "eventType": "PANIC_BUTTON",
    "status": "ACKNOWLEDGED",
    "createdAt": "2026-05-19T10:30:45",
    "acknowledgedAt": "2026-05-19T10:31:20"
  },
  {
    "id": 2,
    "eventType": "FALL_DETECTION",
    "status": "ACTIVE",
    "createdAt": "2026-05-19T10:35:10",
    "acknowledgedAt": null
  }
]
```

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/panic-events
```

### GET /api/panic-events/{id}
Retrieve specific panic alert by ID

**Response:** 200 OK or 404 Not Found

**curl Command:**
```bash
curl http://172.20.100.35:8080/api/panic-events/1
```

### PUT /api/panic-events/{id}/acknowledge
Acknowledge a panic alert (mark as read by caregiver)

**Response:** 200 OK
```json
{
  "id": 1,
  "eventType": "PANIC_BUTTON",
  "status": "ACKNOWLEDGED",
  "createdAt": "2026-05-19T10:30:45",
  "acknowledgedAt": "2026-05-19T10:31:20"
}
```

**Response:** 404 Not Found (if alert ID doesn't exist)

**curl Command:**
```bash
curl -X PUT http://172.20.100.35:8080/api/panic-events/1/acknowledge
```
