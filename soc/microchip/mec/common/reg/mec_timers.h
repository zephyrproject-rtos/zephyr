/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_TIMERS_H
#define _MEC_TIMERS_H

#include <stdint.h>
#include <stddef.h>

/* Basic timers */

/* Offset between instances of the Basic Timer blocks */
#define MCHP_BTMR_INSTANCE_POS		5ul
#define MCHP_BTMR_INSTANCE_OFS		0x20u

/* Base frequency of all basic timers is AHB clock */
#define MCHP_BTMR_BASE_FREQ		48000000u
#define MCHP_BTMR_MIN_FREQ		(MCHP_BTMR_BASE_FREQ / 0x10000u)

/*
 * Basic Timer Count Register (Offset +00h)
 * 32-bit R/W
 * 16-bit Basic timers: bits[15:0]=R/W, bits[31:15]=RO=0
 */
#define MCHP_BTMR_CNT_OFS		0x00u

/*
 * Basic Timer Preload Register (Offset +04h)
 * 32-bit R/W
 * 16-bit Basic timers: bits[15:0]=R/W, bits[31:15]=RO=0
 */
#define MCHP_BTMR_PRELOAD_OFS		0x04u

/* Basic Timer Status Register (Offset +08h)  R/W1C */
#define MCHP_BTMR_STS_OFS		0x08u
#define MCHP_BTMR_STS_MASK		0x01u
#define MCHP_BTMR_STS_ACTIVE_POS	0
#define MCHP_BTMR_STS_ACTIVE		0x01u

/* Basic Timer Interrupt Enable Register (Offset +0Ch) */
#define MCHP_BTMR_INTEN_OFS		0x0cu
#define MCHP_BTMR_INTEN_MASK		0x01u
#define MCHP_BTMR_INTEN_POS		0
#define MCHP_BTMR_INTEN			0x01u
#define MCHP_BTMR_INTDIS		0u

/* Basic Timer Control Register (Offset +10h) */
#define MCHP_BTMR_CTRL_OFS		0x10u
#define MCHP_BTMR_CTRL_MASK		0xffff00fdu

#define MCHP_BTMR_CTRL_PRESCALE_POS	16u
#define MCHP_BTMR_CTRL_PRESCALE_MASK0	0xffffu
#define MCHP_BTMR_CTRL_PRESCALE_MASK	0xffff0000u

#define MCHP_BTMR_CTRL_HALT		0x80u
#define MCHP_BTMR_CTRL_RELOAD		0x40u
#define MCHP_BTMR_CTRL_START		0x20u
#define MCHP_BTMR_CTRL_SOFT_RESET	0x10u
#define MCHP_BTMR_CTRL_AUTO_RESTART	0x08u
#define MCHP_BTMR_CTRL_COUNT_UP		0x04u
#define MCHP_BTMR_CTRL_ENABLE		0x01u
/* */
#define MCHP_BTMR_CTRL_HALT_POS		7u
#define MCHP_BTMR_CTRL_RELOAD_POS	6u
#define MCHP_BTMR_CTRL_START_POS	5u
#define MCHP_BTMR_CTRL_SRESET_POS	4u
#define MCHP_BTMR_CTRL_AUTO_RESTART_POS 3u
#define MCHP_BTMR_CTRL_COUNT_DIR_POS	2u
#define MCHP_BTMR_CTRL_ENABLE_POS	0u

/** @brief Basic Timer(32 and 16 bit) registers. Total size = 20(0x14) bytes */
struct btmr_regs {
	volatile uint32_t CNT;
	volatile uint32_t PRLD;
	volatile uint8_t STS;
	uint8_t RSVDC[3];
	volatile uint8_t IEN;
	uint8_t RSVDD[3];
	volatile uint32_t CTRL;
};

/*
 * Hibernation Timer
 * Set count resolution in bit[0]
 * 0 = 30.5 us (32786 Hz)
 * 1 = 125 ms (8 Hz)
 */
