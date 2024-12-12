/*
 * Copyright (c) 2024, Etienne Raquin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_SYSCFG_ITLINE_STM32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_SYSCFG_ITLINE_STM32_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get itlineXX_mux device bound to given IRQ
 *
 * @param irq IRQ the itlineXX_mux device is bound to
 * @return itlineXX_mux device bound to given IRQ
 */
const struct device *get_itline_mux(uint8_t irq);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_SYSCFG_ITLINE_STM32_H_ */
