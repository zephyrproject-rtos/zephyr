/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AT32_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AT32_H_

#include <zephyr/device.h>

#define CRM_CFG_OFFSET	  0x08U
#define CRM_AHB1EN_OFFSET 0x30U
#define CRM_AHB2EN_OFFSET 0x34U
#define CRM_AHB3EN_OFFSET 0x38U
#define CRM_APB1EN_OFFSET 0x40U
#define CRM_APB2EN_OFFSET 0x44U

#define CRM_CFG_AHBDIV_POS  4U
#define CRM_CFG_AHBDIV_MSK  (BIT_MASK(4) << CRM_CFG_AHBDIV_POS)
#define CRM_CFG_APB1DIV_POS 10U
#define CRM_CFG_APB1DIV_MSK (BIT_MASK(3) << CRM_CFG_APB1DIV_POS)
#define CRM_CFG_APB2DIV_POS 13U
#define CRM_CFG_APB2DIV_MSK (BIT_MASK(3) << CRM_CFG_APB2DIV_POS)

/**
 * @brief Obtain a reference to the AT32 clock controller.
 *
 * There is a single clock controller in the AT32: ctrl. The device can be
 * used without checking for it to be ready since it has no initialization
 * code subject to failures.
 */
#define AT32_CLOCK_CONTROLLER DEVICE_DT_GET(DT_NODELABEL(cctl))

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AT32_H_ */
