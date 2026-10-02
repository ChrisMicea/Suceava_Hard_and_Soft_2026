-- Vital Signs Table
CREATE TABLE IF NOT EXISTS vital_signs (
    id BIGSERIAL PRIMARY KEY,
    heartrate INTEGER NOT NULL,
    oxygen FLOAT NOT NULL,
    confidence FLOAT NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_vital_signs_created_at ON vital_signs(created_at);

-- Temperature Table
CREATE TABLE IF NOT EXISTS temperature (
    id BIGSERIAL PRIMARY KEY,
    temperature FLOAT NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_temperature_created_at ON temperature(created_at);

-- Motion Data Table (Rotation + Acceleration)
CREATE TABLE IF NOT EXISTS motion (
    id BIGSERIAL PRIMARY KEY,
    rot_x FLOAT NOT NULL,
    rot_y FLOAT NOT NULL,
    rot_z FLOAT NOT NULL,
    acc_x FLOAT NOT NULL,
    acc_y FLOAT NOT NULL,
    acc_z FLOAT NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_motion_created_at ON motion(created_at);

-- Panic Events Table
CREATE TABLE IF NOT EXISTS panic_events (
    id BIGSERIAL PRIMARY KEY,
    event_type VARCHAR(50) NOT NULL,
    status VARCHAR(50) DEFAULT 'ACTIVE',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    acknowledged_at TIMESTAMP
);
CREATE INDEX IF NOT EXISTS idx_panic_events_created_at ON panic_events(created_at);
CREATE INDEX IF NOT EXISTS idx_panic_events_status ON panic_events(status);

