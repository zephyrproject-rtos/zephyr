/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mocks/pinctrl_mocks.h>

DEFINE_FAKE_VALUE_FUNC(int,
                        pinctrl_lookup_state,
                        const struct pinctrl_dev_config*,
                        uint8_t,
                        const struct pinctrl_state**);
