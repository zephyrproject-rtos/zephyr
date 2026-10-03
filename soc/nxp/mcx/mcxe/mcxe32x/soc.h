/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

#if defined(CONFIG_SOC_MCXE32B_CPU0)
#include <MCXE32B_cm7_core0.h>
#elif defined(CONFIG_SOC_MCXE32B_CPU1)
#include <MCXE32B_cm7_core1.h>
#endif

#include <soc_common.h>

#ifdef __cplusplus
extern "C" {
#endif

void enable_sram_extra_latency(bool en);

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
