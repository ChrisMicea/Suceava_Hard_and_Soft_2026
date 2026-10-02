#ifndef SENSOR_VITALS_H
#define SENSOR_VITALS_H

#include <Arduino.h>
#include <SparkFun_Bio_Sensor_Hub_Library.h>
#include "Protocentral_MAX30205.h"
#include "hardware_config.h"

// ── Shared Sensor Instantiations & Variables ──────────────────
extern SparkFun_Bio_Sensor_Hub bioHub; 
extern MAX30205 tempSensor;
extern bioData body;  

extern bool tempSensorReady;
extern bool bioHubReady; 
extern float globalBodyTemp;

// ── Task: Pulse Oximeter & Bio Hub Execution (Every 200ms) ────
void readBioHub();

// ── Task: Temperature Sensor Evaluation (Every 3s) ─────────────
void readTemperature();

// ── Initialize Bio Hub ────────────────────────────────────────
void initializeBioHub();

// ── Initialize Temperature Sensor ─────────────────────────────
void initializeTemperatureSensor();

#endif
