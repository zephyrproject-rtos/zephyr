/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTC_WKPU_NXP_S32_PRIV_H_
#define ZEPHYR_DRIVERS_INTC_WKPU_NXP_S32_PRIV_H_

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/intc_wkpu_nxp_s32.h>

/* GPIO configuration must register its callback and enable the IRQ atomically. */
int wkpu_nxp_s32_configure_interrupt(const struct device *dev, uint8_t irq, uint8_t pin,
				     enum wkpu_nxp_s32_trigger trigger,
				     wkpu_nxp_s32_callback_t cb, void *arg);

/*
 * Internal early-boot handoff: call after BSS initialization, before WKPU
 * device initialization and before clearing or resetting the captured status.
 * The caller determines whether this boot followed a wakeup. This function
 * only saves the supplied bits; it does not acknowledge hardware status.
 */
void wkpu_nxp_s32_wuc_capture(const struct device *dev, uint64_t status);

int wkpu_nxp_s32_wuc_enable(const struct device *dev, uint32_t id, bool enable);
int wkpu_nxp_s32_wuc_triggered(const struct device *dev, uint32_t id);
int wkpu_nxp_s32_wuc_clear(const struct device *dev, uint32_t id);

#endif /* ZEPHYR_DRIVERS_INTC_WKPU_NXP_S32_PRIV_H_ */
