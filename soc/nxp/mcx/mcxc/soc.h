/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>

#if CONFIG_PINCTRL

#include <fsl_port.h>

#if CONFIG_UART
#define UART0_CLK_SRC kCLOCK_CoreSysClk
#endif

#define PORT_MUX_GPIO kPORT_MuxAsGpio /* GPIO setting for the Port Mux Register */

#endif

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
