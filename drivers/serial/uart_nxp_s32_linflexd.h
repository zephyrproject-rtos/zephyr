/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_S32_LINFLEXD_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_S32_LINFLEXD_H_

struct uart_nxp_s32_config {
	uint32_t instance;
	LINFLEXD_Type *base;
	const struct pinctrl_dev_config *pincfg;
	Linflexd_Uart_Ip_UserConfigType hw_cfg;
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
struct uart_nxp_s32_int {
	bool tx_fifo_busy;
	bool rx_fifo_busy;
	bool irq_tx_enable;
	bool irq_rx_enable;
	uint8_t rx_fifo_data;
};
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
struct uart_nxp_s32_data {
	struct uart_nxp_s32_int int_data;
	uart_irq_callback_user_data_t callback;
	void *cb_data;
};
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

extern Linflexd_Uart_Ip_StateStructureType
Linflexd_Uart_Ip_apStateStructure[LINFLEXD_UART_IP_NUMBER_OF_INSTANCES];

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_S32_LINFLEXD_H_ */
