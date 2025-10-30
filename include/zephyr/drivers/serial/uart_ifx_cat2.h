/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IFX_CAT2_UART_H
#define IFX_CAT2_UART_H

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <stdint.h>
#include <stdbool.h>

#include <cy_scb_uart.h>
#include <cy_device_headers.h>
#include <cy_sysint.h>
#include <cy_sysclk.h>

#define IFX_CAT2_UART_OVERSAMPLE_MIN              8UL
#define IFX_CAT2_UART_OVERSAMPLE_MAX              16UL
#define IFX_CAT2_UART_MAX_BAUD_PERCENT_DIFFERENCE 10

struct ifx_cat2_uart_data {
	struct uart_config cfg;
	struct ifx_cat2_resource_inst hw_resource;
	struct ifx_cat2_clock clock;
#if CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
	bool cts_enabled;
	cy_stc_scb_uart_context_t context;
	cy_stc_scb_uart_config_t scb_config;
};

struct ifx_cat2_uart_config {
	const struct pinctrl_dev_config *pcfg;
	CySCB_Type *reg_addr;
	struct uart_config dt_cfg;
	const cy_stc_scb_uart_config_t *uart_def_conf;
	uint16_t irq_num;
	uint8_t irq_priority;
};

typedef void (*ifx_cat2_uart_event_callback_t)(void *callback_arg);

#if defined(CY_IP_MXSCB_INSTANCES)
#define _IFX_CAT2_SCB_ARRAY_SIZE (CY_IP_MXSCB_INSTANCES)
#elif defined(CY_IP_M0S8SCB_INSTANCES)
#define _IFX_CAT1_SCB_ARRAY_SIZE (CY_IP_M0S8SCB_INSTANCES)
#endif

#define CONVERT_UART_PARITY_Z_TO_CY(parity) (parity_lut[(parity)])

#define CONVERT_UART_STOP_BITS_Z_TO_CY(sb)                                                         \
	(((sb) <= UART_CFG_STOP_BITS_2) ? stop_bits_lut[(sb)] : CY_SCB_UART_STOP_BITS_1)

#define CONVERT_UART_DATA_BITS_Z_TO_CY(db)                                                         \
	(((db) <= UART_CFG_DATA_BITS_9) ? data_bits_lut[(db)] : 1)

#define IFX_UART_BAUD_DIFF(actual_baud, baudrate)                                                  \
	(((actual_baud) > (baudrate)) ? (((actual_baud) * 100u - (baudrate) * 100u) / (baudrate))  \
				      : (((baudrate) * 100u - (actual_baud) * 100u) / (baudrate)))

#define IFX_UART_DIVIDER(freq, baud, oversample)                                                   \
	(((freq) + (((baud) * (oversample)) / 2)) / ((baud) * (oversample)))

#define IFX_UART_MEM_WIDTH(cfg)                                                                    \
	(((cfg).dataWidth <= CY_SCB_BYTE_WIDTH) ? CY_SCB_CTRL_MEM_WIDTH_BYTE                       \
						: CY_SCB_CTRL_MEM_WIDTH_HALFWORD)

#define IFX_UART_SCB_CTRL_VALUE(cfg, oversample)                                                   \
	(_BOOL2FLD(SCB_CTRL_ADDR_ACCEPT, (cfg).acceptAddrInFifo) |                                 \
	 _BOOL2FLD(SCB_CTRL_MEM_WIDTH, IFX_UART_MEM_WIDTH(cfg)) |                                  \
	 _VAL2FLD(SCB_CTRL_OVS, ((oversample) - 1)) |                                              \
	 _VAL2FLD(SCB_CTRL_MODE, CY_SCB_CTRL_MODE_UART))

#define IFX_UART_TX_MASK(mode)                                                                     \
	(((mode) == CY_SCB_UART_STANDARD) ? (CY_SCB_UART_TX_OVERFLOW | CY_SCB_UART_TRANSMIT_ERR)   \
	 : ((mode) == CY_SCB_UART_SMARTCARD)                                                       \
		 ? (CY_SCB_UART_TX_OVERFLOW | CY_SCB_TX_INTR_UART_NACK |                           \
		    CY_SCB_TX_INTR_UART_ARB_LOST | CY_SCB_UART_TRANSMIT_ERR)                       \
		 : (CY_SCB_UART_TX_OVERFLOW | CY_SCB_TX_INTR_UART_ARB_LOST |                       \
		    CY_SCB_UART_TRANSMIT_ERR))

#endif /* IFX_CAT2_UART_H */
