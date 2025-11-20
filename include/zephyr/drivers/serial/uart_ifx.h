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

#define CONVERT_UART_PARITY_Z_TO_CY(parity) (parity_lut[(parity)])

#define CONVERT_UART_STOP_BITS_Z_TO_CY(sb)                                                         \
	(((sb) <= UART_CFG_STOP_BITS_2) ? stop_bits_lut[(sb)] : CY_SCB_UART_STOP_BITS_1)

#define CONVERT_UART_DATA_BITS_Z_TO_CY(db)                                                         \
	(((db) <= UART_CFG_DATA_BITS_9) ? data_bits_lut[(db)] : 1U)

#define IFX_UART_BAUD_DIFF(actual, baud)                                                           \
	(((actual) > (baud)) ? (((actual) * 100U - (baud) * 100U) / (baud))                        \
	 : (((baud) * 100U - (actual) * 100U) / (baud)))

#define IFX_UART_DIVIDER(freq, baud, oversample)                                                   \
	(((freq) + (((baud) * (oversample)) / 2U)) / ((baud) * (oversample)))

#define IFX_UART_MEM_WIDTH(cfg)                                                                    \
	(((cfg).dataWidth <= CY_SCB_BYTE_WIDTH) ? (CY_SCB_CTRL_MEM_WIDTH_BYTE)                     \
	 : (CY_SCB_CTRL_MEM_WIDTH_HALFWORD))

#endif /* IFX_UART_H */
