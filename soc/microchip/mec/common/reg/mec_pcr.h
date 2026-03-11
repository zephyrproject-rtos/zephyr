/*
 * Copyright (c) 2025 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_PCR_H
#define _MEC_PCR_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

/* sleep control */
#define XEC_PCR_SCR_OFS            0
#define XEC_PCR_SCR_SLP_DEEP_POS   0
#define XEC_PCR_SCR_SLP_ALL_POS    3
#define XEC_PCR_SCR_SLP_PLL_NL_POS 8

#define XEC_PCR_SCR_SLP_LIGHT BIT(XEC_PCR_SCR_SLP_ALL_POS)
#define XEC_PCR_SCR_SLP_DEEP  (BIT(XEC_PCR_SCR_SLP_ALL_POS) | BIT(XEC_PCR_SCR_SLP_DEEP_POS))

/* Processor clock control */
#define XEC_PCR_PCLK_CR_OFS        4u
#define XEC_PCR_PCLK_CR_DIV_POS    0
#define XEC_PCR_PCLK_CR_DIV_MSK    GENMASK(7, 0)
#define XEC_PCR_PCLK_CR_DIV_OFF    0
#define XEC_PCR_PCLK_CR_DIV_1      1u
#define XEC_PCR_PCLK_CR_DIV_2      2u
#define XEC_PCR_PCLK_CR_DIV_4      4u
#define XEC_PCR_PCLK_CR_DIV_16     16u
#define XEC_PCR_PCLK_CR_DIV_48     48u
#define XEC_PCR_PCLK_CR_DIV_SET(d) FIELD_PREP(XEC_PCR_PCLK_CR_DIV_MSK, (d))
#define XEC_PCR_PCLK_CR_DIV_GET(r) FIELD_GET(XEC_PCR_PCLK_CR_DIV_MSK, (r))

/* Slow clock control */
#define XEC_PCR_SCLK_CR_OFS        8u
#define XEC_PCR_SCLK_CR_DIV_POS    0
#define XEC_PCR_SCLK_CR_DIV_MSK    GENMASK(9, 0)
#define XEC_PCR_SCLK_CR_DIV_DFLT   0x1e0u
#define XEC_PCR_SCLK_CR_DIV_SET(d) FIELD_PREP(XEC_PCR_SCLK_CR_DIV_MSK, (d))
#define XEC_PCR_SCLK_CR_DIV_GET(r) FIELD_GET(XEC_PCR_SCLK_CR_DIV_MSK, (r))

/* Oscillator ID (read-only) */
#define XEC_PCR_OID_OFS            0xcu
#define XEC_PCR_OID_PLL_LOCKED_POS 8

/* Power Reset Status */
#define XEC_PCR_PRS_OFS               0x10u
#define XEC_PCR_PRS_RO_MSK            0xc8eu
#define XEC_PCR_PRS_RW1C_MSK          0x170u
#define XEC_PCR_PRS_VCC_PWRGD2_POS    1  /* read-only */
#define XEC_PCR_PRS_VCC_PWRGD_POS     2  /* read-only */
#define XEC_PCR_PRS_RST_HOST_POS      3  /* read-only */
#define XEC_PCR_PRS_RST_VTR_POS       4  /* read-write-1-clear */
#define XEC_PCR_PRS_RST_VBAT_POS      5  /* read-write-1-clear */
#define XEC_PCR_PRS_RST_SYS_POS       6  /* read-write-1-clear */
#define XEC_PCR_PRS_RST_JTAG_POS      7  /* read-only */
#define XEC_PCR_PRS_WDT_EVENT_POS     8  /* read-write-1-clear */
#define XEC_PCR_PRS_32K_CLK_ACTV_POS  10 /* read-only */
#define XEC_PCR_PRS_ESPI_CLK_ACTV_POS 11 /* read-only */

/* Power Reset Control */
#define XEC_PCR_PRC_OFS          0x14u
#define XEC_PCR_PRC_PWR_INV_POS  0
#define XEC_PCR_PRC_HRST_PIN_POS 8

/* System Reset */
#define XEC_PCR_SYS_RST_OFS        0x18u
#define XEC_PCR_SYS_RST_ASSERT_POS 8

/* Turbo clock MEC172x only */
#define XEC_PCR_TURBO_CLK_OFS    0x1cu
#define XEC_PCR_TURBO_CLK_EN_POS 2

/* Peripheral privilege lock */
#define XEC_PCR_PP_LOCK_OFS    0x24u
#define XEC_PCR_PP_LOCK_EN_POS 0

/* Sleep Enable, Clock Required, and Reset Enable 32-bit registers */
#define XEC_PCR_SECRRE_MAX     5
#define XEC_PCR_SLP_EN_OFS(n)  (0x30u + ((uint32_t)(n) * 4u))
#define XEC_PCR_CLK_REQ_OFS(n) (0x50u + ((uint32_t)(n) * 4u))
#define XEC_PCR_RST_EN_OFS(n)  (0x70u + ((uint32_t)(n) * 4u))

/* Peripheral reset lock */
#define XEC_PCR_RST_LOCK_OFS   0x84u
#define XEC_PCR_RST_LOCK_VAL   0xa6382d4du
#define XEC_PCR_RST_UNLOCK_VAL 0xa6382d4cu

