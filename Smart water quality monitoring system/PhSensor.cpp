#include "PhSensor.h"
#include <Arduino.h>


void setupPhSensor(){
  pinMode(phSensorPin, INPUT);
}

int rawPhData(){
    return analogRead(phSensorPin);
}

float readPh(){
  float rawPhSensorData = analogRead(phSensorPin);

  float voltage = rawPhSensorData * (3.3 / 4095.0);

  return 3.3 * voltage;
}