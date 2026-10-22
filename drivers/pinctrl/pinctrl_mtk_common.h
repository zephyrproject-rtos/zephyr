/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PINCTRL_PINCTRL_MTK_COMMON_H_
#define ZEPHYR_DRIVERS_PINCTRL_PINCTRL_MTK_COMMON_H_

#include <stdint.h>

typedef struct {
	uint16_t max_pin;
	uint16_t max_func;
} pinctrl_mtk_config_t;

#endif /* ZEPHYR_DRIVERS_PINCTRL_PINCTRL_MTK_COMMON_H_ */