#define MCHP_HTMR_CTRL_REG_MASK		0x01u
#define MCHP_HTMR_CTRL_RESOL_POS	0u
#define MCHP_HTMR_CTRL_RESOL_MASK	BIT(MCHP_HTMR_CTRL_EN_POS)
#define MCHP_HTMR_CTRL_RESOL_30US	0u
#define MCHP_HTMR_CTRL_RESOL_125MS	BIT(MCHP_HTMR_CTRL_EN_POS)

/*
 * Hibernation timer is started and stopped by writing a value
 * to the CNT (count) register.
 * Writing a non-zero value resets and start the counter counting down.
 * Writing 0 stops the timer.
 */
#define MCHP_HTMR_CNT_STOP_VALUE	0

/** @brief Hibernation Timer (HTMR) */
struct htmr_regs {
	volatile uint16_t PRLD;
	uint16_t RSVD1[1];
	volatile uint16_t CTRL;
	uint16_t RSVD2[1];
	volatile uint16_t CNT;
	uint16_t RSVD3[1];
};

/* Capture/Compare Timer */

/* Control register at offset 0x00. Must use 32-bit access */
#define MCHP_CCT_CTRL_ACTIVATE		BIT(0)
#define MCHP_CCT_CTRL_FRUN_EN		BIT(1)
#define MCHP_CCT_CTRL_FRUN_RESET	BIT(2)	/* self clearing bit */
#define MCHP_CCT_CTRL_TCLK_MASK0	0x07u
#define MCHP_CCT_CTRL_TCLK_MASK		SHLU32(MCHP_CCT_CTRL_TCLK_MASK0, 4)
#define MCHP_CCT_CTRL_TCLK_DIV_1	0u
#define MCHP_CCT_CTRL_TCLK_DIV_2	SHLU32(1, 4)
#define MCHP_CCT_CTRL_TCLK_DIV_4	SHLU32(2, 4)
#define MCHP_CCT_CTRL_TCLK_DIV_8	SHLU32(3, 4)
#define MCHP_CCT_CTRL_TCLK_DIV_16	SHLU32(4, 4)
#define MCHP_CCT_CTRL_TCLK_DIV_32	SHLU32(5, 4)
#define MCHP_CCT_CTRL_TCLK_DIV_64	SHLU32(6, 4)
#define MCHP_CCT_CTRL_TCLK_DIV_128	SHLU32(7, 4)
#define MCHP_CCT_CTRL_COMP0_EN		BIT(8)
#define MCHP_CCT_CTRL_COMP1_EN		BIT(9)
#define MCHP_CCT_CTRL_COMP1_SET		BIT(16) /* R/WS */
#define MCHP_CCT_CTRL_COMP0_SET		BIT(17) /* R/WS */
#define MCHP_CCT_CTRL_COMP1_CLR		BIT(24) /* R/W1C */
#define MCHP_CCT_CTRL_COMP0_CLR		BIT(25) /* R/W1C */

/** @brief Capture/Compare Timer */
struct cct_regs {
	volatile uint32_t CTRL;
	volatile uint32_t CAP0_CTRL;
	volatile uint32_t CAP1_CTRL;
	volatile uint32_t FREE_RUN;
	volatile uint32_t CAP0;
	volatile uint32_t CAP1;
	volatile uint32_t CAP2;
	volatile uint32_t CAP3;
	volatile uint32_t CAP4;
	volatile uint32_t CAP5;
	volatile uint32_t COMP0;
	volatile uint32_t COMP1;
};

/* RTOS Timer */
#define MCHP_RTMR_FREQ_HZ		32768u

#define MCHP_RTMR_CTRL_MASK		0x1fu
#define MCHP_RTMR_CTRL_BLK_EN_POS	0
#define MCHP_RTMR_CTRL_BLK_EN_MASK	BIT(MCHP_RTMR_CTRL_BLK_EN_POS)
#define MCHP_RTMR_CTRL_BLK_EN		BIT(MCHP_RTMR_CTRL_BLK_EN_POS)

