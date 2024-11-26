#ifndef MATRIXRSS_MQTT_H
#define MATRIXRSS_MQTT_H
#define MAX_CONNECTION_RETRIES 10
void MQTT_reconnect();
void MQTT_callback(char *, byte *, unsigned int);
void setupMQTT();
void loopMQTT();



#endif