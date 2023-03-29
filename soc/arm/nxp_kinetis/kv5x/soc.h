/*
 * Copyright (c) 2019 SEAL AG
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>
#include <fsl_port.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <fsl_common.h>


#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

/* address bases */
#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

#define PORT_MUX_GPIO kPORT_MuxAsGpio /* GPIO setting for the Port Mux Register */

#endif /* _SOC__H_ */
