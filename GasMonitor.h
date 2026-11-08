#ifndef GAS_MONITOR_H
#define GAS_MONITOR_H

#define GAS_SENSOR_PIN 34
#define DHT_PIN 4
#define GREEN_LED_PIN 26
#define RED_LED_PIN 27
#define BUZZER_PIN 25

#define DHT_TYPE 11

#define CALIBRATION_SAMPLES 50
#define CALIBRATION_INTERVAL 100

#define BASE_PADDING 300
#define HUMIDITY_LIMIT 70.0
#define HUMIDITY_ADJUSTMENT 10.0
#define TEMPERATURE_LIMIT 20.0
#define TEMPERATURE_ADJUSTEMENT 10.0

#define HIGH_RISK_OFFSET 500
#define ACTIVE_BUZZER 1

#define GAS_SAMPLE_COUNT 10
#define GAS_SAMPLE_INTERVAL 5
#define DHT_READ_INTERVAL 2000

typedef enum
{
	GAS_SAFE = 0,
	GAS_MEDIUM_RISK = 1,
	GAS_HIGH_RISK = 2
} GasLevel;

void initializeSystem();
int readGasSensor();
void calibrateAmbientBaseLine();
int processGasData();

void readClimateData();
float getCompensatedThreshold();

GasLevel evaluateGasLevel(int gasValue);
void manageAlarm(GasLevel level);

void controlBuzzer(bool state);
void DIsplayStatus(int gasValue, GasLevel level);

#endif
