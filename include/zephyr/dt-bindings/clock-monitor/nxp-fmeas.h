/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief INPUTMUX connection cookies for the NXP BASIC FREQME (fmeas) inputs.
 *
 * These mirror the kINPUTMUX_<source>ToFreqmeas enumerators in each SoC's
 * fsl_inputmux_connections.h.
 *
 * A cookie encodes the FMEASURE destination mux id (0x700) shifted by the
 * INPUTMUX PMUX_SHIFT (20) OR'ed with the per-source selector value. The same
 * source cookie is valid for either FMEASURE channel; the devicetree
 * "channel" cell selects the role: channel 0 = reference (FMEASURE_CH_SEL[0]),
 * channel 1 = target (FMEASURE_CH_SEL[1]).
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_NXP_FMEAS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_NXP_FMEAS_H_

/** FMEASURE destination mux id in the INPUTMUX PMUX encoding. */
#define NXP_FMEAS_PMUX_ID    0x700U
/** Shift applied to @ref NXP_FMEAS_PMUX_ID within an INPUTMUX cookie. */
#define NXP_FMEAS_PMUX_SHIFT 20U

/** Build an INPUTMUX FMEASURE cookie from a per-source selector @p sel. */
#define NXP_FMEAS_CONNECTION(sel) \
	(((sel) & ((1U << NXP_FMEAS_PMUX_SHIFT) - 1U)) | \
	 (NXP_FMEAS_PMUX_ID << NXP_FMEAS_PMUX_SHIFT))

/* RT6xx (MIMXRT685S) - see RT600 fsl_inputmux_connections.h */
/** RT6xx FREQME source: crystal oscillator input (XTALIN). */
#define NXP_FMEAS_RT6XX_XTALIN          NXP_FMEAS_CONNECTION(0)
/** RT6xx FREQME source: slow internal FRO (SFRO). */
#define NXP_FMEAS_RT6XX_SFRO            NXP_FMEAS_CONNECTION(1)
/** RT6xx FREQME source: fast internal FRO (FFRO). */
#define NXP_FMEAS_RT6XX_FFRO            NXP_FMEAS_CONNECTION(2)
/** RT6xx FREQME source: low-power oscillator (LPOSC). */
#define NXP_FMEAS_RT6XX_LPOSC          NXP_FMEAS_CONNECTION(3)
/** RT6xx FREQME source: 32 kHz RTC oscillator. */
#define NXP_FMEAS_RT6XX_RTC_32KHZ      NXP_FMEAS_CONNECTION(4)
/** RT6xx FREQME source: main system clock. */
#define NXP_FMEAS_RT6XX_MAIN_SYS_CLK   NXP_FMEAS_CONNECTION(5)
/** RT6xx FREQME source: external clock routed via GPIO (FREQME_GPIO_CLK). */
#define NXP_FMEAS_RT6XX_FREQME_GPIO_CLK NXP_FMEAS_CONNECTION(6)

/* RT5xx (MIMXRT595S) - see RT500 fsl_inputmux_connections.h */
/** RT5xx FREQME source: crystal oscillator input (XTALIN). */
#define NXP_FMEAS_RT5XX_XTALIN          NXP_FMEAS_CONNECTION(0)
/** RT5xx FREQME source: 12 MHz FRO. */
#define NXP_FMEAS_RT5XX_FRO_12M         NXP_FMEAS_CONNECTION(1)
/** RT5xx FREQME source: 192 MHz FRO. */
#define NXP_FMEAS_RT5XX_FRO_192M        NXP_FMEAS_CONNECTION(2)
/** RT5xx FREQME source: low-power oscillator (LPOSC). */
#define NXP_FMEAS_RT5XX_LPOSC          NXP_FMEAS_CONNECTION(3)
/** RT5xx FREQME source: 32 kHz oscillator. */
#define NXP_FMEAS_RT5XX_OSC_32KHZ      NXP_FMEAS_CONNECTION(4)
/** RT5xx FREQME source: main system clock. */
#define NXP_FMEAS_RT5XX_MAIN_SYS_CLK   NXP_FMEAS_CONNECTION(5)
/** RT5xx FREQME source: external clock routed via GPIO (FREQME_GPIO_CLK). */
#define NXP_FMEAS_RT5XX_FREQME_GPIO_CLK NXP_FMEAS_CONNECTION(6)
/** RT5xx FREQME source: CLKOUT. */
#define NXP_FMEAS_RT5XX_CLOCK_OUT      NXP_FMEAS_CONNECTION(11)

/* RW6xx (RW612) - see RW612 fsl_inputmux_connections.h */
/** RW6xx FREQME source: system oscillator (SYSOSC). */
#define NXP_FMEAS_RW6XX_SYSOSC          NXP_FMEAS_CONNECTION(0)
/** RW6xx FREQME source: slow internal FRO (SFRO). */
#define NXP_FMEAS_RW6XX_SFRO            NXP_FMEAS_CONNECTION(1)
/** RW6xx FREQME source: fast internal FRO (FFRO). */
#define NXP_FMEAS_RW6XX_FFRO            NXP_FMEAS_CONNECTION(2)
/** RW6xx FREQME source: low-power oscillator (LPOSC). */
#define NXP_FMEAS_RW6XX_LPOSC          NXP_FMEAS_CONNECTION(3)
/** RW6xx FREQME source: 32 kHz crystal. */
#define NXP_FMEAS_RW6XX_XTAL_32K       NXP_FMEAS_CONNECTION(4)
/** RW6xx FREQME source: CPU0 free-running HCLK (main system clock). */
#define NXP_FMEAS_RW6XX_C0_FR_HCLK     NXP_FMEAS_CONNECTION(5)
/** RW6xx FREQME source: external clock routed via GPIO (FREQME_GPIO_CLK). */
#define NXP_FMEAS_RW6XX_FREQME_GPIO_CLK NXP_FMEAS_CONNECTION(6)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MONITOR_NXP_FMEAS_H_ */
