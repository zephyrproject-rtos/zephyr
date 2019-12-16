/*
 * Copyright (c) 2018 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MQTT-based cloud protocol engine.
 */

#ifndef PROTOCOL_H__
#define PROTOCOL_H__

#include <net/socket.h>

void mqtt_startup(char *hostname, int port);

#endif
