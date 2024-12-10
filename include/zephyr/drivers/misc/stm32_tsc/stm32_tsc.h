/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MISC_STM32_TSC_H_
#define ZEPHYR_DRIVERS_MISC_STM32_TSC_H_

#include <zephyr/sys_clock.h>
#include <zephyr/types.h>

typedef void (*stm32_tsc_callback_t)(uint8_t group, uint32_t value, void *user_data);

void stm32_tsc_start(const struct device *dev);
void stm32_tsc_register_callback(const struct device *dev, stm32_tsc_callback_t cb,
				 void *user_data);

#endif /* ZEPHYR_DRIVERS_MISC_STM32_TSC_H_ */
