/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#define TYPE_RSP 0
#define TYPE_NO_RSP 1
#define TYPE_TEST_START 2
#define TYPE_TEST_END 3

struct data_packet {
	uint32_t type;
	uint8_t data[];
};

#endif /* __COMMON_H__ */
