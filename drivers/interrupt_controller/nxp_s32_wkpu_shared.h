/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_NXP_S32_WKPU_SHARED_H_
#define ZEPHYR_DRIVERS_NXP_S32_WKPU_SHARED_H_

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/intc_wkpu_nxp_s32.h>

/*
 * Internal cross-driver contract of the WKPU hardware owner. GPIO, WUC and
 * startup code use physical sources; WUC ID encoding belongs to the WUC driver.
 * This header is not part of the public driver API.
 */
int wkpu_nxp_s32_configure_interrupt(const struct device *dev, uint8_t irq, uint8_t pin,
				     enum wkpu_nxp_s32_trigger trigger,
				     wkpu_nxp_s32_callback_t cb, void *arg);

#ifdef CONFIG_WUC_NXP_S32_WKPU
int wkpu_nxp_s32_set_wakeup(const struct device *dev, uint32_t source,
			    enum wkpu_nxp_s32_trigger trigger, bool enable);
int wkpu_nxp_s32_get_event(const struct device *dev, uint32_t source);
int wkpu_nxp_s32_clear_event(const struct device *dev, uint32_t source);

/*
 * Internal early-boot handoff: call after BSS initialization, before WKPU
 * device initialization and before clearing or resetting the captured status.
 * The caller determines whether this boot followed a wakeup. This function
 * only saves the supplied bits; it does not acknowledge hardware status.
 */
void wkpu_nxp_s32_capture_events(const struct device *dev, uint64_t status);
#endif

#endif /* ZEPHYR_DRIVERS_NXP_S32_WKPU_SHARED_H_ */
