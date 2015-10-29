/* stellarisUartDrv.c - Stellaris UART driver */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief Driver for Stellaris UART
 *
 * Driver for Stellaris UART found namely on TI LM3S6965 board. It is similar to
 * an 16550 in functionality, but is not register-compatible.
 *
 * There is only support for poll-mode, so it can only be used with the printk
 * and STDOUT_CONSOLE APIs.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <board.h>
#include <uart.h>
#include <sections.h>

/* definitions */

/* Stellaris UART module */
struct _uart {
	uint32_t dr;
	union {
		uint32_t _sr;
		uint32_t _cr;
	} u1;
	uint8_t _res1[0x010];
	uint32_t fr;
	uint8_t _res2[0x04];
	uint32_t ilpr;
	uint32_t ibrd;
	uint32_t fbrd;
	uint32_t lcrh;
	uint32_t ctl;
	uint32_t ifls;
	uint32_t im;
	uint32_t ris;
	uint32_t mis;
	uint32_t icr;
	uint8_t _res3[0xf8c];

	uint32_t peripd_id4;
	uint32_t peripd_id5;
	uint32_t peripd_id6;
	uint32_t peripd_id7;
	uint32_t peripd_id0;
	uint32_t peripd_id1;
	uint32_t peripd_id2;
	uint32_t peripd_id3;

	uint32_t p_cell_id0;
	uint32_t p_cell_id1;
	uint32_t p_cell_id2;
	uint32_t p_cell_id3;
};

/* convenience defines */

#define DEV_CFG(dev) \
	((struct uart_device_config * const)(dev)->config->config_info)
#define UART_STRUCT(dev) \
	((volatile struct _uart *)(DEV_CFG(dev))->base)

/* registers */
#define UARTDR(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x000)))
#define UARTSR(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x004)))
#define UARTCR(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x004)))
#define UARTFR(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x018)))
#define UARTILPR(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x020)))
#define UARTIBRD(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x024)))
#define UARTFBRD(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x028)))
#define UARTLCRH(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x02C)))
#define UARTCTL(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x030)))
#define UARTIFLS(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x034)))
#define UARTIM(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x038)))
#define UARTRIS(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x03C)))
#define UARTMIS(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x040)))
#define UARTICR(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0x044)))

/* ID registers: UARTPID = UARTPeriphID, UARTPCID = UARTPCellId */
#define UARTPID4(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFD0)))
#define UARTPID5(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFD4)))
#define UARTPID6(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFD8)))
#define UARTPID7(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFDC)))
#define UARTPID0(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFE0)))
#define UARTPID1(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFE4)))
#define UARTPID2(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFE8)))
#define UARTPID3(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFEC)))
#define UARTPCID0(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFF0)))
#define UARTPCID1(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFF4)))
#define UARTPCID2(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFF8)))
#define UARTPCID3(dev) (*((volatile uint32_t *)(DEV_CFG(dev)->base + 0xFFC)))

/* muxed UART registers */
#define sr u1._sr /* Read: receive status */
#define cr u1._cr /* Write: receive error clear */

/* bits */
#define UARTFR_BUSY 0x00000008
#define UARTFR_RXFE 0x00000010
#define UARTFR_TXFF 0x00000020
#define UARTFR_RXFF 0x00000040
#define UARTFR_TXFE 0x00000080

#define UARTLCRH_FEN 0x00000010
#define UARTLCRH_WLEN 0x00000060

#define UARTCTL_UARTEN 0x00000001
#define UARTCTL_LBE 0x00000800
#define UARTCTL_TXEN 0x00000100
#define UARTCTL_RXEN 0x00000200

#define UARTTIM_RXIM 0x00000010
#define UARTTIM_TXIM 0x00000020
#define UARTTIM_RTIM 0x00000040
#define UARTTIM_FEIM 0x00000080
#define UARTTIM_PEIM 0x00000100
#define UARTTIM_BEIM 0x00000200
#define UARTTIM_OEIM 0x00000400

#define UARTMIS_RXMIS 0x00000010
#define UARTMIS_TXMIS 0x00000020

static struct uart_driver_api stellaris_uart_driver_api;

/**
 * @brief Set the baud rate
 *
 * This routine set the given baud rate for the UART.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param baudrate Baud rate
 * @param sys_clk_freq_hz System clock frequency in Hz
 *
 * @return N/A
 */
