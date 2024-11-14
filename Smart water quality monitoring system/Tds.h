#ifndef TDS_SENSOR_H
#define TDS_SENSOR_H

#define TdsSensorPin 36

#define VREF 3.3         // analog reference voltage(Volt) of the ADC
#define SCOUNT 30

void setupTdsSensor();

float readTDS(float temp);

#endif 