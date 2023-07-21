/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mocks/gpio_mocks.h>

DEFINE_FAKE_VALUE_FUNC(int,
                        gpio_port_set_masked_raw,
                        const struct device*,
                        gpio_port_pins_t,
                        gpio_port_value_t);

DEFINE_FAKE_VALUE_FUNC(
    int, gpio_port_set_bits_raw, const struct device*, gpio_port_pins_t);

DEFINE_FAKE_VALUE_FUNC(
    int, gpio_port_clear_bits_raw, const struct device*, gpio_port_pins_t);

