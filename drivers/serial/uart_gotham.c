/*
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief UART driver for the Gotham SoC
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <uart.h>
#include <board.h>

#define TX_AVAIL	(0x00FF0000)
#define RX_EMPTY	(0xFF000000)
#define DATA_MASK	0xFF
#define DATA_VALID	(1 << 16)
#define DATA_LEN8	(0x7F)
#define NO_PARITY	0x00
#define STOP_BIT1	(1 << 16)

/* device config */
struct uart_gotham_config {
        struct uart_device_config uconf;
        /* Baud rate */
        u32_t baudrate;
};

struct uart_gotham_regs_t {
	u32_t data;
	u32_t status;
	u32_t clk_div;
	u32_t cfg;
};

#define DEV_CFG(dev)                                            \
        ((struct uart_gotham_config *) (dev)->config->config_info)
#define DEV_UART(dev)                                           \
        ((struct uart_gotham_regs_t *)(DEV_CFG(dev))->uconf.base)

/**
 * @brief Output a character in polled mode.
 *
 * Writes data to tx register if transmitter is not full.
 *
 * @param dev UART device struct
 * @param c Character to send
 *
 * @return Sent character
 */
static unsigned char uart_gotham_poll_out(struct device *dev,
					 unsigned char c)
{
	volatile struct uart_gotham_regs_t *uart = DEV_UART(dev);
	
	/* Wait till TX_FIFO gets empty */
	while (uart->status & TX_AVAIL);

	uart->data = (int)c;

	return c;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty or
 *           the data is not valid.
 */
static int uart_gotham_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct uart_gotham_regs_t *uart = DEV_UART(dev);
	
	u32_t status = uart->status;
	u32_t data = uart->data;

	/* Check for FIFO empty */
	if (!(status & RX_EMPTY))
		return -1;

	/* Check for data validity */
	if (!(data & DATA_VALID))
		return -1;

	*c = (unsigned char)(data & DATA_MASK);

	return 0;
}

static int uart_gotham_init(struct device *dev)
{
	volatile struct uart_gotham_config *cfg = DEV_CFG(dev);
	volatile struct uart_gotham_regs_t *uart = DEV_UART(dev);

	/* Config UART */
	uart->clk_div = ((GOTHAM_FCLK_RATE / (cfg->baudrate * 8)) - 1) & 0xFFFFF;
	uart->cfg = (DATA_LEN8 + NO_PARITY + STOP_BIT1);

	return 0;
}

static const struct uart_driver_api uart_gotham_driver_api = {
	.poll_in          = uart_gotham_poll_in,
	.poll_out         = uart_gotham_poll_out,
	.err_check        = NULL,
};

static const struct uart_gotham_config uart_gotham_dev_cfg = {
	.uconf = {
		.base = (void *)GOTHAM_UART_BASE_ADDR,
		.sys_clk_freq = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	},
	.baudrate = GOTHAM_UART_BAUDRATE,
};

DEVICE_AND_API_INIT(uart_gotham, "uart0",
		    uart_gotham_init,
		    NULL, &uart_gotham_dev_cfg,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&uart_gotham_driver_api);
