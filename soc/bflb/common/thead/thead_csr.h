/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define THEAD_MXSTATUS	"0x7C0"
#define THEAD_MHCR	"0x7C1"
#define THEAD_MHINT	"0x7C5"
#define THEAD_MEXSTATUS	"0x7E1"

/* Enforce memory alignment disable */
#define THEAD_MXSTATUS_MM		BIT(15)
/* XThead extension enable */
#define THEAD_MXSTATUS_THEADISAEE	BIT(22)

/* I-cache enable */
#define THEAD_MHCR_IE			BIT(0)
/* D-cache enable */
#define THEAD_MHCR_DE			BIT(1)
/* Cache write-back enable */
#define THEAD_MHCR_WB			BIT(2)
/* Cache write-allocate enable */
#define THEAD_MHCR_WA			BIT(3)
/* Return stack enable */
#define THEAD_MHCR_RS			BIT(4)
/* Branch prediction enable */
#define THEAD_MHCR_BPE			BIT(5)
/* Branch target prediction enable */
#define THEAD_MHCR_BTB			BIT(12)

/* Cache prefetch enable */
#define THEAD_MHINT_DPLD		BIT(2)
/* Cache write allocation adjustment policy */
#define THEAD_MHINT_AMR			GENMASK(4, 3)
/* ICache prediction enable */
#define THEAD_MHINT_IWPE		BIT(10)
/* The number of cache lines prefetched when DCACHE prefetching is enabled */
#define THEAD_MHINT_PREF_N		GENMASK(14, 13)

/* Interrupt auto-stacking enable */
#define THEAD_MEXSTATUS_SPUSH		BIT(16)
/* Automatic interrupt stack switching */
#define THEAD_MEXSTATUS_SPSWAP		BIT(17)

enum thead_amr {
	/* Defined by 'WA' flag in MHCR */
	AMR_MHCR = 0,
	/* If 3 consecutive cache lines are stored, storage operations of subsequent
	 * continuous addresses are no longer written to L1 Cache.
	 *
	 * C906
	 */
	AMR_3L = 1,
	AMR_64L = 2,
	AMR_128L = 3,
	/* E907 */
	AMR_7L = 1,
	AMR_31L = 2,
	AMR_127L = 3,
};

enum thead_prefetch_n {
	PREF_N_2 = 0,
	PREF_N_4 = 1,
	PREF_N_8 = 2,
	PREF_N_16 = 3,
};

void enable_branchpred(bool yes);
void enable_thead_isa_ext(void);
void set_thead_enforce_aligned(bool enable);
void disable_interrupt_autostacking(void);
void set_thead_mhint(bool dpld, enum thead_amr amr, enum thead_prefetch_n pn);
