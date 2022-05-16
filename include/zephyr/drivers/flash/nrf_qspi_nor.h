/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_NRF_QSPI_NOR_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_NRF_QSPI_NOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Specifies whether the QSPI base clock divider should be kept set
 *        when the driver is idle
 *
 * On nRF53 Series SoCs, it is necessary to change the default base clock
 * divider to achieve the highest possible SCK frequencies. This divider
 * should be changed only for periods when it is actually needed, as such
 * configuration significantly increases power consumption, so the driver
 * normally does this only when it performs an operation on the QSPI bus.
 * But when XIP accesses to the flash chip are also used, and the driver
 * is not aware of those, it may be necessary for the divider to be kept
 * changed also when the driver is idle. This function allows forcing this.
 *
 * @param dev   flash device
 * @param force if true, forces the base clock divider to be kept set even
 *              when the driver is idle
 */
__syscall void nrf_qspi_nor_base_clock_div_force(const struct device *dev,
						 bool force);

#ifdef __cplusplus
}
#endif

#include <syscalls/nrf_qspi_nor.h>

#endif /* __ZEPHYR_INCLUDE_DRIVERS_FLASH_NRF_QSPI_NOR_H__ */
