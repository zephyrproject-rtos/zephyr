/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ZBUS_MESSAGES_H_
#define _ZBUS_MESSAGES_H_
#include <stdint.h>

struct version_msg {
	uint8_t major;
	uint8_t minor;
	uint16_t build;
};

struct foo_msg {
	int a;
	int b;
};

#endif /* _ZBUS_MESSAGES_H_ */
