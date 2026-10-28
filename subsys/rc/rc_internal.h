/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_RC_RC_INTERNAL_H_
#define ZEPHYR_SUBSYS_RC_RC_INTERNAL_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>

static inline struct rc_dev_data *rc_data(const struct device *dev)
{
	return dev->data;
}

int rc_protocol_init(const struct device *dev, const uint16_t *protocols, size_t num_protocols);

#endif /* ZEPHYR_SUBSYS_RC_RC_INTERNAL_H_ */
