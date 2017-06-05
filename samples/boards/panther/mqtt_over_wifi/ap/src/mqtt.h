/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_MQTT_H_
#define SRC_MQTT_H_

#include <zephyr/types.h>

struct mqtt_client_prm {
	char *server_addr;
	u16_t server_port;
	char *client_id;
	char *user_id;
	char *password;
	char *topic;
};

int mqtt_pub_init(void *arg);
int mqtt_status_get(void);
int mqtt_send(char *topic, char *payload);


#endif /* SRC_MQTT_H_ */
