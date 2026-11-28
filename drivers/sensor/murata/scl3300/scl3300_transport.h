/*
 * Copyright (c) 2026 Sacra Systems Private Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MURATA_SCL3300_SCL3300_TRANSPORT_H_
#define ZEPHYR_DRIVERS_SENSOR_MURATA_SCL3300_SCL3300_TRANSPORT_H_

#include <stdint.h>
#include <zephyr/rtio/rtio.h>

typedef int (*scl3300_write_ptr)(void *handle, uint32_t command);

typedef int (*scl3300_read_ptr)(const struct device *dev, uint32_t command, uint8_t *rx_buf);

typedef struct {
	scl3300_write_ptr write;
	scl3300_read_ptr read;
	void *handle;
} scl3300_ctx_t;

int scl3300_transport_read(const scl3300_ctx_t *ctx, uint32_t command, uint8_t *rx_buf);

int scl3300_transport_write(const scl3300_ctx_t *ctx, uint32_t command);

int32_t scl3300_write_command(const scl3300_ctx_t *ctx, uint32_t command);
int32_t scl3300_read_command(const scl3300_ctx_t *ctx, uint32_t command, uint8_t *rx_buf);

#endif /* ZEPHYR_DRIVERS_SENSOR_MURATA_SCL3300_SCL3300_TRANSPORT_H_ */
