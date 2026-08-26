/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_PREPARE_WRITE_QUOTA_SRC_COMMON_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_PREPARE_WRITE_QUOTA_SRC_COMMON_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/uuid.h>

#include "host/att_internal.h"

#define SERVER_NAME "prepare write quota"

#define TEST_SERVICE_UUID                                                                          \
	BT_UUID_DECLARE_128(0x6b, 0x4d, 0x0e, 0x2a, 0x9a, 0x07, 0x4c, 0x1e, 0x93, 0x52, 0x1b,      \
			    0x2f, 0xd7, 0x81, 0x00, 0x00)
#define TEST_CHRC_UUID                                                                             \
	BT_UUID_DECLARE_128(0x6b, 0x4d, 0x0e, 0x2a, 0x9a, 0x07, 0x4c, 0x1e, 0x93, 0x52, 0x1b,      \
			    0x2f, 0xd7, 0x81, 0x00, 0x01)

/* Prepare write buffers the server lets one connection hold: the explicit
 * CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN, or else the equal share of the pool.
 */
#if CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN > 0
#define PREP_QUOTA CONFIG_BT_ATT_PREPARE_COUNT_PER_CONN
#else
#define PREP_QUOTA (CONFIG_BT_ATT_PREPARE_COUNT / CONFIG_BT_MAX_CONN)
#endif

/* Prepare Write Requests the writer's long write takes: an equal share of the pool */
#define WRITER_PREPARES (CONFIG_BT_ATT_PREPARE_COUNT / CONFIG_BT_MAX_CONN)

/* Whether the writer's long write must succeed: only if what the holder may
 * leave in the pool covers it, which the default limit guarantees and an
 * over-subscribed configuration does not.
 */
#define WRITER_EXPECT_SUCCESS ((CONFIG_BT_ATT_PREPARE_COUNT - PREP_QUOTA) >= WRITER_PREPARES)

/* Value octets carried by one Prepare Write Request at the default ATT MTU,
 * which both clients keep since neither of them exchanges the MTU.
 */
#define PREP_CHUNK_SIZE (BT_ATT_DEFAULT_LE_MTU - sizeof(struct bt_att_prepare_write_req) - 1)

/* Characteristic value size: a long write of it takes WRITER_PREPARES buffers. */
#define CHRC_SIZE (WRITER_PREPARES * PREP_CHUNK_SIZE)

/* Octet the holder's prepare writes are filled with, unlike the writer's value */
#define HOLDER_FILL 0xffU

/* The value the writer writes and the server expects: 1, 2, 3, ... */
static inline void writer_value_fill(uint8_t *value, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		value[i] = i + 1;
	}
}

void server_procedure(void);
void holder_procedure(void);
void writer_procedure(void);

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_PREPARE_WRITE_QUOTA_SRC_COMMON_H_ */
