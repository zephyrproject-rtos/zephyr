/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_NRF_RETAINED_MEM_H
#define ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_NRF_RETAINED_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/retained_mem.h>

#if defined(CONFIG_RETAINED_MEM_NRF_RAM_CTRL) || defined(__DOXYGEN__)
/** @brief Apply memory retention settings.
 *
 * Memory retention settings to apply are derived from devicetree configuration.
 *
 * @retval 0 if the retention settings were applied successfully.
 * @retval -ENOTSUP if retention configuration is not present in devicetree.
 */
int z_nrf_retained_mem_retention_apply(void);
#else
static inline int z_nrf_retained_mem_retention_apply(void)
{
	return -ENOTSUP;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RETAINED_MEM_NRF_RETAINED_MEM_H */
