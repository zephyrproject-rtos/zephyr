/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _MESSAGES_H_
#define _MESSAGES_H_
#include <stddef.h>
#include <stdint.h>

struct bm_msg {
	uint8_t bytes[CONFIG_BM_MESSAGE_SIZE];
};

#endif /* _MESSAGES_H_ */
