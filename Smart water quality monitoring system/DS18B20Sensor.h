#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 17 // Pin for DS18B20 sensor

void setupDS18B20Sensor();
float readTemperature();

#endif
