/*
 * Copyright (c) 2025 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Microchip XEC MCU family EC Interrupt Aggregator support.
 *
 */

#ifndef _MICROCHIP_MEC_SOC_ECIA_H_
#define _MICROCHIP_MEC_SOC_ECIA_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/arch/common/sys_bitops.h>

/* zero based GIRQ numbering. 19 total GIRQ units in the aggregator */
#define MCHP_MEC_ECIA_ZGIRQ_MAX 19

/* Historically, GIRQ's have been numbered starting with 8 */
#define MCHP_MEC_ECIA_GIRQ_FIRST 8
#define MCHP_MEC_ECIA_GIRQ_LAST  26

/* MEC ECIA GIRQ's are numbered 8 - 26 for historical reasons.
 * GIRQ's 8 - 12, 22, 24 - 26 interrupt sources are only connected to the GIRQ source bits.
 * GIRQ's 13 - 21, and 23 result bits can be connected to the NVIC.
 */
#define MCHP_MEC_ECIA_GIRQ_ALL_BM        GENMASK(26, 8)
#define MCHP_MEC_ECIA_GIRQ_AGGR_ONLY_BM  (GENMASK(12, 8) | BIT(22) | GENMASK(26, 24))
#define MCHP_MEC_ECIA_GIRQ_DIRECT_CAP_BM (GENMASK(21, 13) | BIT(23))

enum mchp_mec_ecia_girq {
	MCHP_MEC_ECIA_GIRQ8 = MCHP_MEC_ECIA_GIRQ_FIRST,
	MCHP_MEC_ECIA_GIRQ9,
	MCHP_MEC_ECIA_GIRQ10,
	MCHP_MEC_ECIA_GIRQ11,
	MCHP_MEC_ECIA_GIRQ12,
	MCHP_MEC_ECIA_GIRQ13,
	MCHP_MEC_ECIA_GIRQ14,
	MCHP_MEC_ECIA_GIRQ15,
	MCHP_MEC_ECIA_GIRQ16,
	MCHP_MEC_ECIA_GIRQ17,
	MCHP_MEC_ECIA_GIRQ18,
	MCHP_MEC_ECIA_GIRQ19,
	MCHP_MEC_ECIA_GIRQ20,
	MCHP_MEC_ECIA_GIRQ21,
	MCHP_MEC_ECIA_GIRQ22,
	MCHP_MEC_ECIA_GIRQ23,
	MCHP_MEC_ECIA_GIRQ24,
	MCHP_MEC_ECIA_GIRQ25,
	MCHP_MEC_ECIA_GIRQ26,
	MCHP_MEC_ECIA_GIRQ_MAX,
};

#define MCHP_MEC_ECIA_INIT_CLR_ENABLES 0x01u
#define MCHP_MEC_ECIA_INIT_CLR_STATUS  0x02u

int soc_ecia_init(uint32_t aggr_girq_bm, uint32_t direct_girq_bm, uint32_t flags);

int soc_ecia_girq_ctrl_bm(uint8_t girq, uint32_t bitmap, uint8_t enable);
int soc_ecia_girq_ctrl(uint8_t girq, uint8_t srcpos, uint8_t enable);

uint32_t soc_ecia_girq_get_enable_bm(uint8_t girq);

int soc_ecia_girq_status_clear_bm(uint8_t girq, uint32_t bitmap);
int soc_ecia_girq_status_clear(uint8_t girq, uint8_t srcpos);

int soc_ecia_girq_status(uint8_t girq, uint32_t *status);
int soc_ecia_girq_result(uint8_t girq, uint32_t *result);
int soc_ecia_girq_is_result(uint8_t girq, uint32_t bitmap);

int soc_ecia_girq_aggr_ctrl_bm(uint32_t girq_bitmap, uint8_t enable);
int soc_ecia_girq_aggr_ctrl(uint8_t girq, uint8_t enable);

#endif /* _MICROCHIP_MEC_SOC_ECIA_H_ */
