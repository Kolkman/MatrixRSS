#include "mqtt.h"
#include "MatrixRSS.h"
#include "configstore.h"
#include "contentcontainer.h"
#include "debug.h"
#include "secrets.h"
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <algorithm>

#undef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h>

#define MQTT_OUTDOOR_TEMP "home/buitenTemp"
#define MQTT_OUTDOOR_HUMIDITY "home/buitenHumidity"
#define MQTT_INDOOR_TEMP "home/binnenTemp"
#define MQTT_INDOOR_HUMIDITY "home/binnenHumidity"
#define MQTT_NEWS "rss/news"

namespace {

MQTTSettings mqttSettings;

void copySetting(char *target, size_t targetSize, const String &value) {
  value.toCharArray(target, targetSize);
}

void resetMQTTSettingsToDefaults() {
  copySetting(mqttSettings.host, sizeof(mqttSettings.host), MQTT_HOST);
  mqttSettings.port = static_cast<uint16_t>(MQTT_PORT);
  copySetting(mqttSettings.user, sizeof(mqttSettings.user), MQTT_USER);
  copySetting(mqttSettings.pass, sizeof(mqttSettings.pass), MQTT_PASS);
}

uint16_t parsePort(const char *portValue) {
  if (portValue == nullptr || strlen(portValue) == 0) {
    return static_cast<uint16_t>(MQTT_PORT);
  }

  const long parsedPort = strtol(portValue, nullptr, 10);
  if (parsedPort <= 0 || parsedPort > 65535) {
    return static_cast<uint16_t>(MQTT_PORT);
  }
  return static_cast<uint16_t>(parsedPort);
}

}

extern ContentContainer container;

WiFiClient espClient;
PubSubClient client(espClient);
double mqttOutdoorTemperature;
double mqttOutdoorHumidity;
double mqttIndoorTemperature;
double mqttIndoorHumidity;
unsigned long previouspub = 0;
unsigned long mqttFailureSince = 0;

static void clearMQTTFailureWindow() { mqttFailureSince = 0; }

static void trackMQTTFailure(int mqttState) {
  if (WiFi.status() != WL_CONNECTED) {
    clearMQTTFailureWindow();
    return;
  }

  if (mqttState != MQTT_CONNECTION_TIMEOUT &&
      mqttState != MQTT_CONNECTION_LOST &&
      mqttState != MQTT_CONNECT_FAILED) {
    clearMQTTFailureWindow();
    return;
  }

  if (mqttFailureSince == 0) {
    mqttFailureSince = millis();
    return;
  }

  if (millis() - mqttFailureSince < MQTT_PORTAL_FAILOVER_MS) {
    return;
  }

  requestConfigPortalReboot("MQTT socket failure");
}

void MQTT_reconnect() {
  if (!client.connected()) {
    LOGINFO0("Attempting MQTT connection...");

    LOGINFO1("MQTT Client  name:", Hostname);
    const bool useAuth = strlen(mqttSettings.user) > 0;
    const bool connected = useAuth
                               ? client.connect(Hostname.c_str(),
                                                mqttSettings.user,
                                                mqttSettings.pass)
                               : client.connect(Hostname.c_str());

    if (connected) {
      clearMQTTFailureWindow();
      LOGINFO("connected");
      LOGINFO1("MQTT subscribing to: ", MQTT_OUTDOOR_TEMP);
      client.subscribe(MQTT_OUTDOOR_TEMP); // We should be OK with QOS 0
      LOGINFO1("MQTT subscribing to: ", MQTT_OUTDOOR_HUMIDITY);
      client.subscribe(MQTT_OUTDOOR_HUMIDITY); // We should be OK with QOS 0
      LOGINFO1("MQTT subscribing to: ", MQTT_INDOOR_TEMP);
      client.subscribe(MQTT_INDOOR_TEMP); // We should be OK with QOS 0
      LOGINFO1("MQTT subscribing to: ", MQTT_INDOOR_HUMIDITY);
      client.subscribe(MQTT_INDOOR_HUMIDITY); // We should be OK with QOS 0
      LOGINFO1("MQTT subscribing to: ", MQTT_NEWS);
      client.subscribe(MQTT_NEWS); // We should be OK with QOS 0
      LOGINFO("MQTT Subscription Passed")

    } else {
      const int mqttState = client.state();
      LOGINFO1("failed, rc=", mqttState);
      trackMQTTFailure(mqttState);
    }
  }
  LOGDEBUG0("MQTT_reconnect Returns");
}

