/*
 * Copyright 2024, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE

#include <fsl_common.h>
#include <fsl_gpc.h>

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

/* Handle variation to implement Wakeup Interrupt */
#if defined(CONFIG_SOC_MIMXRT1189_CM33)
#define NXP_ENABLE_WAKEUP_SIGNAL(irqn) GPC_CM_EnableIrqWakeup(kGPC_CPU0, irqn, true)
#define NXP_DISABLE_WAKEUP_SIGNAL(irqn) GPC_CM_EnableIrqWakeup(kGPC_CPU0, irqn, false)
#define NXP_GET_WAKEUP_SIGNAL_STATUS(irqn) GPC_CM_GetIrqWakeupStatus(kGPC_CPU0, irqn)
#endif

#if defined(CONFIG_SOC_MIMXRT1189_CM7)
#define NXP_ENABLE_WAKEUP_SIGNAL(irqn) GPC_CM_EnableIrqWakeup(kGPC_CPU1, irqn, true)
#define NXP_DISABLE_WAKEUP_SIGNAL(irqn) GPC_CM_EnableIrqWakeup(kGPC_CPU1, irqn, false)
#define NXP_GET_WAKEUP_SIGNAL_STATUS(irqn) GPC_CM_GetIrqWakeupStatus(kGPC_CPU1, irqn)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MEMC_MCUX_FLEXSPI
uint32_t flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate);
#endif

#ifdef CONFIG_I2S_MCUX_SAI
void imxrt_audio_codec_pll_init(uint32_t clock_name, uint32_t clk_src,
					uint32_t clk_pre_div, uint32_t clk_src_div);

#endif /* CONFIG_I2S_MCUX_SAI */

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
