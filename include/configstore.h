#ifndef MATRIXRSS_CONFIGSTORE_H
#define MATRIXRSS_CONFIGSTORE_H

#include <stddef.h>
#include <stdint.h>

bool initConfigStorage();
bool loadStoredWiFiSettings(char *ssid, size_t ssidSize, char *pass,
                            size_t passSize);
bool saveStoredWiFiSettings(const char *ssid, const char *pass);
bool loadStoredMQTTSettings(char *host, size_t hostSize, uint16_t *port,
                            char *user, size_t userSize, char *pass,
                            size_t passSize);
bool saveStoredMQTTSettings(const char *host, uint16_t port, const char *user,
                            const char *pass);

#endif