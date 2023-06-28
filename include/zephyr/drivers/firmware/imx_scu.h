/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_IMX_SCU_H_
#define ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_IMX_SCU_H_

#include <main/ipc.h>
#include <zephyr/device.h>

sc_ipc_t imx_scu_get_ipc_handle(const struct device *dev);


#endif /* ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_IMX_SCU_H_ */
