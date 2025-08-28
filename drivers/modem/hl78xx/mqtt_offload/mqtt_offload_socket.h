/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef HL78XX_MQTT_SOCKET_H
#define HL78XX_MQTT_SOCKET_H

#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Helper macros and constants */
#define HL78XX_MQTT_OFFLOAD_LENGTH_FIELD_EXTENDED_PREFIX 0x01

#define HL78XX_MQTT_OFFLOAD_FLAGS_MASK_QOS  (BIT(5) | BIT(6))
#define HL78XX_MQTT_OFFLOAD_FLAGS_SHIFT_QOS 5
#define HL78XX_MQTT_OFFLOAD_FLAGS_RETAIN    BIT(4)

#define SOL_MQTT 216

#define MQTT_CLIENT_ID 4

#define MQTT_PACKAGE_TYPE 3

#define MQTT_KEEP_ALIVE 5

#define MQTT_CLEAN_SESSION 6

#define MQTT_QOS 10

#define MQTT_WILL_RETAIN 11

enum hl78xx_mqtt_status_code {
	MQTT_CONNECTION_ABORTED = 0,     /** MQTT connection aborted error */
	MQTT_CONNECTION_SUCCESSFUL = 1,  /** CONNACK received from the MQTT broker */
	MQTT_SUBSCRIBE_SUCCESSFUL = 2,   /** SUBACK received from the MQTT broker */
	MQTT_UNSUBSCRIBE_SUCCESSFUL = 3, /** UNSUBACK received from the MQTT broker */
	MQTT_PUBLISH_SUCCESSFUL = 4,     /** PUBACK or PUBCOMP received from the MQTT broker */
	MQTT_GENERIC_ERROR = 5           /** MQTT generic error */
};

enum hl78xx_mqtt_msg_type {

	HL78XX_MQTT_OFFLOAD_MSG_TYPE_PUBLISH = 0x0C,
	HL78XX_MQTT_OFFLOAD_MSG_TYPE_SUBSCRIBE = 0x12,
	HL78XX_MQTT_OFFLOAD_MSG_TYPE_UNSUBSCRIBE = 0x14,
};

struct hl78xx_mqtt_status {
	enum hl78xx_mqtt_status_code err_code;
	bool is_connected;
};

struct hl78xx_mqtt_config {
	int keep_alive;
	int clean_session;
	int qos;
};
#ifdef __cplusplus
}
#endif

#endif /* HL78XX_MQTT_SOCKET_H */
