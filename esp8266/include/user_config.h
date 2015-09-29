#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#define CFG_HOLDER	0x00FF55A8
#define CFG_LOCATION	0x3C	/* Please don't change or if you know what you doing */

/*DEFAULT CONFIGURATIONS*/

#warning "Change your MQTT broker, SSID and password here!"

#define MQTT_HOST			"iot.eclipse.org"
#define MQTT_PORT			1883

#define MQTT_BUF_SIZE		1024
#define MQTT_KEEPALIVE		30 // 120	 /*second*/

#define MQTT_CLIENT_ID		"DVES_%08X"
#define MQTT_USER			""
#define MQTT_PASS			""


#define STA_SSID "Your SSID"
#define STA_PASS "Your secret password"
#define STA_TYPE AUTH_WPA2_PSK

#define MQTT_RECONNECT_TIMEOUT 	10 // 5	/*second*/

#define CLIENT_SSL_ENABLE
#define DEFAULT_SECURITY	0

#define QUEUE_BUFFER_SIZE		 		2048

#endif