static void baudrate_set(struct device *dev,
			 uint32_t baudrate, uint32_t sys_clk_freq_hz)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	uint32_t brdi, brdf, div, rem;

	/* upon reset, the system clock uses the intenal OSC @ 12MHz */

	div = (16 * baudrate);
	rem = sys_clk_freq_hz % div;

	/*
	 * floating part of baud rate (LM3S6965 p.433), equivalent to
	 * [float part of (SYSCLK / div)] * 64 + 0.5
	 */
	brdf = ((((rem * 64) << 1) / div) + 1) >> 1;

	/* integer part of baud rate (LM3S6965 p.433) */
	brdi = sys_clk_freq_hz / div;

	/*
	 * those registers are 32-bit, but the reserved bits should be
	 * preserved
	 */
	uart->ibrd = (uint16_t)(brdi & 0xffff); /* 16 bits */
	uart->fbrd = (uint8_t)(brdf & 0x3f);    /* 6 bits */
}

/**
 * @brief Enable the UART
 *
 * This routine enables the given UART.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static inline void enable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->ctl |= UARTCTL_UARTEN;
}

/**
 * @brief Disable the UART
 *
 * This routine disables the given UART.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static inline void disable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->ctl &= ~UARTCTL_UARTEN;

	/* ensure transmissions are complete */
	while (uart->fr & UARTFR_BUSY)
		;

	/* flush the FIFOs by disabling them */
	uart->lcrh &= ~UARTLCRH_FEN;
}

/*
 * no stick parity
 * 8-bit frame
 * FIFOs disabled
 * one stop bit
 * parity disabled
 * send break off
 */
#define LINE_CONTROL_DEFAULTS UARTLCRH_WLEN

/**
 * @brief Set the default UART line controls
 *
 * This routine sets the given UART's line controls to their default settings.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static inline void line_control_defaults_set(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->lcrh = LINE_CONTROL_DEFAULTS;
}

/**
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
void stellaris_uart_port_init(struct device *dev,
			      const struct uart_init_info * const init_info)
{
	struct uart_device_config * const dev_cfg = DEV_CFG(dev);

	dev_cfg->irq_pri = init_info->irq_pri;

	disable(dev);
	baudrate_set(dev, init_info->baud_rate, init_info->sys_clk_freq);
	line_control_defaults_set(dev);
	enable(dev);

	dev->driver_api = &stellaris_uart_driver_api;
}

/**
 * @brief Get the UART transmit ready status
 *
 * This routine returns the given UART's transmit ready status.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 0 if ready to transmit, 1 otherwise
 */
