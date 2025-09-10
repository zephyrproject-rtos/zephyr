/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE

#include <fsl_port.h>
#include <fsl_common.h>

#define PORT_MUX_GPIO kPORT_MuxAlt0 /* GPIO setting for the Port Mux Register */

/* Handle variation to implement Wakeup Interrupt */
#define NXP_ENABLE_WAKEUP_SIGNAL(irqn) POWER_EnableWakeup(irqn)
#define NXP_DISABLE_WAKEUP_SIGNAL(irqn) POWER_DisableWakeup(irqn)
#define NXP_GET_WAKEUP_SIGNAL_STATUS(irqn) POWER_GetWakeupStatus(irqn)

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
