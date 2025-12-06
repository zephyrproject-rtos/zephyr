/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IFX_UART_H
#define IFX_UART_H

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <stdint.h>
#include <stdbool.h>

#include <cy_scb_uart.h>
#include <cy_device_headers.h>
#include <cy_sysint.h>
#include <cy_sysclk.h>
#include <zephyr/logging/log.h>

#define IFX_UART_OVERSAMPLE_MIN              8UL
#define IFX_UART_OVERSAMPLE_MAX              16UL
#define IFX_UART_MAX_BAUD_PERCENT_DIFFERENCE 10U

#if defined(CY_IP_MXSCB_INSTANCES)
#define _IFX_CAT1_SCB_ARRAY_SIZE (CY_IP_MXSCB_INSTANCES)
#elif defined(CY_IP_M0S8SCB_INSTANCES)
#define _IFX_CAT1_SCB_ARRAY_SIZE (CY_IP_M0S8SCB_INSTANCES)
#elif defined(CY_IP_MXS22SCB_INSTANCES)
#define _IFX_CAT1_SCB_ARRAY_SIZE (CY_IP_MXS22SCB_INSTANCES)
#endif /* CY_IP_MXSCB_INSTANCES */

extern const uint8_t data_bits_lut[];
extern const uint8_t stop_bits_lut[];
extern const uint8_t parity_lut[];

static inline uint32_t convert_uart_parity_z_to_cy(uint32_t parity)
{
	switch (parity) {
	case UART_CFG_PARITY_NONE:
		return CY_SCB_UART_PARITY_NONE;
	case UART_CFG_PARITY_ODD:
		return CY_SCB_UART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return CY_SCB_UART_PARITY_EVEN;
	default:
		return CY_SCB_UART_PARITY_NONE;
	}
}

static inline uint32_t convert_uart_stop_bits_z_to_cy(uint32_t sb)
{
	if (sb <= UART_CFG_STOP_BITS_2) {
		return stop_bits_lut[sb];
	}

	LOG_WRN("Invalid stop bits (%u), defaulting to 1 stop bit", sb);
	return CY_SCB_UART_STOP_BITS_1;
}

static inline uint32_t convert_uart_data_bits_z_to_cy(uint32_t db)
{
	if (db <= UART_CFG_DATA_BITS_9) {
		return data_bits_lut[db];
	}

	LOG_WRN("Invalid data bits (%u), defaulting to 1 bit", db);
	return 1U;
}

static inline uint32_t ifx_uart_baud_diff(uint32_t actual, uint32_t baud)
{
	return (actual > baud) ? (((actual - baud) * 100U) / baud)
			       : (((baud - actual) * 100U) / baud);
}

static inline uint32_t ifx_uart_divider(uint32_t freq, uint32_t baud, uint32_t oversample)
{
	return (freq + ((baud * oversample) / 2U)) / (baud * oversample);
}

static inline uint32_t ifx_uart_mem_width(uint32_t data_width)
{
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	return (data_width <= CY_SCB_BYTE_WIDTH) ? CY_SCB_CTRL_MEM_WIDTH_BYTE
						 : CY_SCB_CTRL_MEM_WIDTH_HALFWORD;
#else
	return (data_width <= CY_SCB_BYTE_WIDTH) ? CY_SCB_MEM_WIDTH_BYTE
						 : CY_SCB_MEM_WIDTH_HALFWORD;
#endif
}

#endif /* IFX_UART_H */
