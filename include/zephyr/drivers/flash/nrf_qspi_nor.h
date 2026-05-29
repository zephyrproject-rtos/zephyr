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
 * @brief Specifies whether XIP (execute in place) operation should be possible
 *
 * Normally, the driver deactivates the QSPI peripheral for periods when
 * no QSPI operation is performed. This is done to avoid increased current
 * consumption when the peripheral is idle. For the same reason, the base
 * clock on nRF53 Series SoCs (HFCLK192M) is configured for those periods
 * with the default /4 divider that cannot be used otherwise. However, when
 * XIP accesses are used, the driver must be prevented from doing both these
 * things as that would make XIP to fail. Hence, the application should use
 * this function to signal to the driver that XIP accesses are expected to
 * occur so that it keeps the QSPI peripheral operable. When XIP operation
 * is no longer needed, it should be disabled with this function.
 *
 * @param dev   flash device
 * @param enable if true, the driver enables XIP operation and suppresses
 *               idle actions that would make XIP to fail
 */
__syscall void nrf_qspi_nor_xip_enable(const struct device *dev, bool enable);

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/nrf_qspi_nor.h>

#endif /* __ZEPHYR_INCLUDE_DRIVERS_FLASH_NRF_QSPI_NOR_H__ */
