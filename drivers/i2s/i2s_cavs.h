/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2S bus (SSP) driver for Intel CAVS.
 *
 * Limitations:
 * - DMA is used in simple single block transfer mode (with linked list
 *   enabled) and "interrupt on full transfer completion" mode.
 */

#ifndef ZEPHYR_DRIVERS_I2S_I2S_CAVS_H_
#define ZEPHYR_DRIVERS_I2S_I2S_CAVS_H_

#ifdef __cplusplus
extern "C" {
#endif

struct i2s_cavs_ssp {
	u32_t ssc0;		/* 0x00 - Control0 */
	u32_t ssc1;		/* 0x04 - Control1 */
	u32_t sss;		/* 0x08 - Status */
	u32_t ssit;		/* 0x0C - Interrupt Test */
	u32_t ssd;		/* 0x10 - Data */
	u32_t reserved0[5];
	u32_t ssto;		/* 0x28 - Time Out */
	u32_t sspsp;		/* 0x2C - Programmable Serial Protocol */
	u32_t sstsa;		/* 0x30 - TX Time Slot Active */
	u32_t ssrsa;		/* 0x34 - RX Time Slot Active */
	u32_t sstss;		/* 0x38 - Time Slot Status */
	u32_t reserved1;
	u32_t ssc2;		/* 0x40 - Command / Status 2 */
	u32_t sspsp2;		/* 0x44 - Programmable Serial Protocol 2 */
	u32_t ssc3;		/* 0x48 - Command / Status 3 */
	u32_t ssioc;		/* 0x4C - IO Control */
};

/* SSCR0 bits */
#define SSCR0_DSS_MASK		(0x0000000f)
#define SSCR0_DSIZE(x)		((x) - 1)
#define SSCR0_FRF		(0x00000030)
#define SSCR0_MOT		(00 << 4)
#define SSCR0_TI		(1 << 4)
#define SSCR0_NAT		(2 << 4)
#define SSCR0_PSP		(3 << 4)
#define SSCR0_ECS		(1 << 6)
#define SSCR0_SSE		(1 << 7)
#define SSCR0_SCR_MASK		(0x000fff00)
#define SSCR0_SCR(x)		((x) << 8)
#define SSCR0_EDSS		(1 << 20)
#define SSCR0_NCS		(1 << 21)
#define SSCR0_RIM		(1 << 22)
#define SSCR0_TIM		(1 << 23)
#define SSCR0_FRDC(x)		(((x) - 1) << 24)
#define SSCR0_ACS		(1 << 30)
#define SSCR0_MOD		(1 << 31)

/* SSCR1 bits */
#define SSCR1_RIE		(1 << 0)
#define SSCR1_TIE		(1 << 1)
#define SSCR1_LBM		(1 << 2)
#define SSCR1_SPO		(1 << 3)
#define SSCR1_SPH		(1 << 4)
#define SSCR1_MWDS		(1 << 5)
#define SSCR1_EFWR		(1 << 14)
#define SSCR1_STRF		(1 << 15)
#define SSCR1_IFS		(1 << 16)
#define SSCR1_PINTE		(1 << 18)
#define SSCR1_TINTE		(1 << 19)
#define SSCR1_RSRE		(1 << 20)
#define SSCR1_TSRE		(1 << 21)
#define SSCR1_TRAIL		(1 << 22)
#define SSCR1_RWOT		(1 << 23)
#define SSCR1_SFRMDIR		(1 << 24)
#define SSCR1_SCLKDIR		(1 << 25)
#define SSCR1_ECRB		(1 << 26)
#define SSCR1_ECRA		(1 << 27)
#define SSCR1_SCFR		(1 << 28)
#define SSCR1_EBCEI		(1 << 29)
#define SSCR1_TTE		(1 << 30)
#define SSCR1_TTELP		(1 << 31)

/* SSCR2 bits */
#define SSCR2_TURM1		(1 << 1)
#define SSCR2_SDFD		(1 << 14)
#define SSCR2_SDPM		(1 << 16)
#define SSCR2_LJDFD		(1 << 17)

/* SSR bits */
#define SSSR_TNF		(1 << 2)
#define SSSR_RNE		(1 << 3)
#define SSSR_BSY		(1 << 4)
#define SSSR_TFS		(1 << 5)
#define SSSR_RFS		(1 << 6)
#define SSSR_ROR		(1 << 7)

/* SSPSP bits */
#define SSPSP_SCMODE(x)		((x) << 0)
#define SSPSP_SFRMP(x)		((x) << 2)
#define SSPSP_ETDS		(1 << 3)
#define SSPSP_STRTDLY(x)	((x) << 4)
#define SSPSP_DMYSTRT(x)	((x) << 7)
#define SSPSP_SFRMDLY(x)	((x) << 9)
#define SSPSP_SFRMWDTH(x)	((x) << 16)
#define SSPSP_DMYSTOP(x)	((x) << 23)
#define SSPSP_FSRT		(1 << 25)
#define SSPSP_EDMYSTOP(x)	((x) << 26)

/* SSTSA bits */
#define SSTSA_TTSA(x)		(1 << x)
#define SSTSA_TXEN		(1 << 8)

/* SSRSA bits */
#define SSRSA_RTSA(x)		(1 << x)
#define SSRSA_RXEN		(1 << 8)

/* SSCR3 bits */
#define SSCR3_TFL_MASK		(0x0000003f)
#define SSCR3_RFL_MASK		(0x00003f00)
#define SSCR3_TFT_MASK		(0x003f0000)
#define SSCR3_TX(x)		(((x) - 1) << 16)
#define SSCR3_RFT_MASK		(0x3f000000)
#define SSCR3_RX(x)		(((x) - 1) << 24)

/* SSIOC bits */
#define SSIOC_TXDPDEB		(1 << 1)
#define SSIOC_SFCR		(1 << 4)
#define SSIOC_SCOE		(1 << 5)

struct i2s_cavs_mn_div {
	u32_t mval;		/* 0x00 - M value */
	u32_t nval;		/* 0x04 - N value */
};

/* MVAL & NVAL bits */
#define I2S_MNVAL_MASK		(BIT_MASK(24))
#define I2S_MNVAL(x)		((x) & I2S_MNVAL_MASK)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2S_I2S_CAVS_H_ */
