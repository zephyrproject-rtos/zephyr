/*
 * Copyright (c) 2017 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief UART driver for Atmel SAM MCU family.
 *
 * Note:
 * - Error handling is not implemented.
 * - The driver works only in polling mode, interrupt mode is not implemented.
 */

#include <errno.h>
#include <misc/__assert.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <uart.h>

/*
 * Verify Kconfig configuration
 */

#if CONFIG_UART_SAM_PORT_0 == 1

#if CONFIG_UART_SAM_PORT_0_BAUD_RATE == 0
#error "CONFIG_UART_SAM_PORT_0_BAUD_RATE has to be bigger than 0"
#endif

#endif

#if CONFIG_UART_SAM_PORT_1 == 1

#if CONFIG_UART_SAM_PORT_1_BAUD_RATE == 0
#error "CONFIG_UART_SAM_PORT_1_BAUD_RATE has to be bigger than 0"
#endif

#endif

#if CONFIG_UART_SAM_PORT_2 == 1

#if CONFIG_UART_SAM_PORT_2_BAUD_RATE == 0
#error "CONFIG_UART_SAM_PORT_2_BAUD_RATE has to be bigger than 0"
#endif

#endif

#if CONFIG_UART_SAM_PORT_3 == 1

#if CONFIG_UART_SAM_PORT_3_BAUD_RATE == 0
#error "CONFIG_UART_SAM_PORT_3_BAUD_RATE has to be bigger than 0"
#endif

#endif

#if CONFIG_UART_SAM_PORT_4 == 1

#if CONFIG_UART_SAM_PORT_4_BAUD_RATE == 0
#error "CONFIG_UART_SAM_PORT_4_BAUD_RATE has to be bigger than 0"
#endif

#endif

/* Device constant configuration parameters */
struct uart_sam_dev_cfg {
	Uart *regs;
	u32_t periph_id;
	struct soc_gpio_pin pin_rx;
	struct soc_gpio_pin pin_tx;
};

/* Device run time data */
struct uart_sam_dev_data {
	u32_t baud_rate;
};

#define DEV_CFG(dev) \
	((const struct uart_sam_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_sam_dev_data *const)(dev)->driver_data)


static int baudrate_set(Uart *const uart, u32_t baudrate,
			u32_t mck_freq_hz);


static int uart_sam_init(struct device *dev)
{
	int retval;
	const struct uart_sam_dev_cfg *const cfg = DEV_CFG(dev);
	struct uart_sam_dev_data *const dev_data = DEV_DATA(dev);
	Uart *const uart = cfg->regs;

	/* Enable UART clock in PMC */
	soc_pmc_peripheral_enable(cfg->periph_id);

	/* Connect pins to the peripheral */
	soc_gpio_configure(&cfg->pin_rx);
	soc_gpio_configure(&cfg->pin_tx);

	/* Reset and disable UART */
	uart->UART_CR =   UART_CR_RSTRX | UART_CR_RSTTX
			| UART_CR_RXDIS | UART_CR_TXDIS | UART_CR_RSTSTA;

	/* Disable Interrupts */
	uart->UART_IDR = 0xFFFFFFFF;

	/* 8 bits of data, no parity, 1 stop bit in normal mode,  baud rate
	 * driven by the peripheral clock, UART does not filter the receive line
	 */
	uart->UART_MR =   UART_MR_PAR_NO
			| UART_MR_CHMODE_NORMAL;

	/* Set baud rate */
	retval = baudrate_set(uart, dev_data->baud_rate,
			      SOC_ATMEL_SAM_MCK_FREQ_HZ);
	if (retval != 0) {
		return retval;
	};

	/* Enable receiver and transmitter */
	uart->UART_CR = UART_CR_RXEN | UART_CR_TXEN;

	return 0;
}

static int uart_sam_poll_in(struct device *dev, unsigned char *c)
{
	Uart *const uart = DEV_CFG(dev)->regs;

	if (!(uart->UART_SR & UART_SR_RXRDY)) {
		return -EBUSY;
	}

	/* got a character */
	*c = (unsigned char)uart->UART_RHR;

	return 0;
}

