/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_RTS5817_GPLL_REG_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_RTS5817_GPLL_REG_H_

#define GPLL_SSCG_BASE_ADDR 0
#define R_SYSPLL_CFG        (GPLL_SSCG_BASE_ADDR + 0X0000)
#define R_SYSPLL_NF_CODE    (GPLL_SSCG_BASE_ADDR + 0X0004)
#define R_SYSPLL_CTL        (GPLL_SSCG_BASE_ADDR + 0X0008)
#define R_SYSPLL_STS        (GPLL_SSCG_BASE_ADDR + 0X000C)

/* Bits of R_SYSPLL_CFG (0X0000) */

#define PLL_ORDER_OFFSET 0
#define PLL_ORDER_BITS   1
#define PLL_ORDER_MASK   (((1 << 1) - 1) << 0)
#define PLL_ORDER        (PLL_ORDER_MASK)

#define PLL_BYPASS_PI_OFFSET 1
#define PLL_BYPASS_PI_BITS   1
#define PLL_BYPASS_PI_MASK   (((1 << 1) - 1) << 1)
#define PLL_BYPASS_PI        (PLL_BYPASS_PI_MASK)

#define PLL_REG_CCO_BIG_KVCO_OFFSET 2
#define PLL_REG_CCO_BIG_KVCO_BITS   1
#define PLL_REG_CCO_BIG_KVCO_MASK   (((1 << 1) - 1) << 2)
#define PLL_REG_CCO_BIG_KVCO        (PLL_REG_CCO_BIG_KVCO_MASK)

#define PLL_REG_CCO_SEL_OFFSET 3
#define PLL_REG_CCO_SEL_BITS   1
#define PLL_REG_CCO_SEL_MASK   (((1 << 1) - 1) << 3)
#define PLL_REG_CCO_SEL        (PLL_REG_CCO_SEL_MASK)

#define PLL_REG_CP_OFFSET 4
#define PLL_REG_CP_BITS   3
#define PLL_REG_CP_MASK   (((1 << 3) - 1) << 4)
#define PLL_REG_CP        (PLL_REG_CP_MASK)

#define REG_EN_DCP_OFFSET 7
#define REG_EN_DCP_BITS   1
#define REG_EN_DCP_MASK   (((1 << 1) - 1) << 7)
#define REG_EN_DCP        (REG_EN_DCP_MASK)

#define REG_PI_BIAS_OFFSET 8
#define REG_PI_BIAS_BITS   2
#define REG_PI_BIAS_MASK   (((1 << 2) - 1) << 8)
#define REG_PI_BIAS        (REG_PI_BIAS_MASK)

#define PLL_REG_PI_CAP_OFFSET 10
#define PLL_REG_PI_CAP_BITS   2
#define PLL_REG_PI_CAP_MASK   (((1 << 2) - 1) << 10)
#define PLL_REG_PI_CAP        (PLL_REG_PI_CAP_MASK)

#define REG_PI_EN_OFFSET 12
#define REG_PI_EN_BITS   3
#define REG_PI_EN_MASK   (((1 << 3) - 1) << 12)
#define REG_PI_EN        (REG_PI_EN_MASK)

#define PLL_REG_PI_SEL_OFFSET 15
#define PLL_REG_PI_SEL_BITS   1
#define PLL_REG_PI_SEL_MASK   (((1 << 1) - 1) << 15)
#define PLL_REG_PI_SEL        (PLL_REG_PI_SEL_MASK)

#define REG_SD_OFFSET 16
#define REG_SD_BITS   2
#define REG_SD_MASK   (((1 << 2) - 1) << 16)
#define REG_SD        (REG_SD_MASK)

#define PLL_REG_SEL_VLDO_OFFSET 18
#define PLL_REG_SEL_VLDO_BITS   2
#define PLL_REG_SEL_VLDO_MASK   (((1 << 2) - 1) << 18)
#define PLL_REG_SEL_VLDO        (PLL_REG_SEL_VLDO_MASK)

#define REG_SR_H_OFFSET 20
#define REG_SR_H_BITS   3
#define REG_SR_H_MASK   (((1 << 3) - 1) << 20)
#define REG_SR_H        (REG_SR_H_MASK)

#define PLL_REG_EN_NEW_OFFSET 23
#define PLL_REG_EN_NEW_BITS   1
#define PLL_REG_EN_NEW_MASK   (((1 << 1) - 1) << 23)
#define PLL_REG_EN_NEW        (PLL_REG_EN_NEW_MASK)

#define REG_SC_H_OFFSET 24
#define REG_SC_H_BITS   1
#define REG_SC_H_MASK   (((1 << 1) - 1) << 24)
#define REG_SC_H        (REG_SC_H_MASK)

/* Bits of R_SYSPLL_NF_CODE (0X0004) */

#define N_SSC_OFFSET 0
#define N_SSC_BITS   9
#define N_SSC_MASK   (((1 << 9) - 1) << 0)
#define N_SSC        (N_SSC_MASK)

#define F_SSC_OFFSET 16
#define F_SSC_BITS   12
#define F_SSC_MASK   (((1 << 12) - 1) << 16)
#define F_SSC        (F_SSC_MASK)

/* Bits of R_SYSPLL_CTL (0X0008) */

#define POW_SYSPLL_OFFSET 0
#define POW_SYSPLL_BITS   1
#define POW_SYSPLL_MASK   (((1 << 1) - 1) << 0)
#define POW_SYSPLL        (POW_SYSPLL_MASK)

#define PLL_LOAD_EN_OFFSET 1
#define PLL_LOAD_EN_BITS   1
#define PLL_LOAD_EN_MASK   (((1 << 1) - 1) << 1)
#define PLL_LOAD_EN        (PLL_LOAD_EN_MASK)

#define SYSPLL_REF_SEL_OFFSET 2
#define SYSPLL_REF_SEL_BITS   1
#define SYSPLL_REF_SEL_MASK   (((1 << 1) - 1) << 2)
#define SYSPLL_REF_SEL        (SYSPLL_REF_SEL_MASK)

/* Bits of R_SYSPLL_STS (0X000C) */

#define PLL_CLK_TFBAK_OFFSET 0
#define PLL_CLK_TFBAK_BITS   1
#define PLL_CLK_TFBAK_MASK   (((1 << 1) - 1) << 0)
#define PLL_CLK_TFBAK        (PLL_CLK_TFBAK_MASK)

#define PLL_CKUSABLE_OFFSET 1
#define PLL_CKUSABLE_BITS   1
#define PLL_CKUSABLE_MASK   (((1 << 1) - 1) << 1)
#define PLL_CKUSABLE        (PLL_CKUSABLE_MASK)

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_RTS5817_GPLL_REG_H_ */
