/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PINCTRL_MOCKS_H
#define PINCTRL_MOCKS_H

#include <zephyr/fff.h>

#include <zephyr/drivers/pinctrl.h>

DECLARE_FAKE_VALUE_FUNC(int,
                        pinctrl_lookup_state,
                        const struct pinctrl_dev_config*,
                        uint8_t,
                        const struct pinctrl_state**);

#endif /* PINCTRL_MOCKS_H */
