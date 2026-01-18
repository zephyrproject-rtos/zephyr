/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz GmbH
 * Copyright (c) 2025 Bang & Olufsen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MSPM0_SOC_H
#define _MSPM0_SOC_H

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/m0p/dl_core.h>

#if defined(CONFIG_SOC_TI_MSPM0_INT_GROUP_0) ||		\
	defined(CONFIG_SOC_TI_MSPM0_INT_GROUP_1)
/**
 * @brief Register an isr_handler into an interrupt group
 *
 * @param group The targeted interrupt group (i.e. the NVIC interrupt).
 * @param int_idx The interrupt group index (aka IIDX).
 * @param isr_handler  Pointer to the ISR function for the device.
 * @param dev Pointer to the device that will service the interrupt.
 *
 * @return 0 on success, a negative errno otherwise: -ENOTSUP if the given group
 *         was not enabled, -EINVAL if one of the parameter is invalid and
 *         -EALREADY if the given int_idx has already been registered.
 */
int mspm0_register_int_to_group(int group, uint8_t int_idx,
				void (*isr_handler)(const struct device *dev),
				const struct device *dev);
#else

#define mspm0_register_int_to_group(...) (-ENOTSUP)

#endif /* CONFIG_SOC_TI_MSPM0_INT_GROUP_0 || CONFIG_SOC_TI_MSPM0_INT_GROUP_1 */

#endif /* _MSPM0_SOC_H */
