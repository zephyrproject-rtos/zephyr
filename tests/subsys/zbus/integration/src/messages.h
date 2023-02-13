/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ZBUS_MESSAGES_H_
#define _ZBUS_MESSAGES_H_
#include <stdbool.h>
#include <stdint.h>

struct version_msg {
	uint8_t major;
	uint8_t minor;
	uint16_t build;
};

struct sensor_data_msg {
	int a;
	int b;
};

struct net_pkt_msg {
	int total;
};

struct action_msg {
	bool status;
};

#endif /* _ZBUS_MESSAGES_H_ */
