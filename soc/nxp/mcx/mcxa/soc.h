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
#include <fsl_spc.h>

#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
#include <fsl_cmc.h>
#include <fsl_wuu.h>
#endif

#define PORT_MUX_GPIO kPORT_MuxAlt0 /* GPIO setting for the Port Mux Register */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
