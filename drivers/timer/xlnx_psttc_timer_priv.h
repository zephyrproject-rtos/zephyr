/*
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (c) 2018 Xilinx, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_TIMER_XLNX_PSTTC_TIMER_PRIV_H_
#define ZEPHYR_DRIVERS_TIMER_XLNX_PSTTC_TIMER_PRIV_H_

/*
 * Refer to the "Zynq UltraScale+ Device Technical Reference Manual" document
 * from Xilinx for more information on this peripheral.
 */

/*
 * Triple-timer Counter (TTC) Register Offsets
 */

/* Clock Control Register */
#define XTTCPS_CLK_CNTRL_OFFSET     0x00000000U
/* Counter Control Register*/
#define XTTCPS_CNT_CNTRL_OFFSET     0x0000000CU
/* Current Counter Value */
#define XTTCPS_COUNT_VALUE_OFFSET   0x00000018U
/* Interval Count Value */
#define XTTCPS_INTERVAL_VAL_OFFSET  0x00000024U
/* Match 1 value */
#define XTTCPS_MATCH_0_OFFSET       0x00000030U
/* Match 2 value */
#define XTTCPS_MATCH_1_OFFSET       0x0000003CU
/* Match 3 value */
#define XTTCPS_MATCH_2_OFFSET       0x00000048U
/* Interrupt Status Register */
#define XTTCPS_ISR_OFFSET           0x00000054U
/* Interrupt Enable Register */
#define XTTCPS_IER_OFFSET           0x00000060U

/*
 * Clock Control Register Definitions
 */

/* Prescale enable */
#define XTTCPS_CLK_CNTRL_PS_EN_MASK     0x00000001U
/* Prescale value */
#define XTTCPS_CLK_CNTRL_PS_VAL_MASK    0x0000001EU
/* Prescale shift */
#define XTTCPS_CLK_CNTRL_PS_VAL_SHIFT   1U
/* Prescale disable */
#define XTTCPS_CLK_CNTRL_PS_DISABLE     16U
/* Clock source */
#define XTTCPS_CLK_CNTRL_SRC_MASK       0x00000020U
/* External Clock edge */
#define XTTCPS_CLK_CNTRL_EXT_EDGE_MASK  0x00000040U

/*
 * Counter Control Register Definitions
 */

/* Disable the counter */
#define XTTCPS_CNT_CNTRL_DIS_MASK       0x00000001U
/* Interval mode */
#define XTTCPS_CNT_CNTRL_INT_MASK       0x00000002U
/* Decrement mode */
#define XTTCPS_CNT_CNTRL_DECR_MASK      0x00000004U
/* Match mode */
#define XTTCPS_CNT_CNTRL_MATCH_MASK     0x00000008U
/* Reset counter */
#define XTTCPS_CNT_CNTRL_RST_MASK       0x00000010U
/* Enable waveform */
#define XTTCPS_CNT_CNTRL_EN_WAVE_MASK   0x00000020U
/* Waveform polarity */
#define XTTCPS_CNT_CNTRL_POL_WAVE_MASK  0x00000040U
/* Reset value */
#define XTTCPS_CNT_CNTRL_RESET_VALUE    0x00000021U

/*
 * Interrupt Register Definitions
 */

/* Interval Interrupt */
#define XTTCPS_IXR_INTERVAL_MASK    0x00000001U
/* Match 1 Interrupt */
#define XTTCPS_IXR_MATCH_0_MASK     0x00000002U
/* Match 2 Interrupt */
#define XTTCPS_IXR_MATCH_1_MASK     0x00000004U
/* Match 3 Interrupt */
#define XTTCPS_IXR_MATCH_2_MASK     0x00000008U
/* Counter Overflow */
#define XTTCPS_IXR_CNT_OVR_MASK     0x00000010U
/* All valid Interrupts */
#define XTTCPS_IXR_ALL_MASK         0x0000001FU

/*
 * Constants
 */

/* Maximum value of interval counter */
#define XTTC_MAX_INTERVAL_COUNT 0xFFFFFFFFU

#endif /* ZEPHYR_DRIVERS_TIMER_XLNX_PSTTC_TIMER_PRIV_H_ */
