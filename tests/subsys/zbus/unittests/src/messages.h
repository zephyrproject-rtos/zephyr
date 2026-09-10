/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ZBUS_MESSAGES_H_
#define _ZBUS_MESSAGES_H_
#include <stdbool.h>
#include <stddef.h>
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
	char x;
	bool y;
};

struct action_msg {
	bool status;
};

struct s1_msg {
	int m;
	struct {
		int a, b, c;
	} field;
};

struct hard_msg {
	int16_t range;	  /* Range 0 to 1023 */
	uint8_t binary;	  /* Range 0 to 1 */
	int16_t *pointer; /* Not null */
};

#endif /* _ZBUS_MESSAGES_H_ */
