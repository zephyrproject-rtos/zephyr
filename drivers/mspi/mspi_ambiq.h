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

#define MSPI_CQ_MAX_ENTRY MSPI0_CQCURIDX_CQCURIDX_Msk

enum mspi_ambiq_timing_param {
	MSPI_AMBIQ_SET_WLC            = BIT(0),
	MSPI_AMBIQ_SET_RLC            = BIT(1),
	MSPI_AMBIQ_SET_TXNEG          = BIT(2),
	MSPI_AMBIQ_SET_RXNEG          = BIT(3),
	MSPI_AMBIQ_SET_RXCAP          = BIT(4),
	MSPI_AMBIQ_SET_TXDQSDLY       = BIT(5),
	MSPI_AMBIQ_SET_RXDQSDLY       = BIT(6),
};

enum mspi_ambiq_timing_scan_type {
	MSPI_AMBIQ_TIMING_SCAN_MEMC   = 0,
	MSPI_AMBIQ_TIMING_SCAN_FLASH,
};

struct mspi_ambiq_timing_scan_range {
	int8_t rlc_start;
	int8_t rlc_end;
	int8_t txneg_start;
	int8_t txneg_end;
	int8_t rxneg_start;
	int8_t rxneg_end;
	int8_t rxcap_start;
	int8_t rxcap_end;
	int8_t txdqs_start;
	int8_t txdqs_end;
	int8_t rxdqs_start;
	int8_t rxdqs_end;
};

struct mspi_ambiq_timing_cfg {
	uint8_t ui8WriteLatency;
	uint8_t ui8TurnAround;
	bool bTxNeg;
	bool bRxNeg;
	bool bRxCap;
	uint32_t ui32TxDQSDelay;
	uint32_t ui32RxDQSDelay;
};

struct mspi_ambiq_timing_scan {
	struct mspi_ambiq_timing_scan_range  range;
	enum mspi_ambiq_timing_scan_type     scan_type;
	uint32_t                             min_window;
	uint32_t                             device_addr;
	struct mspi_ambiq_timing_cfg         result;
};

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

#define MSPI_AMBIQ_TIMING_CONFIG(n)                                                               \
	{                                                                                         \
		.ui8WriteLatency    = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 0),             \
		.ui8TurnAround      = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 1),             \
		.bTxNeg             = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 2),             \
		.bRxNeg             = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 3),             \
		.bRxCap             = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 4),             \
		.ui32TxDQSDelay     = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 5),             \
		.ui32RxDQSDelay     = DT_INST_PROP_BY_IDX(n, ambiq_timing_config, 6),             \
	}

#define MSPI_AMBIQ_TIMING_CONFIG_MASK(n) DT_INST_PROP(n, ambiq_timing_config_mask)

#define MSPI_AMBIQ_PORT(n) ((DT_REG_ADDR(DT_INST_BUS(n)) - MSPI0_BASE) /                          \
				(MSPI1_BASE - MSPI0_BASE))


int mspi_ambiq_timing_scan(const struct device           *dev,
			   const struct device           *bus,
			   const struct mspi_dev_id      *dev_id,
			   uint32_t                       param_mask,
			   struct mspi_ambiq_timing_cfg  *timing,
			   struct mspi_ambiq_timing_scan *scan);

#endif
