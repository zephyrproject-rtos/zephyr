/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_RC_RC_INTERNAL_H_
#define ZEPHYR_SUBSYS_RC_RC_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>

#define RC_FLAG_OVERFLOW     0
#define RC_FLAG_IDLE         1
#define RC_FLAG_CORE_TIMEOUT 2
#define RC_FLAG_TIMEOUT_AUTO 3

static inline struct rc_dev_data *rc_data(const struct device *dev)
{
	return dev->data;
}

int rc_protocol_init(const struct device *dev, const uint16_t *protocols, size_t num_protocols);

void rc_protocol_reset_all(const struct device *dev);

void rc_protocol_decode(const struct device *dev, uint32_t word);

uint32_t rc_protocol_min_timeout_us(const struct device *dev);

int rc_rx_timeout_update(const struct device *dev);

void rc_raw_init(const struct device *dev, bool core_timeout);

#endif /* ZEPHYR_SUBSYS_RC_RC_INTERNAL_H_ */
