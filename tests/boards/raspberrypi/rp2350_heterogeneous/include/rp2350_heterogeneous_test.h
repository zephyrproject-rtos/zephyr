/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RP2350_HETEROGENEOUS_TEST_H_
#define RP2350_HETEROGENEOUS_TEST_H_

#include <stdint.h>

#define SHARED_STATUS_ADDR   0x20080000U
#define SHARED_MAGIC         0x485A3354U
#define MAILBOX_RESPONSE_XOR 0xA5A50000U

struct shared_status {
	uint32_t magic;
	uint32_t hart_id;
	uint32_t counter;
	uint32_t mailbox_ready;
	uint32_t mailbox_responses;
};

#endif /* RP2350_HETEROGENEOUS_TEST_H_ */
