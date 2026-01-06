/*
 * Copyright (C) 2017-2024 Alibaba Group Holding Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_xuantie_ll.h"

int32_t dw_uart_wait_idle(dw_uart_regs_t *uart_base)
{
	uint32_t timecount = 0U;
	int32_t ret = 0;

	while ((uart_base->USR & DW_UART_USR_BUSY_SET) && (timecount < UART_BUSY_TIMEOUT)) {
		timecount++;
	}

	if (timecount >= UART_BUSY_TIMEOUT) {
		ret = -1;
	}

	return ret;
}

int32_t dw_uart_wait_timeout(dw_uart_regs_t *uart_base)
{
	uint32_t timecount = 0U;
	int32_t ret = 0;

	while ((uart_base->LSR & 0x81U) || (uart_base->USR & 0x1U)) {
		uart_base->LSR;
		uart_base->RBR;
		timecount++;

		if (timecount >= UART_BUSY_TIMEOUT) {
			ret = -1;
			break;
		}
	}

	if (ret == 0) {
		ret = dw_uart_wait_idle(uart_base);
	}

	return ret;
}

int32_t dw_uart_config_baudrate(dw_uart_regs_t *uart_base, uint32_t baud, uint32_t uart_freq)
{
	uint32_t divisor;
	int32_t ret = 0;

	ret = dw_uart_wait_timeout(uart_base);

	if (ret == 0) {
		if ((uart_freq / 16) % baud >= (baud / 2)) {
			divisor = (uart_freq / 16) / baud + 1;
		} else {
			divisor = (uart_freq / 16) / baud;
		}

		uart_base->LCR |= DW_UART_LCR_DLAB_EN;

		/* DLL and DLH is lower 8-bits and higher 8-bits of divisor.*/
		uart_base->DLH = (divisor >> 8U) & 0xFFU;
		uart_base->DLL = divisor & 0xFFU;
		/*
		 * The DLAB must be cleared after the baudrate is setted
		 * to access other registers.
		 */
		uart_base->LCR &= (~DW_UART_LCR_DLAB_EN);
	}

	return ret;
}

int32_t dw_uart_config_stop_bits(dw_uart_regs_t *uart_base, uint32_t stop_bits)
{
	int32_t ret;

	ret = dw_uart_wait_timeout(uart_base);

	if (ret == 0) {

		/* when data length is 5 bits,
		 * use dw_uart_config_stop_bits_2 will be 1.5 stop bits
		 */
		if (stop_bits == 1U) {
			dw_uart_config_stop_bits_1(uart_base);
		} else if (stop_bits == 2U) {
			dw_uart_config_stop_bits_2(uart_base);
		}
	}

	return ret;
}

int32_t dw_uart_config_parity_none(dw_uart_regs_t *uart_base)
{
	int32_t ret;

	ret = dw_uart_wait_timeout(uart_base);

	if (ret == 0) {
		uart_base->LCR &= (~DW_UART_LCR_PEN_EN);
	}

	return ret;
}

int32_t dw_uart_config_parity_odd(dw_uart_regs_t *uart_base)
{
	int32_t ret;

	ret = dw_uart_wait_timeout(uart_base);

	if (ret == 0) {
		uart_base->LCR |= DW_UART_LCR_PEN_EN;
		uart_base->LCR &= ~(DW_UART_LCR_EPS_EN);
	}

	return ret;
}

int32_t dw_uart_config_parity_even(dw_uart_regs_t *uart_base)
{
	int32_t ret;

	ret = dw_uart_wait_timeout(uart_base);

	if (ret == 0) {
		uart_base->LCR |= DW_UART_LCR_PEN_EN;
		uart_base->LCR |= DW_UART_LCR_EPS_EN;
	}

	return ret;
}

int32_t dw_uart_config_data_bits(dw_uart_regs_t *uart_base, uint32_t data_bits)
{
	int32_t ret;

	ret = dw_uart_wait_timeout(uart_base);

	uart_base->LCR &= 0xFCU;
	uart_base->LCR |= (data_bits - 5U);

	return ret;
}
