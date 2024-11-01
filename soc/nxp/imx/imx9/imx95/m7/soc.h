/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_NXP_IMX_IMX95_M7_SOC_H_
#define _SOC_NXP_IMX_IMX95_M7_SOC_H_

#include <fsl_device_registers.h>

/* IRQSTEER used for NETC MSGINTR2 interrupt */
#if defined(CONFIG_ETH_NXP_IMX_NETC) && (DT_CHILD_NUM_STATUS_OKAY(DT_NODELABEL(netc)) != 0)
#define NETC_MSGINTR_IRQ IRQSTEER_0_IRQn
#endif

#endif /* _SOC_NXP_IMX_IMX95_M7_SOC_H_ */
