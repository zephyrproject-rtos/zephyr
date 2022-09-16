/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE

#include <fsl_common.h>

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_I2S_MCUX_SAI
void imxrt_audio_codec_pll_init(uint32_t clock_name, uint32_t clk_src,
					uint32_t clk_pre_div, uint32_t clk_src_div);

#endif

#if (DT_DEP_ORD(DT_NODELABEL(ocram)) != DT_DEP_ORD(DT_CHOSEN(zephyr_sram))) && \
	CONFIG_OCRAM_NOCACHE
/* OCRAM addresses will be defined by linker */
extern char __ocram_start;
extern char __ocram_bss_start;
extern char __ocram_bss_end;
extern char __ocram_noinit_start;
extern char __ocram_noinit_end;
extern char __ocram_data_start;
extern char __ocram_data_end;
extern char __ocram_end;
extern char __ocram_data_load_start;
#endif
#if CONFIG_MIPI_DSI
void imxrt_pre_init_display_interface(void);

void imxrt_post_init_display_interface(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
