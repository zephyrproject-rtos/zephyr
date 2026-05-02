/*
 * Copyright 2017, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>
#include <fsl_gpc.h>

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

/* Handle variation to implement Wakeup Interrupt */
#define NXP_ENABLE_WAKEUP_SIGNAL(irqn) GPC_EnableIRQ(GPC, irqn)
#define NXP_DISABLE_WAKEUP_SIGNAL(irqn) GPC_DisableIRQ(GPC, irqn)
#define NXP_GET_WAKEUP_SIGNAL_STATUS(irqn) GPC_GetStatusFlag(GPC, irqn)

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_I2S_MCUX_SAI
void imxrt_audio_codec_pll_init(uint32_t clock_name, uint32_t clk_src,
					uint32_t clk_pre_div, uint32_t clk_src_div);

#endif

#if CONFIG_MIPI_DSI
void imxrt_pre_init_display_interface(void);

void imxrt_post_init_display_interface(void);
#endif

void flexspi_clock_set_div(uint32_t value);
uint32_t flexspi_clock_get_freq(void);

#ifdef CONFIG_MEMC
uint32_t flexspi_clock_set_freq(uint32_t clock_name, uint32_t rate);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
