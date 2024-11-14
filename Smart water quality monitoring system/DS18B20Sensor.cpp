#include "DS18B20Sensor.h"

OneWire oneWire(ONE_WIRE_BUS); // Declare OneWire object
DallasTemperature sensors(&oneWire); // Declare DallasTemperature object

void setupDS18B20Sensor() {
  oneWire.begin(ONE_WIRE_BUS); // Initialize OneWire object with pin
  sensors.begin();
}

float readTemperature() {
  sensors.requestTemperatures(); // Send the command to get temperatures
  return sensors.getTempCByIndex(0); // Get temperature in Celsius
}
