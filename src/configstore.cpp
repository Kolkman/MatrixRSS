#include "configstore.h"

#include "secrets.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

namespace {

constexpr char kConfigPath[] = "/config.json";
bool configStorageReady = false;

void copySetting(char *target, size_t targetSize, const String &value) {
  value.toCharArray(target, targetSize);
}

void populateDefaultConfig(JsonDocument &configDocument) {
  configDocument["wifi"]["ssid"] = WIFI_SSID;
  configDocument["wifi"]["pass"] = WIFI_PASS;
  configDocument["mqtt"]["host"] = MQTT_HOST;
  configDocument["mqtt"]["port"] = MQTT_PORT;
  configDocument["mqtt"]["user"] = MQTT_USER;
  configDocument["mqtt"]["pass"] = MQTT_PASS;
}

bool writeConfigDocument(JsonDocument &configDocument) {
  File configFile = LittleFS.open(kConfigPath, "w");
  if (!configFile) {
    Serial.println("[Config] Failed to open config file for writing");
    return false;
  }

  const size_t bytesWritten = serializeJson(configDocument, configFile);
  configFile.close();
  return bytesWritten > 0;
}

bool ensureConfigStorage() {
  if (configStorageReady) {
    return true;
  }

  if (!LittleFS.begin(true)) {
    Serial.println("[Config] LittleFS mount failed");
    return false;
  }

  configStorageReady = true;

  if (LittleFS.exists(kConfigPath)) {
    return true;
  }

  Serial.println("[Config] Creating default config file");
  JsonDocument configDocument;
  populateDefaultConfig(configDocument);
  return writeConfigDocument(configDocument);
}

bool loadConfigDocument(JsonDocument &configDocument) {
  if (!ensureConfigStorage()) {
    return false;
  }

  File configFile = LittleFS.open(kConfigPath, "r");
  if (!configFile) {
    Serial.println("[Config] Failed to open config file for reading");
    return false;
  }

  const DeserializationError error = deserializeJson(configDocument, configFile);
  configFile.close();
  if (!error) {
    return true;
  }

  Serial.println("[Config] Config parse failed, resetting to defaults");
  configDocument.clear();
  populateDefaultConfig(configDocument);
  return writeConfigDocument(configDocument);
}

}

bool initConfigStorage() { return ensureConfigStorage(); }

bool loadStoredWiFiSettings(char *ssid, size_t ssidSize, char *pass,
                            size_t passSize) {
  JsonDocument configDocument;
  if (!loadConfigDocument(configDocument)) {
    return false;
  }

  copySetting(ssid, ssidSize,
              configDocument["wifi"]["ssid"].as<String>());
  copySetting(pass, passSize,
              configDocument["wifi"]["pass"].as<String>());
  return strlen(ssid) > 0;
}

bool saveStoredWiFiSettings(const char *ssid, const char *pass) {
  JsonDocument configDocument;
  if (!loadConfigDocument(configDocument)) {
    return false;
  }

  configDocument["wifi"]["ssid"] = ssid == nullptr ? "" : ssid;
  configDocument["wifi"]["pass"] = pass == nullptr ? "" : pass;
  return writeConfigDocument(configDocument);
}

bool loadStoredMQTTSettings(char *host, size_t hostSize, uint16_t *port,
                            char *user, size_t userSize, char *pass,
                            size_t passSize) {
  JsonDocument configDocument;
  if (!loadConfigDocument(configDocument)) {
    return false;
  }

  copySetting(host, hostSize,
              configDocument["mqtt"]["host"].as<String>());
  *port = configDocument["mqtt"]["port"] | static_cast<uint16_t>(MQTT_PORT);
  copySetting(user, userSize,
              configDocument["mqtt"]["user"].as<String>());
  copySetting(pass, passSize,
              configDocument["mqtt"]["pass"].as<String>());
  return strlen(host) > 0;
}

bool saveStoredMQTTSettings(const char *host, uint16_t port, const char *user,
                            const char *pass) {
  JsonDocument configDocument;
  if (!loadConfigDocument(configDocument)) {
    return false;
  }

  configDocument["mqtt"]["host"] = host == nullptr ? "" : host;
  configDocument["mqtt"]["port"] = port;
  configDocument["mqtt"]["user"] = user == nullptr ? "" : user;
  configDocument["mqtt"]["pass"] = pass == nullptr ? "" : pass;
  return writeConfigDocument(configDocument);
}