#ifndef ESPRressoMach_WEB_UPD_H
#define ESPRressoMach_WEB_UPD_H
#include <Arduino.h>
#include "ESPAsyncWebServer.h"
class webInterfaceOTAUpdate
{
public:
    webInterfaceOTAUpdate();
    void begin(AsyncWebServer *server);
    void handleDoUpdate(AsyncWebServerRequest *, const String &, size_t, uint8_t *, size_t, bool);
    void printProgress(size_t, size_t);
    void restart();

private:
    AsyncWebServer *_server;
    size_t content_len;
    bool UpdateError;

};


extern webInterfaceOTAUpdate webOTAUpdate;


#endif