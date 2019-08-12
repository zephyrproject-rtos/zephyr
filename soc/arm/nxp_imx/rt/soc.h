/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

/* ARM CMSIS definitions must be included before kernel_includes.h.
 * Therefore, it is essential to include kernel_includes.h after including
 * core SOC-specific headers.
 */
#include <kernel_includes.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_DISK_ACCESS_USDHC1) ||	\
	defined(CONFIG_DISK_ACCESS_USDHC2)

typedef void (*usdhc_pin_cfg_cb)(u16_t nusdhc, bool init,
	u32_t speed, u32_t strength);

void imxrt_usdhc_pinmux(u16_t nusdhc,
	bool init, u32_t speed, u32_t strength);

void imxrt_usdhc_pinmux_cb_register(usdhc_pin_cfg_cb cb);

#endif

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
