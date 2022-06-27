/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_CAVS_SHIM_H_
#define ZEPHYR_SOC_INTEL_ADSP_CAVS_SHIM_H_

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
	uint32_t hspgctl; /* cAVS 1.5, see cavs_l2lm for 1.8+ */
	uint32_t lspgctl; /* cAVS 1.5, see cavs_l2lm for 1.8+ */
	uint32_t hsrmctl; /* cAVS 1.5, see cavs_l2lm for 1.8+ */
	uint32_t lsrmctl; /* cAVS 1.5, see cavs_l2lm for 1.8+ */
	uint16_t pwrctl;
	uint16_t pwrsts;
	uint32_t lpsctl;
	uint32_t lpsdmas0;
	uint32_t lpsdmas1;
	uint32_t spsreq;
	uint32_t ldoctl;
	uint32_t _unused3[2];
	union {
		/* cAVS 1.5 */
		struct {
			uint32_t hspgists;
			uint32_t lspgists;
			uint32_t _unused4[2];
		};
		/* cAVS 1.8+ */
		struct {
			uint32_t lpsalhss0;
			uint32_t lpsalhss1;
			uint32_t lpsalhss2;
			uint32_t lpsalhss3;
		};
	};
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

#define CAVS_SHIM (*((volatile struct cavs_shim *)DT_REG_ADDR(DT_NODELABEL(shim))))

/* cAVS 1.8+ CLKCTL bits */
#define CAVS_CLKCTL_RHROSCC   BIT(31)   /* Request HP RING oscillator */
#define CAVS_CLKCTL_RXOSCC    BIT(30)   /* Request XTAL oscillator */
#define CAVS_CLKCTL_RLROSCC   BIT(29)   /* Request LP RING oscillator */
#define CAVS_CLKCTL_SLIMFDCGB BIT(25)   /* Slimbus force dynamic clock gating*/
#define CAVS_CLKCTL_TCPLCG(x) BIT(16+x) /* Set bit: prevent clock gating on core x */
#define CAVS_CLKCTL_SLIMCSS   BIT(6)    /* Slimbus clock (0: XTAL, 1: Audio) */
#define CAVS_CLKCTL_OCS       BIT(2)    /* Oscillator clock (0: LP, 1: HP) */
#define CAVS_CLKCTL_LMCS      BIT(1)    /* LP mem divisor (0: div/2, 1: div/4) */
#define CAVS_CLKCTL_HMCS      BIT(0)    /* HP mem divisor (0: div/2, 1: div/4) */

/* cAVS 1.5 had a somewhat different CLKCTL (some fields were the same) */
#define CAVS15_CLKCTL_RAPLLC          BIT(31)
#define CAVS15_CLKCTL_RFROSCC         BIT(29)
#define CAVS15_CLKCTL_HPGPDMAFDCGB    BIT(28)
#define CAVS15_CLKCTL_LPGPDMAFDCGB(x) BIT(26+x)
#define CAVS15_CLKCTL_SLIMFDCGB       BIT(25)
#define CAVS15_CLKCTL_DMICFDCGB       BIT(24)
#define CAVS15_CLKCTL_I2SFDCGB(x)     BIT(20+x)
#define CAVS15_CLKCTL_I2SEFDCGB(x)    BIT(18+x)
#define CAVS15_CLKCTL_DPCS(div) ((((div)-1) & 3) << 8) /* DSP PLL divisor (1/2/4) */
#define CAVS15_CLKCTL_TCPAPLLS        BIT(7)
#define CAVS15_CLKCTL_LDCS            BIT(5)
#define CAVS15_CLKCTL_HDCS            BIT(4)
#define CAVS15_CLKCTL_LDOCS           BIT(3)
#define CAVS15_CLKCTL_HDOCS           BIT(2)
#define CAVS15_CLKCTL_LMPCS           BIT(1)
#define CAVS15_CLKCTL_HMPCS           BIT(0)

#define CAVS_PWRCTL_TCPDSPPG(x) BIT(x)
#define CAVS_PWRSTS_PDSPPGS(x)  BIT(x)

#ifdef SOC_SERIES_INTEL_CAVS_V25
# define SHIM_LDOCTL_HPSRAM_LDO_ON     (3 << 0 | 3 << 16)
# define SHIM_LDOCTL_HPSRAM_LDO_BYPASS BIT(16)
#else
# define SHIM_LDOCTL_HPSRAM_LDO_ON     (3 << 0)
# define SHIM_LDOCTL_HPSRAM_LDO_BYPASS BIT(0)
#endif
#define SHIM_LDOCTL_LPSRAM_LDO_ON     (3 << 2)
#define SHIM_LDOCTL_LPSRAM_LDO_BYPASS BIT(2)

#define CAVS_DMWBA_ENABLE   BIT(0)
#define CAVS_DMWBA_READONLY BIT(1)

#endif /* ZEPHYR_SOC_INTEL_ADSP_CAVS_SHIM_H_ */
