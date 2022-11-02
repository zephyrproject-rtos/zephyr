/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_SHIM_H_
#define ZEPHYR_SOC_INTEL_ADSP_SHIM_H_

/* The "shim" block contains most of the general system control
 * registers on cAVS platforms.  While the base address changes, it
 * has remained largely, but not perfectly, compatible between
 * versions.
 */

#ifndef _ASMLANGUAGE
struct cavs_shim {
	uint32_t skuid;
	uint32_t _unused0[7];
	uint32_t dspwc_lo;
	uint32_t dspwc_hi;
	uint32_t dspwctcs;
	uint32_t _unused1[1];
	uint32_t dspwct0c_lo;
	uint32_t dspwct0c_hi;
	uint32_t dspwct1c_lo;
	uint32_t dspwct1c_hi;
	uint32_t _unused2[14];
	uint32_t clkctl;
	uint32_t clksts;
	uint32_t _unused3[4];
	uint16_t pwrctl;
	uint16_t pwrsts;
	uint32_t lpsctl;
	uint32_t lpsdmas0;
	uint32_t lpsdmas1;
	uint32_t spsreq;
	uint32_t ldoctl;
	uint32_t _unused4[2];
	uint32_t lpsalhss0;
	uint32_t lpsalhss1;
	uint32_t lpsalhss2;
	uint32_t lpsalhss3;
	uint32_t _unused5[4];
	uint32_t l2mecs;
	uint32_t l2mpat;
	uint32_t _unused6[2];
	uint32_t ltrc;
	uint32_t _unused8[3];
	uint32_t dbgo;
	uint32_t svcfg;
	uint32_t _unused9[2];
};

#define CAVS_SHIM (*((volatile struct cavs_shim *)DT_REG_ADDR(DT_NODELABEL(shim))))

#define ADSP_DSPWC_OFFSET	0x20
#define ADSP_DSPWCTCS_OFFSET	0x28
#define ADSP_DSPWCT0C_OFFSET	0x30
#define ADSP_DSPWCT1C_OFFSET	0x38
#define ADSP_CLKCTL_OFFSET	0x78
#define ADSP_CLKSTS_OFFSET	0x7C
#define ADSP_PWRCTL_OFFSET	0x90
#define ADSP_PWRSTS_OFFSET	0x92
#define ADSP_LPSCTL_OFFSET	0x94


#define ADSP_SHIM_DSPWCTCS_TTIE(c) BIT(8 + (c))

/* L2 Local Memory control (cAVS 1.8+) */
struct cavs_l2lm {
	uint32_t l2lmcap;
	uint32_t l2lmpat;
	uint32_t _unused0[2];
	uint32_t hspgctl0;
	uint32_t hsrmctl0;
	uint32_t hspgists0;
	uint32_t _unused1;
	uint32_t hspgctl1;
	uint32_t hsrmctl1;
	uint32_t hspgists1;
	uint32_t _unused2[9];
	uint32_t lspgctl;
	uint32_t lsrmctl;
	uint32_t lspgists;
};

#define CAVS_L2LM (*((volatile struct cavs_l2lm *)DT_REG_ADDR(DT_NODELABEL(l2lm))))

/* Host memory window control.  Not strictly part of the shim block. */
struct cavs_win {
	uint32_t dmwba;
	uint32_t dmwlo;
};

#define CAVS_WIN ((volatile struct cavs_win *)DT_REG_ADDR(DT_NODELABEL(win)))

#endif /* _ASMLANGUAGE */

/* cAVS 1.8+ CLKCTL bits */
#define CAVS_CLKCTL_RHROSCC   BIT(31)	  /* Request HP RING oscillator */
#define CAVS_CLKCTL_RXOSCC    BIT(30)	  /* Request XTAL oscillator */
#define CAVS_CLKCTL_RLROSCC   BIT(29)	  /* Request LP RING oscillator */
#define CAVS_CLKCTL_SLIMFDCGB BIT(25)	  /* Slimbus force dynamic clock gating*/
#define CAVS_CLKCTL_TCPLCG(x) BIT(16 + x) /* Set bit: prevent clock gating on core x */
#define CAVS_CLKCTL_SLIMCSS   BIT(6)	  /* Slimbus clock (0: XTAL, 1: Audio) */
#define CAVS_CLKCTL_WOVCRO    BIT(4)	  /* Request WOVCRO clock */
#define CAVS_CLKCTL_WOVCROSC  BIT(3)	  /* WOVCRO select */
#define CAVS_CLKCTL_OCS	      BIT(2)	  /* Oscillator clock (0: LP, 1: HP) */
#define CAVS_CLKCTL_LMCS      BIT(1)	  /* LP mem divisor (0: div/2, 1: div/4) */
#define CAVS_CLKCTL_HMCS      BIT(0)	  /* HP mem divisor (0: div/2, 1: div/4) */

#define CAVS_PWRCTL_TCPDSPPG(x) BIT(x)
#define CAVS_PWRSTS_PDSPPGS(x)	BIT(x)

#define SHIM_LDOCTL_HPSRAM_LDO_ON     (3 << 0)
#define SHIM_LDOCTL_HPSRAM_LDO_BYPASS BIT(0)

#define SHIM_LDOCTL_LPSRAM_LDO_ON     (3 << 2)
#define SHIM_LDOCTL_LPSRAM_LDO_BYPASS BIT(2)

#define ADSP_DMWBA_ENABLE   BIT(0)
#define ADSP_DMWBA_READONLY BIT(1)

#define ADSP_CLKCTL_OSC_SOURCE_MASK  BIT_MASK(2)
#define ADSP_CLKCTL_OSC_REQUEST_MASK (~BIT_MASK(28))


/** LDO Control */
#define ADSP_DSPRA_ADDRESS        (0x71A60)
#define ADSP_LPGPDMACxO_ADDRESS(x) (ADSP_DSPRA_ADDRESS + 0x0000 + 0x0002*(x))
#define ADSP_DSPIOPO_ADDRESS      (ADSP_DSPRA_ADDRESS + 0x0008)
#define ADSP_GENO_ADDRESS         (ADSP_DSPRA_ADDRESS + 0x000C)
#define ADSP_DSPALHO_ADDRESS      (ADSP_DSPRA_ADDRESS + 0x0010)


#define DSP_INIT_IOPO   ADSP_DSPIOPO_ADDRESS
#define IOPO_DMIC_FLAG          BIT(0)
#define IOPO_DSPKOSEL_FLAG      BIT(1)
#define IOPO_ANCOSEL_FLAG       BIT(2)
#define IOPO_DMIXOSEL_FLAG      BIT(3)
#define IOPO_SLIMOSEL_FLAG      BIT(4)
#define IOPO_SNDWOSEL_FLAG      BIT(5)
#define IOPO_SLIMDOSEL_FLAG     BIT(20)
#define IOPO_I2SSEL_MASK	(0x7 << 0x8)

#define DSP_INIT_GENO   ADSP_GENO_ADDRESS
#define GENO_MDIVOSEL           BIT(1)
#define GENO_DIOPTOSEL          BIT(2)

#endif /* ZEPHYR_SOC_INTEL_ADSP_SHIM_H_ */
