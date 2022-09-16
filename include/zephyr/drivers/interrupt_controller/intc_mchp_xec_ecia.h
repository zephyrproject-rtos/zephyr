/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for External interrupt controller in Microchip XEC devices
 *
 * Based on reference manuals:
 *   Reference Manuals for MEC152x and MEC172x ARM(r) 32-bit MCUs
 *
 * Chapter: EC Interrupt Aggregator (ECIA)
 *
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_MCHP_XEC_ECIA_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_MCHP_XEC_ECIA_H_

#include <zephyr/device.h>
#include <zephyr/irq.h>

/**
 * @brief enable GIRQn interrupt for specific source
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param src is the interrupt source in the GIRQ (0 - 31)
 */
int mchp_xec_ecia_enable(int girq_id, int src);

/**
 * @brief enable EXTI interrupt for specific line specified by parameter
 * encoded with MCHP_XEC_ECIA macro.
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 */
int mchp_xec_ecia_info_enable(int ecia_info);

/**
 * @brief disable EXTI interrupt for specific line
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param src is the interrupt source in the GIRQ (0 - 31)
 */
int mchp_xec_ecia_disable(int girq_id, int src);

/**
 * @brief disable EXTI interrupt for specific line specified by parameter
 * encoded with MCHP_XEC_ECIA macro.
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 */
int mchp_xec_ecia_info_disable(int ecia_info);


/* callback for ECIA GIRQ interrupt source */
typedef void (*mchp_xec_ecia_callback_t) (int girq_id, int src, void *user);

/**
 * @brief set GIRQn interrupt source callback
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param src is the interrupt source in the GIRQ (0 - 31)
 * @param cb user callback
 * @param data user data
 */
int mchp_xec_ecia_set_callback(int girq_id, int src,
			       mchp_xec_ecia_callback_t cb, void *data);

/**
 * @brief set GIRQn interrupt source callback
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 * @param cb user callback
 * @param data user data
 */
int mchp_xec_ecia_info_set_callback(int ecia_info, mchp_xec_ecia_callback_t cb,
				    void *data);

/**
 * @brief set GIRQn interrupt source callback
 *
 * @param dev_girq is a handle to the GIRQn device
 * @param src is the interrupt source in the GIRQ (0 - 31)
 * @param cb user callback
 * @param data user data
 */
int mchp_xec_ecia_set_callback_by_dev(const struct device *dev_girq, int src,
				      mchp_xec_ecia_callback_t cb, void *data);

/**
 * @brief unset GIRQn interrupt source callback
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param src is the interrupt source in the GIRQ (0 - 31)
 */
int mchp_ecia_unset_callback(int girq_id, int src);

/**
 * @brief unset GIRQn interrupt source callback
 *
 * @param dev_girq is a handle to the GIRQn device
 * @param src is the interrupt source in the GIRQ (0 - 31)
 */
int mchp_ecia_unset_callback_by_dev(const struct device *dev_girq, int src);

/* platform specific */
/** @brief enable or disable aggregated GIRQ output
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param enable non-zero enables aggregated output else disables
 */
void mchp_xec_ecia_girq_aggr_en(uint8_t girq_id, uint8_t enable);

/** @brief clear GIRQ latched source status bit
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param src_bit is the source bit position in the GIRQ registers (0 - 31)
 */
void mchp_xec_ecia_girq_src_clr(uint8_t girq_id, uint8_t src_bit);

/** @brief enable a source in a GIRQ
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param src_bit is the source bit position in the GIRQ registers (0 - 31)
 */
void mchp_xec_ecia_girq_src_en(uint8_t girq_id, uint8_t src_bit);

/** @brief disable a source in a GIRQ
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param src_bit is the source bit position in the GIRQ registers (0 - 31)
 */
void mchp_xec_ecia_girq_src_dis(uint8_t girq_id, uint8_t src_bit);

/** @brief clear GIRQ latches sources specified in bitmap
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param bitmap contains the source bits to clear
 */
void mchp_xec_ecia_girq_src_clr_bitmap(uint8_t girq_id, uint32_t bitmap);

/** @brief enable sources in a GIRQ
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param bitmap contains the source bits to enable
 */
void mchp_xec_ecia_girq_src_en_bitmap(uint8_t girq_id, uint32_t bitmap);

/** @brief disable sources in a GIRQ
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @param bitmap contains the source bits to disable
 */
void mchp_xec_ecia_girq_src_dis_bitmap(uint8_t girq_id, uint32_t bitmap);

/** @brief Read GIRQ result register (bit-wise OR of enable and source)
 *
 * @param girq_id is the GIRQ number (8 - 26)
 * @return 32-bit unsigned result register value
 */
uint32_t mchp_xec_ecia_girq_result(uint8_t girq_id);

/** @brief Clear external NVIC input pending status
 *
 * @param nvic_num is 0 to maximum NVIC inputs for the chip.
 */
void mchp_xec_ecia_nvic_clr_pend(uint32_t nvic_num);

/* API using GIRQ parameters encoded with MCHP_XEC_ECIA */

/** @brief enable or disable aggregated GIRQ output
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 * @param enable is flag to indicate enable(1) or disable(0)
 */
void mchp_xec_ecia_info_girq_aggr_en(int ecia_info, uint8_t enable);

/** @brief clear GIRQ latched source status bit
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 */
void mchp_xec_ecia_info_girq_src_clr(int ecia_info);

/** @brief enable a source in a GIRQ
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 */
void mchp_xec_ecia_info_girq_src_en(int ecia_info);

/** @brief disable a source in a GIRQ
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 */
void mchp_xec_ecia_info_girq_src_dis(int ecia_info);

/** @brief Read GIRQ result register (bit-wise OR of enable and source)
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 * @return 32-bit unsigned result register value
 */
uint32_t mchp_xec_ecia_info_girq_result(int ecia_info);

/** @brief Clear external NVIC input pending status given encoded ECIA info.
 *
 * @param ecia_info is GIRQ connection encoded with MCHP_XEC_ECIA
 */
void mchp_xec_ecia_info_nvic_clr_pend(int ecia_info);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_MCHP_XEC_ECIA_H_ */
