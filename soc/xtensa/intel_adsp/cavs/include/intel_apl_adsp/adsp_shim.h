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

#define CAVS_SHIM (*((volatile struct cavs_shim *)DT_REG_ADDR(DT_NODELABEL(shim))))

#define ADSP_SHIM_DSPWCTS (&CAVS_SHIM.dspwctcs)
#define ADSP_SHIM_DSPWCH  (&CAVS_SHIM.dspwc_hi)
#define ADSP_SHIM_DSPWCL  (&CAVS_SHIM.dspwc_lo)
#define ADSP_SHIM_COMPARE_HI(idx) (&CAVS_SHIM.UTIL_CAT(UTIL_CAT(dspwct, idx), c_hi))
#define ADSP_SHIM_COMPARE_LO(idx) (&CAVS_SHIM.UTIL_CAT(UTIL_CAT(dspwct, idx), c_lo))

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

#define CAVS_CLKCTL_RAPLLC	    BIT(31)
#define CAVS_CLKCTL_RFROSCC	    BIT(29)
#define CAVS_CLKCTL_HPGPDMAFDCGB    BIT(28)
#define CAVS_CLKCTL_LPGPDMAFDCGB(x) BIT(26 + x)
#define CAVS_CLKCTL_SLIMFDCGB	    BIT(25)
#define CAVS_CLKCTL_DMICFDCGB	    BIT(24)
#define CAVS_CLKCTL_I2SFDCGB(x)	    BIT(20 + x)
#define CAVS_CLKCTL_I2SEFDCGB(x)    BIT(18 + x)
#define CAVS_CLKCTL_TCPLCG(x)	    BIT(16 + x) /* Set bit: prevent clock gating on core x */

#define CAVS_CLKCTL_DPCS(div) ((((div)-1) & 3) << 8) /* DSP PLL divisor (1/2/4) */
#define CAVS_CLKCTL_TCPAPLLS  BIT(7)
#define CAVS_CLKCTL_LDCS      BIT(5)
#define CAVS_CLKCTL_HDCS      BIT(4)
#define CAVS_CLKCTL_LDOCS     BIT(3)
#define CAVS_CLKCTL_HDOCS     BIT(2)
#define CAVS_CLKCTL_LMPCS     BIT(1)
#define CAVS_CLKCTL_HMPCS     BIT(0)

#define CAVS_CLKCTL_DPCS_MASK(x) (0x3 << (8 + (x)*2))

#define CAVS_PWRCTL_TCPDSPPG(x) BIT(x)
#define CAVS_PWRSTS_PDSPPGS(x)	BIT(x)

#define SHIM_LDOCTL_HPSRAM_LDO_ON     (3 << 0)
#define SHIM_LDOCTL_HPSRAM_LDO_BYPASS BIT(0)

#define SHIM_LDOCTL_LPSRAM_LDO_ON     (3 << 2)
#define SHIM_LDOCTL_LPSRAM_LDO_BYPASS BIT(2)

#define CAVS_DMWBA_ENABLE   BIT(0)
#define CAVS_DMWBA_READONLY BIT(1)

#define CAVS_CLKCTL_OSC_SOURCE_MASK  BIT_MASK(2)
#define CAVS_CLKCTL_OSC_REQUEST_MASK (~BIT_MASK(28))

#endif /* ZEPHYR_SOC_INTEL_ADSP_SHIM_H_ */