/* VBAT control */
#define XEC_PCR_VBAT_CR_OFS      0x88u
#define XEC_PCR_VBAT_CR_SRST_POS 0

/* VTR 32K clock source select */
#define XEC_PCR_VTR_CLK32K_OFS          0x8cu
#define XEC_PCR_VTR_CLK32K_CSEL_POS     0
#define XEC_PCR_VTR_CLK32K_CSEL_MSK     GENMASK(1, 0)
#define XEC_PCR_VTR_CLK32K_CSEL_SIL_OSC 0
#define XEC_PCR_VTR_CLK32K_CSEL_XTAL    1u
#define XEC_PCR_VTR_CLK32K_CSEL_PIN     2u
#define XEC_PCR_VTR_CLK32K_CSEL_OFF     3u
#define XEC_PCR_VTR_CLK32K_CSEL_SET(s)  FIELD_PREP(XEC_PCR_VTR_32K_CSEL_MSK, (s))
#define XEC_PCR_VTR_CLK32K_CSEL_GET(r)  FIELD_GET(XEC_PCR_VTR_32K_CSEL_MSK, (r))
#define XEC_PCR_VTR_CLK32K_LOCK_POS     8

/* 32K CLKMON period counter (read-only) */
#define XEC_PCR_CMON_PC_OFS    0xc0u
#define XEC_PCR_CMON_PC_MSK    GENMASK(15, 0)
#define XEC_PCR_CMON_PC_GET(r) FIELD_GET(XEC_PCR_CMON_PC_MSK, (r))

/* 32K CLKMON High Pulse counter (read-only) */
#define XEC_PCR_CMON_HPC_OFS 0xc4u
#define XEC_PCR_CMON_HPC_MSK GENMASK(15, 0)

/* 32K CLKMON Period minimum counter (read-only) */
#define XEC_PCR_CMDON_PMIN_OFS 0xc8u
#define XEC_PCR_CMDON_PMIN_MSK GENMASK(15, 0)

/* 32K CLKMON Period maximum count limit (read-write) */
#define XEC_PCR_CMDON_PMAX_LIM_OFS 0xccu
#define XEC_PCR_CMDON_PMAX_LIM_MSK GENMASK(15, 0)

/* 32K CLKMON Duty cycle variation counter (read-only) */
#define XEC_PCR_CMON_DCV_OFS 0xd0u
#define XEC_PCR_CMON_DCV_MSK GENMASK(15, 0)

/* 32K CLKMON Duty cycle variation max limit (read-write) */
#define XEC_PCR_CMON_DCV_MAX_OFS 0xd4u
#define XEC_PCR_CMON_DCV_MAX_MSK GENMASK(15, 0)

/* 32K CLKMON valid count (read-only) */
#define XEC_PCR_CMON_VC_OFS 0xd8u
#define XEC_PCR_CMON_VC_MSK GENMASK(7, 0)

/* 32k CLKMON valid count minimum (read-write) */
#define XEC_PCR_CMON_VC_MIN_OFS 0xdcu
#define XEC_PCR_CMON_VC_MIN_MSK GENMASK(7, 0)

/* 32k CLKMON control */
#define XEC_PCR_CMON_CR_OFS          0xe0u
#define XEC_PCR_CMON_CR_PC_EN_POS    0
#define XEC_PCR_CMON_CR_DC_EN_POS    1
#define XEC_PCR_CMON_CR_VC_EN_POS    2
#define XEC_PCR_CMON_CR_SRC_POS      4
#define XEC_PCR_CMON_CR_SRC_MSK      BIT(4)
#define XEC_PCR_CMON_CR_SRC_XTAL     0
#define XEC_PCR_CMON_CR_SRC_SIL_OSC  1u
#define XEC_PCR_CMON_CR_SRC_SET(s)   FIELD_PREP(XEC_PCR_CMON_CR_SRC_MSK, (s))
#define XEC_PCR_CMON_CR_SRC_GET(r)   FIELD_GET(XEC_PCR_CMON_CR_SRC_MSK, (r))
#define XEC_PCR_CMON_CR_CLR_CNTS_POS 24 /* write-only */

/* 32k CLKMON status (read-write-1-clear) */
#define XEC_PCR_CMON_SR_OFS           0xe4u
#define XEC_PCR_CMON_SR_PULSE_RDY_POS 0
#define XEC_PCR_CMON_SR_PER_PASS_POS  1
#define XEC_PCR_CMON_SR_DCV_PASS_POS  2
#define XEC_PCR_CMON_SR_FAIL_POS      3
#define XEC_PCR_CMON_SR_STALL_POS     4
#define XEC_PCR_CMON_SR_VALID_POS     5
#define XEC_PCR_CMON_SR_UNWELL_POS    6

#define XEC_PCR_CMON_IER_POS           0xe8u
#define XEC_PCR_CMON_IER_PULSE_RDY_POS 0
#define XEC_PCR_CMON_IER_PER_PASS_POS  1
#define XEC_PCR_CMON_IER_DCV_PASS_POS  2
#define XEC_PCR_CMON_IER_FAIL_POS      3
#define XEC_PCR_CMON_IER_STALL_POS     4
#define XEC_PCR_CMON_IER_VALID_POS     5
#define XEC_PCR_CMON_IER_UNWELL_POS    6

#endif /* #ifndef _MEC_PCR_H */
