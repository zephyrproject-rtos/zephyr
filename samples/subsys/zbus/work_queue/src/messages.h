/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _MESSAGES_H_
#define _MESSAGES_H_
#include <stdint.h>

struct version_msg {
	uint8_t major;
	uint8_t minor;
	uint16_t build;
};

struct sensor_msg {
	uint32_t temp;
	uint32_t press;
	uint32_t humidity;
};

#endif /* _MESSAGES_H_ */
