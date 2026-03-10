/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_MEMC_MEMC_NXP_S32_XSPI_H_
#define ZEPHYR_DRIVERS_MEMC_MEMC_NXP_S32_XSPI_H_

#include <zephyr/device.h>
#include <sys/types.h>

/**
 * @brief Get the XSPI peripheral hardware instance number.
 *
 * @param dev device pointer
 */
uint8_t memc_nxp_s32_xspi_get_instance(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_MEMC_MEMC_NXP_S32_XSPI_H_ */
