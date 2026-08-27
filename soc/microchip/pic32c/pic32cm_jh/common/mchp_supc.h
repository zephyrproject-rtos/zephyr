/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_MICROCHIP_PIC32C_PIC32CM_JH_COMMON_MCHP_SUPC_H_
#define ZEPHYR_SOC_MICROCHIP_PIC32C_PIC32CM_JH_COMMON_MCHP_SUPC_H_
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SUPC VREF output voltage selection */
enum vref_sel {
	SUPC_VREF_1V024 = 0x00,
	SUPC_VREF_2V048 = 0x02,
	SUPC_VREF_4V096 = 0x03
};

/**
 * @brief Enable the SUPC reference voltage output for adc input channel.
 *
 * This function enables the internal reference voltage for adc input channel.
 *
 * @retval 0 If the reference voltage was enabled successfully.
 */
int supc_mchp_vref_enable(void);

/**
 * @brief Disable the SUPC reference voltage output for adc input channel.
 *
 * This function disables the internal reference voltage for adc input channel.
 *
 * @retval 0 If the reference voltage was disabled successfully.
 */
int supc_mchp_vref_disable(void);

/**
 * @brief Configure the SUPC reference voltage level.
 *
 * This function selects the reference voltage level for the SUPC VREF
 * peripheral.
 *
 * @param vref_sel Reference voltage selection value. Checkout dt-bindings header
 *              for enum values
 *
 * @retval 0 If the reference voltage level was configured successfully.
 */
int supc_mchp_vref_set_voltage(enum vref_sel vref_sel);

/**
 * @brief Get the SUPC reference voltage level.
 *
 * This function retrieves the currently configured reference voltage level
 * for the SUPC VREF peripheral.
 *
 * @return Configured reference voltage in volts.
 */
enum vref_sel supc_mchp_vref_get_voltage(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_MICROCHIP_PIC32C_PIC32CM_JH_COMMON_MCHP_SUPC_H_ */
