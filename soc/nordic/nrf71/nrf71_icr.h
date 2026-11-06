/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_NRF71_NRF71_ICR_H_
#define ZEPHYR_SOC_NORDIC_NRF71_NRF71_ICR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers that have corresponding nRF71 UICR and FICR locations.
 */
enum nrf71_icr_register {
	MACADDR0_LOW,
	MACADDR0_HIGH,
	MACADDR1_LOW,
	MACADDR1_HIGH,
	XTALCOMP_OFFSET,
	XTALCOMP_LOWTEMPTHRESHOLD,
	XTALCOMP_LOWTEMPADJUST,
	ICR_REGISTER_COUNT,
};

/**
 * @brief Source selected by an nRF71 ICR read.
 */
enum nrf71_icr_source {
	NRF71_ICR_SOURCE_FICR,
	NRF71_ICR_SOURCE_UICR,
	NRF71_ICR_SOURCE_DEFAULT,
};

/**
 * @brief Read an nRF71 configuration register with fallback.
 *
 * Read the requested register from UICR. If the UICR register is erased, read
 * the corresponding register from FICR. If both registers are erased,
 * @p reg_value is left unchanged so that its input value acts as the default.
 *
 * @note The calling domain must have read access to UICR and FICR.
 *
 * @param reg ICR register to read.
 * @param[in,out] reg_value Pointer to the default value. Updated when
 *							a valid value is read from the UICR or FICR.
 *
 * @retval NRF71_ICR_SOURCE_UICR if a valid value was read from UICR.
 * @retval NRF71_ICR_SOURCE_FICR if a valid value was read from FICR.
 * @retval NRF71_ICR_SOURCE_DEFAULT if both registers values were 0xFFFFFFFF.
 * @retval -EINVAL if @p reg is unsupported or @p reg_value is @c NULL.
 */
int nrf71_icr_read(enum nrf71_icr_register reg, uint32_t *reg_value);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_NORDIC_NRF71_NRF71_ICR_H_ */
