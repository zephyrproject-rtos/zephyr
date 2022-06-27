/*
 * Copyright (c) 2019 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>

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

#endif /* _SOC__H_ */
