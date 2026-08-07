/*
 * Copyright 2023-2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <fsl_port.h>
#include <soc_common.h>

#define PORT_MUX_GPIO kPORT_MuxAsGpio

#if CONFIG_SOC_MCXW70AC
#define nbu_handler MU0_IRQHandler
#else
#define nbu_handler RF_IMU0_IRQHandler
#endif

#if CONFIG_PM
void nxp_mcxw7x_power_init(void);
#else
#define nxp_mcxw7x_power_init(...) do { } while (0)
#endif

/* Apply the active-mode DCDC output voltage configured on the SPC device tree
 * node. Compiled unconditionally (independent of CONFIG_PM); a no-op when no
 * voltage is configured.
 */
void nxp_mcxw7x_dcdc_init(void);

#endif /* _SOC__H_ */
