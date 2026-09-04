/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SMBUS_TEST_I2C_TARGET_H_
#define SMBUS_TEST_I2C_TARGET_H_

#include <zephyr/device.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 7-bit target address used by the test target on i2c1 */
#define SMBUS_TEST_TARGET_ADDR 0x52

/* Register the test target on @p bus */
int test_target_start(const struct device *bus);

/* Unregister the test target. Safe to call even if not registered */
int test_target_stop(void);

/* Clear captured write data, the preloaded read queue, and all counters */
void test_target_reset(void);

/* Number of bytes captured during the most recent write-direction phase */
size_t test_target_write_len(void);

/* Captured write-direction bytes; valid until the next reset/transaction */
const uint8_t *test_target_write_buf(void);

/* Preload the bytes to hand back on the next read-direction phase */
void test_target_set_read_data(const uint8_t *data, size_t len);

#endif /* SMBUS_TEST_I2C_TARGET_H_ */
