/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _MESSAGES_H_
#define _MESSAGES_H_
#include <stddef.h>
#include <stdint.h>

struct version_msg {
	uint8_t major;
	uint8_t minor;
	uint16_t build;
};

struct external_data_msg {
	void *reference;
	size_t size;
};

struct ack_msg {
	uint8_t value;
};

#endif /* _MESSAGES_H_ */