#define MCHP_RTMR_CTRL_AUTO_RELOAD_POS	1u
#define MCHP_RTMR_CTRL_AUTO_RELOAD_MASK BIT(MCHP_RTMR_CTRL_AUTO_RELOAD_POS)
#define MCHP_RTMR_CTRL_AUTO_RELOAD	BIT(MCHP_RTMR_CTRL_AUTO_RELOAD_POS)

#define MCHP_RTMR_CTRL_START_POS	2u
#define MCHP_RTMR_CTRL_START_MASK	BIT(MCHP_RTMR_CTRL_START_POS)
#define MCHP_RTMR_CTRL_START		BIT(MCHP_RTMR_CTRL_START_POS)

#define MCHP_RTMR_CTRL_HW_HALT_EN_POS	3u
#define MCHP_RTMR_CTRL_HW_HALT_EN_MASK	BIT(MCHP_RTMR_CTRL_HW_HALT_EN_POS)
#define MCHP_RTMR_CTRL_HW_HALT_EN	BIT(MCHP_RTMR_CTRL_HW_HALT_EN_POS)

#define MCHP_RTMR_CTRL_FW_HALT_EN_POS	4u
#define MCHP_RTMR_CTRL_FW_HALT_EN_MASK	BIT(MCHP_RTMR_CTRL_FW_HALT_EN_POS)
#define MCHP_RTMR_CTRL_FW_HALT_EN	BIT(MCHP_RTMR_CTRL_FW_HALT_EN_POS)

/** @brief RTOS Timer (RTMR) */
struct rtmr_regs {
	volatile uint32_t CNT;
	volatile uint32_t PRLD;
	volatile uint32_t CTRL;
	volatile uint32_t SOFTIRQ;
};

/* Week Timer */
#define MCHP_WKTMR_CTRL_MASK		0x41u
#define MCHP_WKTMR_CTRL_WT_EN_POS	0
#define MCHP_WKTMR_CTRL_WT_EN_MASK	BIT(MCHP_WKTMR_CTRL_WT_EN_POS)
#define MCHP_WKTMR_CTRL_WT_EN		BIT(MCHP_WKTMR_CTRL_WT_EN_POS)
#define MCHP_WKTMR_CTRL_PWRUP_EV_EN_POS 6u
#define MCHP_WKTMR_CTRL_PWRUP_EV_EN_MASK BIT(MCHP_WKTMR_CTRL_PWRUP_EV_EN_POS)
#define MCHP_WKTMR_CTRL_PWRUP_EV_EN	BIT(MCHP_WKTMR_CTRL_PWRUP_EV_EN_POS)

#define MCHP_WKTMR_ALARM_CNT_MASK	0x0fffffffu
#define MCHP_WKTMR_TMR_CMP_MASK		0x0fffffffu
#define MCHP_WKTMR_CLK_DIV_MASK		0x7fffu

/* Sub-second interrupt select at +0x10 */
#define MCHP_WKTMR_SS_MASK		0x0fu
#define MCHP_WKTMR_SS_RATE_DIS		0x00u
#define MCHP_WKTMR_SS_RATE_2HZ		0x01u
#define MCHP_WKTMR_SS_RATE_4HZ		0x02u
#define MCHP_WKTMR_SS_RATE_8HZ		0x03u
#define MCHP_WKTMR_SS_RATE_16HZ		0x04u
#define MCHP_WKTMR_SS_RATE_32HZ		0x05u
#define MCHP_WKTMR_SS_RATE_64HZ		0x06u
#define MCHP_WKTMR_SS_RATE_128HZ	0x07u
#define MCHP_WKTMR_SS_RATE_256HZ	0x08u
#define MCHP_WKTMR_SS_RATE_512HZ	0x09u
#define MCHP_WKTMR_SS_RATE_1024HZ	0x0au
#define MCHP_WKTMR_SS_RATE_2048HZ	0x0bu
#define MCHP_WKTMR_SS_RATE_4096HZ	0x0cu
#define MCHP_WKTMR_SS_RATE_8192HZ	0x0du
#define MCHP_WKTMR_SS_RATE_16384HZ	0x0eu
#define MCHP_WKTMR_SS_RATE_32768HZ	0x0fu

