/*
 * Copyright 2023-2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <fsl_port.h>
#include <soc_common.h>

#define PORT_MUX_GPIO kPORT_MuxAsGpio

#define nbu_handler RF_IMU0_IRQHandler

#if CONFIG_PM
void nxp_mcxw7x_power_init(void);
#else
#define nxp_mcxw7x_power_init(...) do { } while (0)
#endif

#endif /* _SOC__H_ */
