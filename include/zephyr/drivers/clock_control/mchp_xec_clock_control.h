/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCHP_XEC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCHP_XEC_H_

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>

/** @brief Structure for interfacing with clock control API */
union clock_mchp_xec_subsys {
	/** @brief Raw 32-bit value */
	uint32_t val;

	/** @brief bitfield view */
	struct {
		/** @brief Encoded PCR data */
		uint32_t pcr_data: 24;
		/** @brief Clock ID */
		uint32_t bus: 8;
	} bits;
};

/* @brief set or clear Microchip XEC peripheral PCR sleep enable
 *
 * @param slp_idx is the peripheral's sleep enable zero based index (0 through 4)
 * @param slp_pos is the bit position in the 32-bit sleep enable register
 * @param slp_en is 0 to clear(disable) clock gating and non-zero to enable clock gating
 * @return 0 success or -EINVAL on error
 */
int z_mchp_xec_pcr_periph_sleep(uint8_t slp_idx, uint8_t slp_pos, uint8_t slp_en);

/* @brief Reset a single peripheral similar to chip reset
 *
 * @param rst_idx is the peripheral's reset enable register index (0 through 4).
 * @param rst_pos is the bit position in the 32-bit sleep enable register
 * @return 0 success or -EINVAL on error
 * @note This routine lock and unlocks IRQs to safely touch multiple PCR registers. The reset
 * index and bit position are the same as the peripheral's sleep index and bit position.
 */
int z_mchp_xec_pcr_periph_reset(uint8_t rst_idx, uint8_t rst_pos);

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_LPC11U6X_CLOCK_CONTROL_H_ */