static int poll_tx_ready(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	return (uart->fr & UARTFR_TXFE);
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */

static int stellaris_uart_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	if (uart->fr & UARTFR_RXFE)
		return (-1);

	/* got a character */
	*c = (unsigned char)uart->dr;

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param c Character to send
 *
 * @return Sent character
 */
static unsigned char stellaris_uart_poll_out(struct device *dev,
					     unsigned char c)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	while (!poll_tx_ready(dev))
		;

	/* send a character */
	uart->dr = (uint32_t)c;
	return c;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param tx_data Data to transmit
 * @param len Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int stellaris_uart_fifo_fill(struct device *dev, const uint8_t *tx_data,
				    int len)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	uint8_t num_tx = 0;

	while ((len - num_tx > 0) && ((uart->fr & UARTFR_TXFF) == 0)) {
		uart->dr = (uint32_t)tx_data[num_tx++];
	}

	return (int)num_tx;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param rx_data Pointer to data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int stellaris_uart_fifo_read(struct device *dev, uint8_t *rx_data,
				    const int size)
{
	volatile struct _uart *uart = UART_STRUCT(dev);
	uint8_t num_rx = 0;

	while ((size - num_rx > 0) && ((uart->fr & UARTFR_RXFE) == 0)) {
		rx_data[num_rx++] = (uint8_t)uart->dr;
	}

	return num_rx;
}

/**
 * @brief Enable TX interrupt
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void stellaris_uart_irq_tx_enable(struct device *dev)
{
	static uint8_t first_time =
		1;	   /* used to allow the first transmission */
	uint32_t saved_ctl;  /* saved UARTCTL (control) register */
	uint32_t saved_ibrd; /* saved UARTIBRD (integer baud rate) register */
	uint32_t saved_fbrd; /* saved UARTFBRD (fractional baud rate) register
				*/
	volatile struct _uart *uart = UART_STRUCT(dev);

	if (first_time) {
		/*
		 * The Tx interrupt will not be set when transmission is first
		 * enabled.
		 * A character has to be transmitted before Tx interrupts will
		 * work,
		 * so send one via loopback mode.
		 */
		first_time = 0;

		/* save current control and baud rate settings */
		saved_ctl = uart->ctl;
		saved_ibrd = uart->ibrd;
		saved_fbrd = uart->fbrd;

		/* send a character with default settings via loopback */
		disable(dev);
		uart->fbrd = 0;
		uart->ibrd = 1;
		uart->lcrh = 0;
		uart->ctl = (UARTCTL_UARTEN | UARTCTL_TXEN | UARTCTL_LBE);
		uart->dr = 0;

		while (uart->fr & UARTFR_BUSY)
			;

		/* restore control and baud rate settings */
		disable(dev);
		uart->ibrd = saved_ibrd;
		uart->fbrd = saved_fbrd;
		line_control_defaults_set(dev);
		uart->ctl = saved_ctl;
	}

	uart->im |= UARTTIM_TXIM;
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void stellaris_uart_irq_tx_disable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->im &= ~UARTTIM_TXIM;
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if a Tx IRQ is pending, 0 otherwise
 */
static int stellaris_uart_irq_tx_ready(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	return ((uart->mis & UARTMIS_TXMIS) == UARTMIS_TXMIS);
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void stellaris_uart_irq_rx_enable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->im |= UARTTIM_RXIM;
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void stellaris_uart_irq_rx_disable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->im &= ~UARTTIM_RXIM;
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int stellaris_uart_irq_rx_ready(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	return ((uart->mis & UARTMIS_RXMIS) == UARTMIS_RXMIS);
}

/**
 * @brief Enable error interrupts
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void stellaris_uart_irq_err_enable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->im |= (UARTTIM_RTIM | UARTTIM_FEIM | UARTTIM_PEIM |
		      UARTTIM_BEIM | UARTTIM_OEIM);
}

/**
 * @brief Disable error interrupts
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static void stellaris_uart_irq_err_disable(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	uart->im &= ~(UARTTIM_RTIM | UARTTIM_FEIM | UARTTIM_PEIM |
		       UARTTIM_BEIM | UARTTIM_OEIM);
}

/**
 * @brief Check if Tx or Rx IRQ is pending
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if a Tx or Rx IRQ is pending, 0 otherwise
 */
static int stellaris_uart_irq_is_pending(struct device *dev)
{
	volatile struct _uart *uart = UART_STRUCT(dev);

	/* Look only at Tx and Rx data interrupt flags */
	return ((uart->mis & (UARTMIS_RXMIS | UARTMIS_TXMIS)) ? 1 : 0);
}

/**
 * @brief Update IRQ status
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return Always 1
 */
static int stellaris_uart_irq_update(struct device *dev)
{
	return 1;
}

/**
 * @brief Returns UART interrupt number
 *
 * Returns the IRQ number used by the specified UART port
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return N/A
 */
static unsigned int stellaris_uart_irq_get(struct device *dev)
{
	return (unsigned int)DEV_CFG(dev)->irq;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static struct uart_driver_api stellaris_uart_driver_api = {
	.poll_in = stellaris_uart_poll_in,
	.poll_out = stellaris_uart_poll_out,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	.fifo_fill = stellaris_uart_fifo_fill,
	.fifo_read = stellaris_uart_fifo_read,
	.irq_tx_enable = stellaris_uart_irq_tx_enable,
	.irq_tx_disable = stellaris_uart_irq_tx_disable,
	.irq_tx_ready = stellaris_uart_irq_tx_ready,
	.irq_rx_enable = stellaris_uart_irq_rx_enable,
	.irq_rx_disable = stellaris_uart_irq_rx_disable,
	.irq_rx_ready = stellaris_uart_irq_rx_ready,
	.irq_err_enable = stellaris_uart_irq_err_enable,
	.irq_err_disable = stellaris_uart_irq_err_disable,
	.irq_is_pending = stellaris_uart_irq_is_pending,
	.irq_update = stellaris_uart_irq_update,
	.irq_get = stellaris_uart_irq_get,

#endif
};
