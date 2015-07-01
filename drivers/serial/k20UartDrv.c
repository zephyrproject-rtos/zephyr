/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This is the UART driver for the Freescale K20 Family of microprocessors.

USAGE
An _K20_UART_t structure is used to describe the UART.
The BSP's _InitHardware() routine initializes all the
values in the uart_init_info structure before calling uart_init().

INCLUDE FILES: drivers/serial/k20_uart.h
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <stdint.h>

#include <board.h>
#include <drivers/uart.h>
#include <drivers/k20_uart.h>
#include <drivers/k20_sim.h>
#include <toolchain.h>
#include <sections.h>

typedef struct {
	uint8_t *base; /* base address of registers */
	uint8_t irq;       /* interrupt request level */
	uint8_t intPri;    /* interrupt priority */
} _k20Uart_t;

UART_PORTS_CONFIGURE(_k20Uart_t, uart);

/**
 *
 * @brief Initialize UART channel
 *
 * This routine is called to reset the chip in a quiescent state.
 * It is assumed that this function is called only once per UART.
 *
 * @return N/A
 */

void uart_init(int port, /* UART channel to initialize */
	       const struct uart_init_info * const init_info
	       )
{
	int oldLevel; /* old interrupt lock level */
	K20_SIM_t *sim_p =
		(K20_SIM_t *)PERIPH_ADDR_BASE_SIM; /* sys integ. ctl */
	C1_t c1;				   /* UART C1 register value */
	C2_t c2;				   /* UART C2 register value */

	uart[port].intPri = init_info->int_pri;

	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	/* disable interrupts */
	oldLevel = irq_lock();

	/* enable clock to Uart - must be done prior to device access */
	_k20SimUartClkEnable(sim_p, port);

	_k20UartBaudRateSet(uart_p, init_info->sys_clk_freq, init_info->baud_rate);

	/* 1 start bit, 8 data bits, no parity, 1 stop bit */
	c1.value = 0;

	uart_p->c1 = c1;

	/* enable Rx and Tx with interrupts disabled */
	c2.value = 0;
	c2.field.rxEnable = 1;
	c2.field.txEnable = 1;

	uart_p->c2 = c2;

	/* restore interrupt state */
	irq_unlock(oldLevel);
}

/**
 *
 * @brief Poll the device for input.
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */

int uart_poll_in(int port,		/* UART channel to select for input */
		 unsigned char *pChar /* pointer to char */
		 )
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	if (uart_p->s1.field.rxDataFull == 0)
		return (-1);

	/* got a character */
	*pChar = uart_p->d;

	return 0;
}

/**
 *
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * If the hardware flow control is enabled then the handshake signal CTS has to
 * be asserted in order to send a character.
 *
 * @return sent character
 */
unsigned char uart_poll_out(
	int port,	    /* UART channel to select for output */
	unsigned char outChar /* char to send */
	)
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	/* wait for transmitter to ready to accept a character */
	while (uart_p->s1.field.txDataEmpty == 0)
		;

	uart_p->d = outChar;

	return outChar;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

/**
 *
 * @brief Fill FIFO with data

 * @return number of bytes sent
 */

int uart_fifo_fill(int port, /* UART on port to send */
			    const uint8_t *txData, /* data to transmit */
			    int len /* number of bytes to send */
			    )
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;
	uint8_t numTx = 0;

	while ((len - numTx > 0) && (uart_p->s1.field.txDataEmpty == 1)) {
		uart_p->d = txData[numTx++];
	}

	return numTx;
}

/**
 *
 * @brief Read data from FIFO
 *
 * @return number of bytes read
 */

int uart_fifo_read(int port, /* UART to receive from */
			    uint8_t *rxData, /* data container */
			    const int size   /* container size */
			    )
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;
	uint8_t numRx = 0;

	while ((size - numRx > 0) && (uart_p->s1.field.rxDataFull == 0)) {
		rxData[numRx++] = uart_p->d;
	}

	return numRx;
}

/**
 *
 * @brief Enable TX interrupt
 *
 * @return N/A
 */

void uart_irq_tx_enable(int port /* UART to enable Tx
					      interrupt */
				 )
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	uart_p->c2.field.txInt_DmaTx_en = 1;
}

/**
 *
 * @brief Disable TX interrupt in IER
 *
 * @return N/A
 */

void uart_irq_tx_disable(
	int port /* UART to disable Tx interrupt */
	)
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	uart_p->c2.field.txInt_DmaTx_en = 0;
}

/**
 *
 * @brief Check if Tx IRQ has been raised
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */

int uart_irq_tx_ready(int port /* UART to check */
			       )
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	return uart_p->s1.field.txDataEmpty;
}

/**
 *
 * @brief Enable RX interrupt in IER
 *
 * @return N/A
 */

void uart_irq_rx_enable(int port /* UART to enable Rx
					      interrupt */
				 )
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	uart_p->c2.field.rxFullInt_dmaTx_en = 1;
}

/**
 *
 * @brief Disable RX interrupt in IER
 *
 * @return N/A
 */

void uart_irq_rx_disable(
	int port /* UART to disable Rx interrupt */
	)
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	uart_p->c2.field.rxFullInt_dmaTx_en = 0;
}

/**
 *
 * @brief Check if Rx IRQ has been raised
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */

int uart_irq_rx_ready(int port /* UART to check */
			       )
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	return uart_p->s1.field.rxDataFull;
}

/**
 *
 * @brief Enable error interrupt
 *
 * @return N/A
 */

void uart_irq_err_enable(int port)
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;
	C3_t c3 = uart_p->c3;

	c3.field.parityErrIntEn = 1;
	c3.field.frameErrIntEn = 1;
	c3.field.noiseErrIntEn = 1;
	c3.field.overrunErrIntEn = 1;
	uart_p->c3 = c3;
}

/**
 *
 * @brief Disable error interrupt
 *
 * @return N/A
 */

void uart_irq_err_disable(int port /* UART to disable Rx interrupt */
			  )
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;
	C3_t c3 = uart_p->c3;

	c3.field.parityErrIntEn = 0;
	c3.field.frameErrIntEn = 0;
	c3.field.noiseErrIntEn = 0;
	c3.field.overrunErrIntEn = 0;
	uart_p->c3 = c3;
}

/**
 *
 * @brief Check if Tx or Rx IRQ is pending
 *
 * @return 1 if a Tx or Rx IRQ is pending, 0 otherwise
 */

int uart_irq_is_pending(int port /* UART to check */
				 )
{
	K20_UART_t *uart_p = (K20_UART_t *)uart[port].base;

	/* Look only at Tx and Rx data interrupt flags */

	return ((uart_p->s1.value & (TX_DATA_EMPTY_MASK | RX_DATA_FULL_MASK))
			? 1
			: 0);
}

/**
 *
 * @brief Update IRQ status
 *
 * @return always 1
 */

int uart_irq_update(int port)
{
	return 1;
}

/**
 *
 * @brief Returns UART interrupt number
 *
 * Returns the IRQ number used by the specified UART port
 *
 * @return N/A
 */

unsigned int uart_irq_get(int port /* UART port */
			  )
{
	return (unsigned int)uart[port].irq;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
