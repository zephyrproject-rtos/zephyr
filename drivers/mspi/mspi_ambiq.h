/*
 * Copyright (c) 2024, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MSPI_AMBIQ_H_
#define MSPI_AMBIQ_H_

#include <soc.h>

/* Hand-calculated minimum heap sizes needed to return a successful
 * 1-byte allocation.  See details in lib/os/heap.[ch]
 */
#define MSPI_AMBIQ_HEAP_MIN_SIZE (sizeof(void *) > 4 ? 56 : 44)

#define MSPI_AMBIQ_HEAP_DEFINE(name, bytes)                                                       \
	char __attribute__((section(".mspi_buff")))                                               \
		kheap_##name[MAX(bytes, MSPI_AMBIQ_HEAP_MIN_SIZE)];                               \
	STRUCT_SECTION_ITERABLE(k_heap, name) = {                                                 \
		.heap =                                                                           \
			{                                                                         \
				.init_mem = kheap_##name,                                         \
				.init_bytes = MAX(bytes, MSPI_AMBIQ_HEAP_MIN_SIZE),               \
			},                                                                        \
	}

struct mspi_ambiq_timing_cfg {
	uint8_t ui8WriteLatency;
	uint8_t ui8TurnAround;
	bool bTxNeg;
	bool bRxNeg;
	bool bRxCap;
	uint32_t ui32TxDQSDelay;
	uint32_t ui32RxDQSDelay;
	uint32_t ui32RXDQSDelayEXT;
};

enum mspi_ambiq_timing_param {
	MSPI_AMBIQ_SET_WLC            = BIT(0),
	MSPI_AMBIQ_SET_RLC            = BIT(1),
	MSPI_AMBIQ_SET_TXNEG          = BIT(2),
	MSPI_AMBIQ_SET_RXNEG          = BIT(3),
	MSPI_AMBIQ_SET_RXCAP          = BIT(4),
	MSPI_AMBIQ_SET_TXDQSDLY       = BIT(5),
	MSPI_AMBIQ_SET_RXDQSDLY       = BIT(6),
	MSPI_AMBIQ_SET_RXDQSDLYEXT    = BIT(7),
};

#define MSPI_PORT(n)    ((DT_REG_ADDR(DT_INST_BUS(n)) - MSPI0_BASE) /                             \
			(DT_REG_SIZE(DT_INST_BUS(n)) * 4))

#define TIMING_CFG_GET_RX_DUMMY(cfg)                                                              \
	{                                                                                         \
		mspi_timing_cfg *timing = (mspi_timing_cfg *)cfg;                                 \
		timing->ui8TurnAround;                                                            \
	}

#define TIMING_CFG_SET_RX_DUMMY(cfg, num)                                                         \
	{                                                                                         \
		mspi_timing_cfg *timing = (mspi_timing_cfg *)cfg;                                 \
		timing->ui8TurnAround = num;                                                      \
	}

#endif
