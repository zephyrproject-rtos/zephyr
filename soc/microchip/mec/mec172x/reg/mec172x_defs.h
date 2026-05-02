/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MEC172X_DEFS_H
#define MEC172X_DEFS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Delay register address. Write n to delay for n + 1 microseconds where
 * 0 <= n <= 31.
 * Implementation stalls the CPU fetching instructions including blocking
 * interrupts.
 */
#define MCHP_DELAY_US_ADDR		0x08000000u

/* ARM Cortex-M4 input clock from PLL */
#define MCHP_EC_CLOCK_INPUT_HZ		96000000u

#define MCHP_ACMP_INSTANCES		1
#define MCHP_ACPI_EC_INSTANCES		5
#define MCHP_ACPI_PM1_INSTANCES		1
#define MCHP_ADC_INSTANCES		1
#define MCHP_BCL_INSTANCES		1
#define MCHP_BTMR16_INSTANCES		4
#define MCHP_BTMR32_INSTANCES		2
#define MCHP_CCT_INSTANCES		1
#define MCHP_CTMR_INSTANCES		4
#define MCHP_DMA_INSTANCES		1
#define MCHP_ECIA_INSTANCES		1
#define MCHP_EMI_INSTANCES		3
#define MCHP_HTMR_INSTANCES		2
#define MCHP_I2C_INSTANCES		0
#define MCHP_I2C_SMB_INSTANCES		5
#define MCHP_LED_INSTANCES		4
#define MCHP_MBOX_INSTANCES		1
#define MCHP_OTP_INSTANCES		1
#define MCHP_P80BD_INSTANCES		1
#define MCHP_PECI_INSTANCES		1
#define MCHP_PROCHOT_INSTANCES		1
#define MCHP_PS2_INSTANCES		1
#define MCHP_PWM_INSTANCES		9
#define MCHP_QMSPI_INSTANCES		1
#define MCHP_RCID_INSTANCES		3
#define MCHP_RPMFAN_INSTANCES		2
#define MCHP_RTC_INSTANCES		1
#define MCHP_RTMR_INSTANCES		1
#define MCHP_SPIP_INSTANCES		1
#define MCHP_TACH_INSTANCES		4
#define MCHP_TFDP_INSTANCES		1
#define MCHP_UART_INSTANCES		2
#define MCHP_WDT_INSTANCES		1
#define MCHP_WKTMR_INSTANCES		1

#define MCHP_ACMP_CHANNELS		2
#define MCHP_ADC_CHANNELS		8
#define MCHP_BGPO_GPIO_PINS		2
#define MCHP_DMA_CHANNELS		16
#define MCHP_ESPI_SAF_TAGMAP_MAX	3
#define MCHP_GIRQS			19
#define MCHP_GPIO_PINS			123
#define MCHP_GPIO_PORTS			6
#define MCHP_GPTP_PORTS			6
#define MCHP_I2C_SMB_PORTS		15
#define MCHP_I2C_PORTMAP		0xf7ffu
#define MCHP_QMSPI_PORTS		3
#define MCHP_PS2_PORTS			2
#define MCHP_VCI_IN_PINS		4
#define MCHP_VCI_OUT_PINS		1
#define MCHP_VCI_OVRD_IN_PINS		1

#define SHLU32(v, n)			((uint32_t)(v) << (n))

#ifdef __cplusplus
}
#endif
#endif /* MEC172X_DEFS_H */
