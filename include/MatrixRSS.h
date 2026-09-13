#pragma once

#ifndef MatrixRSS_H
#define MatrixRSS_H
#include <Arduino.h>

#include <ArduinoJson.h>


static String Hostname =
    String("Mtrx_") + String(ESP.getEfuseMac(), HEX).substring(0, 4);


//global
extern JsonDocument statusObject;

#endif