/* Sub-week control at +0x14 */
#define MCHP_WKTMR_SWKC_MASK			0x3f3u
#define MCHP_WKTMR_SWKC_PWRUP_EV_STS_POS	0ul
#define MCHP_WKTMR_SWKC_PWRUP_EV_STS_MASK	\
	BIT(MCHP_WKTMR_SWKC_PWRUP_EV_STS_POS)
#define MCHP_WKTMR_SWKC_PWRUP_EV_STS		\
	BIT(MCHP_WKTMR_SWKC_PWRUP_EV_STS_POS)
#define MCHP_WKTMR_SWKC_SYSPWR_PRES_STS_POS	4
#define MCHP_WKTMR_SWKC_SYSPWR_PRES_STS_MASK	\
	BIT(MCHP_WKTMR_SWKC_SYSPWR_PRES_STS_POS)
#define MCHP_WKTMR_SWKC_SYSPWR_PRES_STS		\
	BIT(MCHP_WKTMR_SWKC_SYSPWR_PRES_STS_POS)
#define MCHP_WKTMR_SWKC_SYSPWR_PRES_EN_POS	5
#define MCHP_WKTMR_SWKC_SYSPWR_PRES_EN_MASK	\
	BIT(MCHP_WKTMR_SWKC_SYSPWR_PRES_EN_POS)
#define MCHP_WKTMR_SWKC_SYSPWR_PRES_EN		\
	BIT(MCHP_WKTMR_SWKC_SYSPWR_PRES_EN_POS)
#define MCHP_WKTMR_SWKC_AUTO_RELOAD_POS		6
#define MCHP_WKTMR_SWKC_AUTO_RELOAD_MASK	\
	BIT(MCHP_WKTMR_SWKC_AUTO_RELOAD_POS)
#define MCHP_WKTMR_SWKC_AUTO_RELOAD		\
	BIT(MCHP_WKTMR_SWKC_AUTO_RELOAD_POS)

/* Sub-week alarm counter at +0x18 */
#define MCHP_WKTMR_SWAC_MASK			0x1ff01ffu
#define MCHP_WKTMR_SWAC_LOAD_POS		0
#define MCHP_WKTMR_SWAC_CNT_RO_POS		16
#define MCHP_WKTMR_SWAC_LOAD_MASK		GENMASK(8, 0)
#define MCHP_WKTMR_SWAC_CNT_RO_MASK		GENMASK(24, 16)

/* Week timer BGPO Data at +0x1c */
#define MCHP_WKTMR_BGPO_DATA_MASK		GENMASK(5, 0)

/* Week timer BGPO Power at +0x20 */
#define MCHP_WKTMR_BGPO_PWR_MASK		GENMASK(5, 0)
#define MCHP_WKTMR_BGPO_0_PWR_RO		BIT(0)

/* Week timer BGPO Reset at +0x24 */
#define MCHP_WKTMR_BGPO_RST_MASK		GENMASK(5, 0)
#define MCHP_WKTMR_BGPO_RST_VBAT(n)		BIT(n)

/** @brief Week Timer (WKTMR) */
struct wktmr_regs {
	volatile uint32_t CTRL;
	volatile uint32_t ALARM_CNT;
	volatile uint32_t TMR_CMP;
	volatile uint32_t CLKDIV;
	volatile uint32_t SUBSEC_ISEL;
	volatile uint32_t SUBWK_CTRL;
	volatile uint32_t SUBWK_ALARM_CNT;
	volatile uint32_t BGPO_DATA;
	volatile uint32_t BGPO_PWR;
	volatile uint32_t BGPO_RST;
};

#endif	/* #ifndef _MEC_TIMERS_H */
