/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ARM_FOCALTECH_FT9001_SOC_H_
#define _SOC_ARM_FOCALTECH_FT9001_SOC_H_

/* CMSIS required definitions */
#define __FPU_PRESENT CONFIG_CPU_HAS_FPU
#define __MPU_PRESENT CONFIG_CPU_HAS_ARM_MPU

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>
#include <cmsis_core_m_defaults.h>

#if !defined(_ASMLANGUAGE) && !defined(__ASSEMBLER__)
#include <stdint.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

#ifdef __cplusplus
extern "C" {
#endif

__ramfunc void xip_clock_switch(uint32_t clk_div);
__ramfunc void xip_return_to_boot(void);

#ifdef __cplusplus
}
#endif
#endif /* !_ASMLANGUAGE && !__ASSEMBLER__ */

#endif /* _SOC_ARM_FOCALTECH_FT9001_SOC_H_ */