void MQTT_callback(char *topic, byte *payload, unsigned int length) {
  LOGDEBUG2("Message arrived [", topic, "] '");

  if (strcmp(topic, MQTT_OUTDOOR_TEMP) == 0) {
    // obvioulsy state of my red LED

    char msg[ELEMENT_LENGTH];
    for (int i = 0; i < min(length, (unsigned int)ELEMENT_LENGTH); i++) {
      LOGDEBUG0((char)payload[i]);
      msg[i] = (char)payload[i];
    }

    msg[ELEMENT_LENGTH - 1] = '\0'; // to be sure

    mqttOutdoorTemperature = atof(msg);
    LOGINFO0(mqttOutdoorTemperature);
  }

  if (strcmp(topic, MQTT_OUTDOOR_HUMIDITY) == 0) {
    char msg[ELEMENT_LENGTH];
    unsigned int messageLength = min(length, (unsigned int)(ELEMENT_LENGTH - 1));
    memcpy(msg, payload, messageLength);
    msg[messageLength] = '\0';
    mqttOutdoorHumidity = atof(msg);
    LOGINFO0(mqttOutdoorHumidity);
  }

  if (strcmp(topic, MQTT_INDOOR_TEMP) == 0) {
    char msg[ELEMENT_LENGTH];
    unsigned int messageLength = min(length, (unsigned int)(ELEMENT_LENGTH - 1));
    memcpy(msg, payload, messageLength);
    msg[messageLength] = '\0';
    mqttIndoorTemperature = atof(msg);
    LOGINFO0(mqttIndoorTemperature);
  }

  if (strcmp(topic, MQTT_INDOOR_HUMIDITY) == 0) {
    char msg[ELEMENT_LENGTH];
    unsigned int messageLength = min(length, (unsigned int)(ELEMENT_LENGTH - 1));
    memcpy(msg, payload, messageLength);
    msg[messageLength] = '\0';
    mqttIndoorHumidity = atof(msg);
    LOGINFO0(mqttIndoorHumidity);
  }

  if (strcmp(topic, MQTT_NEWS) == 0) {
    // obvioulsy state of my red LED

    char msg[ELEMENT_LENGTH];

    unsigned int j = 0;
    for (unsigned int i = 0; i < length && j < ELEMENT_LENGTH; i++) {

      if ((char)payload[i] == '\n') {
        continue;
      }
      if ((char)payload[i] == '\r') {
        continue;
      }
      if ((char)payload[i] == '\t') {
        continue;
      }
      msg[j] = (char)payload[i];

      LOGDEBUG((char)payload[i]);
      j++;
    }

    msg[min(j, (unsigned int)(ELEMENT_LENGTH - 1))] = '\0';
    LOGDEBUG('\n');
    container.addcontent(msg);
    LOGINFO(msg);
  }
}

void loadMQTTSettings() {
  resetMQTTSettingsToDefaults();
  initConfigStorage();
  loadStoredMQTTSettings(mqttSettings.host, sizeof(mqttSettings.host),
                         &mqttSettings.port, mqttSettings.user,
                         sizeof(mqttSettings.user), mqttSettings.pass,
                         sizeof(mqttSettings.pass));
}

const MQTTSettings &getMQTTSettings() { return mqttSettings; }

void saveMQTTSettings(const char *host, const char *port, const char *user,
                      const char *pass) {
  initConfigStorage();
  saveStoredMQTTSettings(host, parsePort(port), user, pass);
  loadMQTTSettings();
}

void setupMQTT() {
  mqttOutdoorTemperature = 150.0;
  mqttOutdoorHumidity = 150.0;
  mqttIndoorTemperature = 150.0;
  mqttIndoorHumidity = 150.0;
  client.setServer(mqttSettings.host, mqttSettings.port);
  client.setCallback(MQTT_callback);
}

void loopMQTT() {
  unsigned long timestamp = millis();
  for (int i = 0; i < MAX_CONNECTION_RETRIES && !client.connected(); i++) {

    LOGINFO0("MQTT Reconnection Attempt");
    MQTT_reconnect();
    delay(100);
  }

  if (client.connected()) {
    clearMQTTFailureWindow();
  }

  if (((timestamp - previouspub) > 60 * 1000) || (previouspub > timestamp)) {
    char pubbuf[256];
    size_t n = serializeJson(statusObject, pubbuf);
    client.publish(Hostname.c_str(), pubbuf, n);
    previouspub = timestamp;
  }
  client.loop();
}
