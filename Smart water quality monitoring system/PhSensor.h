#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#define phSensorPin A6

void setupPhSensor();

int rawPhData();

float readPh();

#endif