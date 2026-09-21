#ifndef MATRIXRSS_MQTT_H
#define MATRIXRSS_MQTT_H
#include <PubSubClient.h>
#include <stdint.h>
#define MAX_CONNECTION_RETRIES 10
#define MQTT_PORTAL_FAILOVER_MS (1UL * 60UL * 1000UL)

struct MQTTSettings {
	char host[64];
	uint16_t port;
	char user[64];
	char pass[64];
};

void MQTT_reconnect();
void MQTT_callback(char *, byte *, unsigned int);
void loadMQTTSettings();
const MQTTSettings &getMQTTSettings();
void saveMQTTSettings(const char *host, const char *port, const char *user,
											const char *pass);
void requestConfigPortalReboot(const char *reason);
void setupMQTT();
void loopMQTT();


#endif