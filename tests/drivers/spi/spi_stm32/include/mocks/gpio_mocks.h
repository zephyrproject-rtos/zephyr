/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GPIO_MOCKS_H
#define GPIO_MOCKS_H

#include <zephyr/fff.h>
#include <zephyr/drivers/gpio.h>

DECLARE_FAKE_VALUE_FUNC(int,
                        gpio_port_set_masked_raw,
                        const struct device*,
                        gpio_port_pins_t,
                        gpio_port_value_t);
DECLARE_FAKE_VALUE_FUNC(
    int, gpio_port_set_bits_raw, const struct device*, gpio_port_pins_t);
DECLARE_FAKE_VALUE_FUNC(
    int, gpio_port_clear_bits_raw, const struct device*, gpio_port_pins_t);

#endif /* GPIO_MOCKS_H */
