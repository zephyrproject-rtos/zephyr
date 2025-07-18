/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <math.h>

#ifndef _ASMLANGUAGE
#include <xc.h>
#endif

#define DT_DRV_COMPAT microchip_dspic33_uart

#define OFFSET_MODE    0x00U
#define OFFSET_STA     0x04U
#define OFFSET_TXREG   0x10U
#define OFFSET_BRG     0x08U
#define OFFSET_RXREG   0x0CU
#define BIT_UTXEN      0x00000020U
#define BIT_URXEN      0x00000010U
#define BIT_UARTEN     0x00008000U
#define BIT_TXBF       0x00100000U
#define BIT_RXBE       0x00020000U
#define FRACTIONAL_BRG 0x8000000U

#define CALCULATE_BRG(baudrate)                                                                    \
	((ceil(((double)(sys_clock_hw_cycles_per_sec())) / ((double)(2U * (baudrate))))))
const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
static struct k_spinlock lock;

struct uart_dspic_config {
	uint32_t base;
	uint32_t baudrate;
};

static void uart_dspic_poll_out(const struct device *dev, unsigned char c)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);
	volatile uint32_t *UxTXREG = (void *)(cfg->base + OFFSET_TXREG);

	/* Wait until there is space in the TX FIFO */
	while ((bool)(void *)((*UxSTA) & BIT_TXBF)) {
		;
	}
	key = k_spin_lock(&lock);
	*UxTXREG = c;
	k_spin_unlock(&lock, key);
}

static int uart_dspic_poll_in(const struct device *dev, unsigned char *c)
{
	k_spinlock_key_t key;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxSTA = (void *)(cfg->base + OFFSET_STA);
	volatile uint32_t *UxRXREG = (void *)(cfg->base + OFFSET_RXREG);
	int ret_val;

	key = k_spin_lock(&lock);
	/* If receiver buffer is empty, return -1 */
	if ((*UxSTA & BIT_RXBE) != 0U) {
		ret_val = -EPERM;
	}

	else {
		*c = *UxRXREG & 0xFF;
		ret_val = 0;
	}

	k_spin_unlock(&lock, key);
	return ret_val;
}

static int uart_dspic_init(const struct device *dev)
{
	LATB = 0x0040UL;
	TRISB = 0x0FBFUL;
	ANSELA = 0x0FFFUL;
	ANSELB = 0x033FUL;

	/* Assign U1TX to RP23 and U1RX to RP24*/
	_RP23R = 9;
	_U1RXR = 24;
	const struct uart_dspic_config *cfg = dev->config;
	volatile uint32_t *UxCON = (void *)(cfg->base + OFFSET_MODE);

	/* Setting the baudrate */
	*UxCON = FRACTIONAL_BRG;
	volatile uint32_t *UxBRG = (void *)(cfg->base + OFFSET_BRG);
	*UxBRG = (uint32_t)CALCULATE_BRG(cfg->baudrate);

	/* Enable UART */
	*UxCON |= BIT_UARTEN | BIT_UTXEN | BIT_URXEN;

	return 0;
}

static const struct uart_driver_api uart_dspic_api = {
	.poll_out = uart_dspic_poll_out,
	.poll_in = uart_dspic_poll_in,
};

#define UART_DSPIC_INIT(inst)                                                                      \
	static const struct uart_dspic_config uart_dspic_config_##inst = {                         \
		.base = DT_REG_ADDR(DT_INST(inst, microchip_dspic33_uart)),                        \
		.baudrate = DT_PROP(DT_INST(inst, microchip_dspic33_uart), current_speed),         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, uart_dspic_init, NULL, NULL, &uart_dspic_config_##inst,        \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_dspic_api);

DT_INST_FOREACH_STATUS_OKAY(UART_DSPIC_INIT)
