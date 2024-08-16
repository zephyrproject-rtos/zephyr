/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT quicklogic_usbserialport_s3b

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <eoss3_dev.h>

#include "uart_ql_usbserialport_s3b.h"

/*
 * code is a modified version of usbserial driver from https://github.com/QuickLogic-Corp/qorc-sdk
 * freertos_gateware/src/eoss3_hal_fpga_usbserial.c
 * freertos_gateware/inc/eoss3_hal_fpga_usbserial.h
 */

volatile struct fpga_usbserial_regs *usbserial_regs
	= (struct fpga_usbserial_regs *)(FPGA_PERIPH_BASE);

static uint32_t usbserial_tx_fifo_status(void)
{
	return usbserial_regs->m2u_fifo_flags;
}

static bool usbserial_tx_fifo_full(void)
{
	return usbserial_tx_fifo_status() == USBSERIAL_TX_FIFO_FULL;
}

static uint32_t usbserial_rx_fifo_status(void)
{
	return usbserial_regs->u2m_fifo_flags;
}

static bool usbserial_rx_fifo_empty(void)
{
	return usbserial_rx_fifo_status() == USBSERIAL_RX_FIFO_EMPTY;
}

/**
 * @brief Output a character in polled mode.
 *
 * Writes data to tx register. Waits for space if transmitter is full.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_usbserial_poll_out(const struct device *dev, unsigned char c)
{
	/* Wait for room in Tx FIFO */
	while (usbserial_tx_fifo_full()) {
		;
	}
	usbserial_regs->wdata = c;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_usbserial_poll_in(const struct device *dev, unsigned char *c)
{
	if (usbserial_rx_fifo_empty()) {
		return -1;
	}

	*c = usbserial_regs->rdata;
	return 0;
}

static const struct uart_driver_api uart_usbserial_driver_api = {
	.poll_in		= uart_usbserial_poll_in,
	.poll_out		= uart_usbserial_poll_out,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_SERIAL_INIT_PRIORITY,
		      (void *)&uart_usbserial_driver_api);
