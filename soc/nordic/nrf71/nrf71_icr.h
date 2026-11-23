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
	NRF71_ICR_MACADDR0_LOW,
	NRF71_ICR_MACADDR0_HIGH,
	NRF71_ICR_MACADDR1_LOW,
	NRF71_ICR_MACADDR1_HIGH,
	NRF71_ICR_XTALCOMP_OFFSET,
	NRF71_ICR_XTALCOMP_LOWTEMPTHRESHOLD,
	NRF71_ICR_XTALCOMP_LOWTEMPADJUST,
	NRF71_ICR_REGISTER_COUNT,
};

/**
 * @brief Source selected by an nRF71 ICR read.
 */
enum nrf71_icr_source {
	NRF71_ICR_SOURCE_FICR,
	NRF71_ICR_SOURCE_UICR,
};

/**
 * @brief Read an nRF71 configuration register with fallback.
 *
 * For secure builds, the UICR is read. If the UICR register is erased,
 * the corresponding FICR register is read. If both registers are erased, the
 * input value of @p reg_value is left unchanged so that it acts as the default
 * value. For non-secure builds, attempting to read UICR is skipped since the
 * calling domain does not have read access.
 *
 * @param reg ICR register to read.
 * @param[in,out] reg_value Pointer to the default value. Updated when
 *							a valid value is read from the UICR or FICR.
 *
 * @retval NRF71_ICR_SOURCE_UICR if a valid value was read from UICR.
 * @retval NRF71_ICR_SOURCE_FICR if a valid value was read from FICR.
 * @retval -ENODATA if both registers values were 0xFFFFFFFF.
 * @retval -EINVAL if @p reg is unsupported or @p reg_value is @c NULL.
 */
int nrf71_icr_read(enum nrf71_icr_register reg, uint32_t *reg_value);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_NORDIC_NRF71_NRF71_ICR_H_ */
