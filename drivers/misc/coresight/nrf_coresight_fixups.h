/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_MISC_CORESIGHT_NRF_CORESIGHT_FIXUPS_H_
#define ZEPHYR_DRIVERS_MISC_CORESIGHT_NRF_CORESIGHT_FIXUPS_H_

#include <nrf.h>

#ifndef NRF_TSGEN_NS_BASE
#define NRF_TSGEN_NS_BASE 0xBF041000UL
#endif

#ifndef NRF_TSGEN_NS
#define NRF_TSGEN_NS ((NRF_TSGEN_Type *)NRF_TSGEN_NS_BASE)
#endif

#ifndef NRF_TSGEN
#define NRF_TSGEN NRF_TSGEN_NS
#endif

#ifndef NRF_ETR_NS_BASE
#define NRF_ETR_NS_BASE 0xBF045000UL
#endif

#ifndef NRF_ETR_NS
#define NRF_ETR_NS ((NRF_ETR_Type *)NRF_ETR_NS_BASE)
#endif

#ifndef NRF_ETR
#define NRF_ETR NRF_ETR_NS
#endif

typedef struct {
	__IOM uint32_t STMSTIMR[32];
	__IM uint32_t RESERVED0[737];
	__OM uint32_t STMDMASTARTR;
	__OM uint32_t STMDMASTOPR;
	__IM uint32_t STMDMASTATR;
	__IOM uint32_t STMDMACTLR;
	__IM uint32_t RESERVED1[58];
	__IM uint32_t STMDMAIDR;
	__IOM uint32_t STMHEER;
	__IM uint32_t RESERVED2[7];
	__IOM uint32_t STMHETER;
	__IM uint32_t RESERVED3[16];
	__IOM uint32_t STMHEMCR;
	__IM uint32_t RESERVED4[35];
	__IM uint32_t STMHEMASTR;
	__IM uint32_t STMHEFEAT1R;
	__IM uint32_t STMHEIDR;
	__IOM uint32_t STMSPER;
	__IM uint32_t RESERVED5[7];
	__IOM uint32_t STMSPTER;
	__IM uint32_t RESERVED6[15];
	__IOM uint32_t STMSPSCR;
	__IOM uint32_t STMSPMSCR;
	__IOM uint32_t STMSPOVERRIDER;
	__IOM uint32_t STMSPMOVERRIDER;
	__IOM uint32_t STMSPTRIGCSR;
	__IM uint32_t RESERVED7[3];
	__IOM uint32_t STMTCSR;
	__OM uint32_t STMTSSTIMR;
	__IM uint32_t RESERVED8;
	__IOM uint32_t STMTSFREQR;
	__IOM uint32_t STMSYNCR;
	__IOM uint32_t STMAUXCR;
	__IM uint32_t RESERVED9[2];
	__IOM uint32_t STMSPFEAT1R;
	__IOM uint32_t STMSPFEAT2R;
	__IOM uint32_t STMSPFEAT3R;
	__IM uint32_t RESERVED10[15];
	__OM uint32_t STMITTRIGGER;
	__OM uint32_t STMITATBDATA0;
	__OM uint32_t STMITATBCTR2;
	__OM uint32_t STMITATBID;
	__OM uint32_t STMITATBCTR0;
	__IM uint32_t RESERVED11;
	__IOM uint32_t ITCTRL;
	__IM uint32_t RESERVED12[43];
	__IOM uint32_t LAR;
	__IOM uint32_t LSR;
	__IOM uint32_t AUTHSTATUS;
	__IM uint32_t RESERVED13[3];
	__IM uint32_t DEVID;
	__IM uint32_t DEVTYPE;
	__IOM uint32_t PIDR4;
	__IM uint32_t RESERVED14[3];
	__IOM uint32_t PIDR0;
	__IOM uint32_t PIDR1;
	__IOM uint32_t PIDR2;
	__IOM uint32_t PIDR3;
	__IOM uint32_t CIDR0;
	__IOM uint32_t CIDR1;
	__IOM uint32_t CIDR2;
	__IOM uint32_t CIDR3;
} NRF_STM_Type_fixed;

#if defined(NRF_STM_NS)
#undef NRF_STM_NS
#define NRF_STM_NS ((NRF_STM_Type_fixed *)NRF_STM_NS_BASE)
#endif

#define NRF_STM_Type NRF_STM_Type_fixed

#endif /* ZEPHYR_DRIVERS_MISC_CORESIGHT_NRF_CORESIGHT_FIXUPS_H_ */
