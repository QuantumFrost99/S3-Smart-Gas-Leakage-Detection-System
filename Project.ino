#include <Arduino.h>
#include <DHT.h>
#include <math.h>
#include "GasMonitor.h"

DHT dht(DHT_PIN,DHT_TYPE); // creates an object so that ESP32 can interact with the DHT11 sensor
int gasReadings[GAS_SAMPLE_COUNT];
float temperature = NAN;
float humidity = NAN;
bool climateDataValid = false;
float ambientBaseLine = 0.0;
GasLevel currentLevel = GAS_SAFE;

unsigned long lastDHTRead = 0;
unsigned long lastBuzzerChange = 0;
unsigned long lastStatusDisplay = 0;
bool buzzerState = false;

void initializeSystem(){
  pinMode(GREEN_LED_PIN,OUTPUT);
  pinMode(RED_LED_PIN,OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  analogReadResolution(12); // for ADC where range will be from 0 to 4095

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.begin(115200); 
  dht.begin();
  Serial.println("Smart Gas Leakage Detection System");
}

int readGasSensor(){
  return analogRead(GAS_SENSOR_PIN);
}

void calibrateAmbientBaseline(){
  long total = 0;
  Serial.println("Calibrating Sensor Readings!!");
  for(int i = 0; i < CALIBRATION_SAMPLES; i++){
    int reading = readGasSensor();
    total += reading;
    delay(CALIBRATION_INTERVAL);
  }
  ambientBaseLine = (float) total / CALIBRATION_SAMPLES;
  Serial.println("Ambient BaseLine: ");
  Serial.println(ambientBaseLine);  
}

int processGasData(){
  long total = 0;
  for(int i = 0; i < GAS_SAMPLE_COUNT; i++){
    gasReadings[i] = readGasSensor();
    total += gasReadings[i];
    delay(GAS_SAMPLE_INTERVAL);
  }
  return (int) (total / GAS_SAMPLE_COUNT);
}

void readClimateData(){
  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature();
  if(isnan(newHumidity) || isnan(newTemperature)){
    climateDataValid = false;
    Serial.println("DHT11 Disconnected or reading failed");
    return;
  }
  humidity = newHumidity;
  temperature = newTemperature;
  climateDataValid = true;
}

float getCompensatedThreshold(){
  float padding = BASE_PADDING;
  if(climateDataValid && humidity > HUMIDITY_LIMIT){
    float excessHumidity = humidity - HUMIDITY_LIMIT;
    padding += excessHumidity * HUMIDITY_ADJUSTMENT;
  }
  if(climateDataValid && temperature < TEMPERATURE_LIMIT){
    float temperatureDiff = TEMPERATURE_LIMIT - temperature;
    padding -= temperatureDiff * TEMPERATURE_ADJUSTEMENT;
  }
  if(padding < 0 ){padding = 0;}
  return ambientBaseLine + padding;
}

GasLevel evaluateGasLevel(int gasValue){
  float dynamicThreshold = getCompensatedThreshold();
  float highRiskThreshold = dynamicThreshold + HIGH_RISK_OFFSET;
  if(gasValue  >= highRiskThreshold){
    return GAS_HIGH_RISK;
  }
  if(gasValue  >= dynamicThreshold){
    return GAS_MEDIUM_RISK;
  }
  return GAS_SAFE;
}

void controlBuzzer(bool state){
  if(state){
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else{
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void manageAlarm(GasLevel level){
  unsigned long currentTime = millis();
  unsigned long beepDuration;
  unsigned long silentDuration;

  static GasLevel previousLevel = GAS_SAFE;
  if(level == GAS_SAFE){
    digitalWrite(GREEN_LED_PIN,HIGH);
    digitalWrite(RED_LED_PIN,LOW);
    controlBuzzer(false);
    buzzerState = false;
    previousLevel = GAS_SAFE;
    return;
  }
  digitalWrite(GREEN_LED_PIN,LOW);
  digitalWrite(RED_LED_PIN,HIGH);
  if(level != previousLevel){
    buzzerState = false;
    controlBuzzer(false);
    lastBuzzerChange = currentTime;
    previousLevel = level;
  }
  if(level == GAS_MEDIUM_RISK){
    beepDuration = 300;
    silentDuration = 1000;
  }
  else{
    beepDuration = 100;
    silentDuration = 100;
  }
  unsigned long requiredDuration = buzzerState? beepDuration : silentDuration;
  if(currentTime - lastBuzzerChange >= requiredDuration){
    buzzerState = !buzzerState;
    controlBuzzer(buzzerState);
    lastBuzzerChange = currentTime;
  }
}

void displayStatus(int gasValue, GasLevel level){
  Serial.println();
  Serial.print("MQ-5 Reading: ");
  Serial.println(gasValue);
  
  Serial.print("Clean-Air Baseline: ");
  Serial.println(ambientBaseLine);
  
  Serial.print("Dynamic Threshold: ");
  Serial.println(getCompensatedThreshold());
  if(climateDataValid){
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }
  else{
    Serial.println("Climate Data Unavailable. ");
  }
  Serial.print("Risk Level: ");
  switch(level){
    case GAS_SAFE:
      Serial.println("SAFE");
      break;
    case GAS_MEDIUM_RISK:
      Serial.println("MEDIUM RISK - POSSIBLE GAS LEAK");
      break;
    case GAS_HIGH_RISK:
      Serial.println("HIGH RISK - SEVERE GAS LEAK");
      break;
  }

  if(level == GAS_HIGH_RISK){Serial.println("WARNING: HIGH GAS LEVEL DETECTED!");}
  Serial.println();
}

void setup(){
  initializeSystem();
  delay(2000);
  readClimateData();
  calibrateAmbientBaseline();
  currentLevel = GAS_SAFE;
  manageAlarm(GAS_SAFE);
}

void loop(){
  unsigned long currentTime = millis();
  if(currentTime  - lastDHTRead >= DHT_READ_INTERVAL){
    readClimateData(); // to read values from DHT11
    lastDHTRead = currentTime;
  }
  int gasValue = processGasData();
  currentLevel = evaluateGasLevel(gasValue);
  manageAlarm(currentLevel);
  if(currentTime - lastStatusDisplay >= 1000){
    displayStatus(gasValue, currentLevel);
    lastStatusDisplay = currentTime;
  }
  delay(50);
}
