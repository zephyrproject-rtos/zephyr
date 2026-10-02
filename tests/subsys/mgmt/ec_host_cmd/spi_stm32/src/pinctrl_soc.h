/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef PINCTRL_SOC_H_
#define PINCTRL_SOC_H_
#include <zephyr/types.h>
typedef uint32_t pinctrl_soc_pin_t;
#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx) 0,
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)     {0}
#endif /* PINCTRL_SOC_H_ */
