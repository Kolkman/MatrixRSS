#pragma once

#ifndef MatrixRSS_H
#define MatrixRSS_H
#include <Arduino.h>

#include <ArduinoJson.h>

#define DISPLAY_DELAY 2500 // Delay in microseconds, to hold display after message.
static String Hostname =
    String("Matrix_") + String(ESP.getEfuseMac(), HEX).substring(0, 4);


//global
extern JsonDocument statusObject;

#endif