static unsigned char uart_sam_poll_out(struct device *dev, unsigned char c)
{
	Uart *const uart = DEV_CFG(dev)->regs;

	/* Wait for transmitter to be ready */
	while (!(uart->UART_SR & UART_SR_TXRDY))
		;

	/* send a character */
	uart->UART_THR = (u32_t)c;
	return c;
}

static int baudrate_set(Uart *const uart, u32_t baudrate,
			u32_t mck_freq_hz)
{
	u32_t divisor;

	__ASSERT(baudrate,
		 "baud rate has to be bigger than 0");
	__ASSERT(mck_freq_hz/16 >= baudrate,
		 "MCK frequency is too small to set required baud rate");

	divisor = mck_freq_hz / 16 / baudrate;

	if (divisor > 0xFFFF) {
		return -EINVAL;
	};

	uart->UART_BRGR = UART_BRGR_CD(divisor);

	return 0;
}

static const struct uart_driver_api uart_sam_driver_api = {
	.poll_in = uart_sam_poll_in,
	.poll_out = uart_sam_poll_out,
};

/* UART0 */

#ifdef CONFIG_UART_SAM_PORT_0
static const struct uart_sam_dev_cfg uart0_sam_config = {
	.regs = UART0,
	.periph_id = ID_UART0,
	.pin_rx = PIN_UART0_RXD,
	.pin_tx = PIN_UART0_TXD,
};

static struct uart_sam_dev_data uart0_sam_data = {
	.baud_rate = CONFIG_UART_SAM_PORT_0_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart0_sam, CONFIG_UART_SAM_PORT_0_NAME, &uart_sam_init,
		    &uart0_sam_data, &uart0_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);
#endif

/* UART1 */

#ifdef CONFIG_UART_SAM_PORT_1
static const struct uart_sam_dev_cfg uart1_sam_config = {
	.regs = UART1,
	.periph_id = ID_UART1,
	.pin_rx = PIN_UART1_RXD,
	.pin_tx = PIN_UART1_TXD,
};

static struct uart_sam_dev_data uart1_sam_data = {
	.baud_rate = CONFIG_UART_SAM_PORT_1_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart1_sam, CONFIG_UART_SAM_PORT_1_NAME, &uart_sam_init,
		    &uart1_sam_data, &uart1_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);
#endif

/* UART2 */

#ifdef CONFIG_UART_SAM_PORT_2
static const struct uart_sam_dev_cfg uart2_sam_config = {
	.regs = UART2,
	.periph_id = ID_UART2,
	.pin_rx = PIN_UART2_RXD,
	.pin_tx = PIN_UART2_TXD,
};

static struct uart_sam_dev_data uart2_sam_data = {
	.baud_rate = CONFIG_UART_SAM_PORT_2_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart2_sam, CONFIG_UART_SAM_PORT_2_NAME, &uart_sam_init,
		    &uart2_sam_data, &uart2_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);
#endif

/* UART3 */

#ifdef CONFIG_UART_SAM_PORT_3
static const struct uart_sam_dev_cfg uart3_sam_config = {
	.regs = UART3,
	.periph_id = ID_UART3,
	.pin_rx = PIN_UART3_RXD,
	.pin_tx = PIN_UART3_TXD,
};

static struct uart_sam_dev_data uart3_sam_data = {
	.baud_rate = CONFIG_UART_SAM_PORT_3_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart3_sam, CONFIG_UART_SAM_PORT_3_NAME, &uart_sam_init,
		    &uart3_sam_data, &uart3_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);
#endif

/* UART4 */

#ifdef CONFIG_UART_SAM_PORT_4
static const struct uart_sam_dev_cfg uart4_sam_config = {
	.regs = UART4,
	.periph_id = ID_UART4,
	.pin_rx = PIN_UART4_RXD,
	.pin_tx = PIN_UART4_TXD,
};

static struct uart_sam_dev_data uart4_sam_data = {
	.baud_rate = CONFIG_UART_SAM_PORT_4_BAUD_RATE,
};

DEVICE_AND_API_INIT(uart4_sam, CONFIG_UART_SAM_PORT_4_NAME, &uart_sam_init,
		    &uart4_sam_data, &uart4_sam_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uart_sam_driver_api);
#endif
