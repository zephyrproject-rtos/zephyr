/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ARM_FOCALTECH_FT90_SOC_H_
#define _SOC_ARM_FOCALTECH_FT90_SOC_H_

#ifndef _ASMLANGUAGE

#include <ft9001.h>

void __attribute__((section(".ramfunc"))) xip_clock_switch(uint32_t clk_div);
void __attribute__((section(".ramfunc"))) xip_reback_boot(void);

#endif /* _ASMLANGUAGE */

#endif /* _SOC_ARM_FOCALTECH_FT90_SOC_H_ */
