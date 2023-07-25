/*
 * Copyright (c) 2023 Caspar Friedrich <c.s.w.friedrich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_W1_W1_DS2482_800_H_
#define ZEPHYR_DRIVERS_W1_W1_DS2482_800_H_

#include <zephyr/drivers/i2c.h>

int ds2482_change_bus_lock_impl(const struct device *dev, bool lock);

#endif /* ZEPHYR_DRIVERS_W1_W1_DS2482_800_H_ */
