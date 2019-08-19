/*
 * Copyright (c) 2019 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

/* ARM CMSIS definitions must be included before kernel_includes.h.
 * Therefore, it is essential to include kernel_includes.h after including
 * core SOC-specific headers.
 */
#include <kernel_includes.h>

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

/* address bases */
#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

#endif /* _SOC__H_ */
