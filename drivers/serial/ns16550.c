/* ns16550.c - NS16550D serial driver */

/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
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
This is the driver for the Intel NS16550 UART Chip used on the PC 386.
It uses the SCCs in asynchronous mode only.


USAGE
An ns16550 structure is used to describe the chip.
The BSP's _InitHardware() routine initializes all the
values in the uart_init_info structure before calling uart_init().

A board support package's board.h header must provide definitions for:

- the following register access routines:

  unsigned int inByte(unsigned int address);
  void outByte(unsigned char data, unsigned int address);

- and the following macro for the number of bytes between register addresses:

  UART_REG_ADDR_INTERVAL


INCLUDE FILES: drivers/uart.h
*/

#include <nanokernel.h>
#include <arch/cpu.h>
#include <stdint.h>

#include <board.h>
#include <toolchain.h>
#include <sections.h>
#include <drivers/uart.h>
#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif /* CONFIG_PCI */

/* register definitions */

#define REG_THR 0x00  /* Transmitter holding reg. */
#define REG_RDR 0x00  /* Receiver data reg.       */
#define REG_BRDL 0x00 /* Baud rate divisor (LSB)  */
#define REG_BRDH 0x01 /* Baud rate divisor (MSB)  */
#define REG_IER 0x01  /* Interrupt enable reg.    */
#define REG_IIR 0x02  /* Interrupt ID reg.        */
#define REG_FCR 0x02  /* FIFO control reg.        */
#define REG_LCR 0x03  /* Line control reg.        */
#define REG_MDC 0x04  /* Modem control reg.       */
#define REG_LSR 0x05  /* Line status reg.         */
#define REG_MSR 0x06  /* Modem status reg.        */

/* equates for interrupt enable register */

#define IER_RXRDY 0x01 /* receiver data ready */
#define IER_TBE 0x02   /* transmit bit enable */
#define IER_LSR 0x04   /* line status interrupts */
#define IER_MSI 0x08   /* modem status interrupts */

/* equates for interrupt identification register */

#define IIR_IP 0x01    /* interrupt pending bit */
#define IIR_MASK 0x07  /* interrupt id bits mask */
#define IIR_MSTAT 0x00 /* modem status interrupt */
#define IIR_THRE 0X02  /* transmit holding register empty */
#define IIR_RBRF 0x04  /* receiver buffer register full */
#define IIR_ID 0x06    /* interupt ID mask without IP */
#define IIR_SEOB 0x06  /* serialization error or break */

/* equates for FIFO control register */

#define FCR_FIFO 0x01    /* enable XMIT and RCVR FIFO */
#define FCR_RCVRCLR 0x02 /* clear RCVR FIFO */
#define FCR_XMITCLR 0x04 /* clear XMIT FIFO */

/*
 * Per PC16550D (Literature Number: SNLS378B):
 *
 * RXRDY, Mode 0: When in the 16450 Mode (FCR0 = 0) or in
 * the FIFO Mode (FCR0 = 1, FCR3 = 0) and there is at least 1
 * character in the RCVR FIFO or RCVR holding register, the
 * RXRDY pin (29) will be low active. Once it is activated the
 * RXRDY pin will go inactive when there are no more charac-
 * ters in the FIFO or holding register.
 *
 * RXRDY, Mode 1: In the FIFO Mode (FCR0 = 1) when the
 * FCR3 = 1 and the trigger level or the timeout has been
 * reached, the RXRDY pin will go low active. Once it is acti-
 * vated it will go inactive when there are no more characters
 * in the FIFO or holding register.
 *
 * TXRDY, Mode 0: In the 16450 Mode (FCR0 = 0) or in the
 * FIFO Mode (FCR0 = 1, FCR3 = 0) and there are no charac-
 * ters in the XMIT FIFO or XMIT holding register, the TXRDY
 * pin (24) will be low active. Once it is activated the TXRDY
 * pin will go inactive after the first character is loaded into the
 * XMIT FIFO or holding register.
 *
 * TXRDY, Mode 1: In the FIFO Mode (FCR0 = 1) when
 * FCR3 = 1 and there are no characters in the XMIT FIFO, the
 * TXRDY pin will go low active. This pin will become inactive
 * when the XMIT FIFO is completely full.
 */
#define FCR_MODE0 0x00 /* set receiver in mode 0 */
#define FCR_MODE1 0x08 /* set receiver in mode 1 */

/* RCVR FIFO interrupt levels: trigger interrupt with this bytes in FIFO */
#define FCR_FIFO_1 0x00  /* 1 byte in RCVR FIFO */
#define FCR_FIFO_4 0x40  /* 4 bytes in RCVR FIFO */
#define FCR_FIFO_8 0x80  /* 8 bytes in RCVR FIFO */
#define FCR_FIFO_14 0xC0 /* 14 bytes in RCVR FIFO */

/* constants for line control register */

#define LCR_CS5 0x00   /* 5 bits data size */
#define LCR_CS6 0x01   /* 6 bits data size */
#define LCR_CS7 0x02   /* 7 bits data size */
#define LCR_CS8 0x03   /* 8 bits data size */
#define LCR_2_STB 0x04 /* 2 stop bits */
#define LCR_1_STB 0x00 /* 1 stop bit */
#define LCR_PEN 0x08   /* parity enable */
#define LCR_PDIS 0x00  /* parity disable */
#define LCR_EPS 0x10   /* even parity select */
#define LCR_SP 0x20    /* stick parity select */
#define LCR_SBRK 0x40  /* break control bit */
#define LCR_DLAB 0x80  /* divisor latch access enable */

/* constants for the modem control register */

#define MCR_DTR 0x01  /* dtr output */
#define MCR_RTS 0x02  /* rts output */
#define MCR_OUT1 0x04 /* output #1 */
#define MCR_OUT2 0x08 /* output #2 */
#define MCR_LOOP 0x10 /* loop back */

/* constants for line status register */

#define LSR_RXRDY 0x01 /* receiver data available */
#define LSR_OE 0x02    /* overrun error */
#define LSR_PE 0x04    /* parity error */
#define LSR_FE 0x08    /* framing error */
#define LSR_BI 0x10    /* break interrupt */
#define LSR_THRE 0x20  /* transmit holding register empty */
#define LSR_TEMT 0x40  /* transmitter empty */

/* constants for modem status register */

#define MSR_DCTS 0x01 /* cts change */
#define MSR_DDSR 0x02 /* dsr change */
#define MSR_DRI 0x04  /* ring change */
#define MSR_DDCD 0x08 /* data carrier change */
#define MSR_CTS 0x10  /* complement of cts */
#define MSR_DSR 0x20  /* complement of dsr */
#define MSR_RI 0x40   /* complement of ring signal */
#define MSR_DCD 0x80  /* complement of dcd */

/* convenience defines */

#define THR(n) (uart[n].port + REG_THR * UART_REG_ADDR_INTERVAL)
#define RDR(n) (uart[n].port + REG_RDR * UART_REG_ADDR_INTERVAL)
#define BRDL(n) (uart[n].port + REG_BRDL * UART_REG_ADDR_INTERVAL)
#define BRDH(n) (uart[n].port + REG_BRDH * UART_REG_ADDR_INTERVAL)
#define IER(n) (uart[n].port + REG_IER * UART_REG_ADDR_INTERVAL)
#define IIR(n) (uart[n].port + REG_IIR * UART_REG_ADDR_INTERVAL)
#define FCR(n) (uart[n].port + REG_FCR * UART_REG_ADDR_INTERVAL)
#define LCR(n) (uart[n].port + REG_LCR * UART_REG_ADDR_INTERVAL)
#define MDC(n) (uart[n].port + REG_MDC * UART_REG_ADDR_INTERVAL)
#define LSR(n) (uart[n].port + REG_LSR * UART_REG_ADDR_INTERVAL)
#define MSR(n) (uart[n].port + REG_MSR * UART_REG_ADDR_INTERVAL)

#define IIRC(n) uart[n].iirCache

#define INBYTE(x) inByte(x)
#define OUTBYTE(x, d) outByte(d, x)

#if defined(CONFIG_X86_32)
#define INT_CONNECT(port, isr, arg, stub)                  \
	irq_connect((unsigned int)uart[port].irq,    \
			  (unsigned int)uart[port].intPri, \
			  isr,                              \
			  arg,                              \
			  stub)
#else
#define INT_CONNECT(port, isr, arg, stub)                          \
	do {                                                        \
		ARG_UNUSED(stub);                                   \
		irq_connect((unsigned int)uart[port].irq,    \
				  (unsigned int)uart[port].intPri, \
				  isr,                              \
				  arg);                             \
	} while (0)
#endif /* CONFIG_X86_32 */

struct ns16550 {
	uint32_t port;    /* base port number or MM base address */
	uint8_t irq;      /* interrupt request level */
	uint8_t intPri;   /* interrupt priority */
	uint8_t iirCache; /* cache of IIR since it clears when read */
};

#if !(defined(UART_PORTS_CONFIGURE)) && !(defined(CONFIG_PCI))

  #error "CONFIG_PCI or UART_PORTS_CONFIGURE is needed"

#elif !(defined(UART_PORTS_CONFIGURE)) && defined(CONFIG_PCI)

static struct ns16550 uart[CONFIG_UART_NUM_SYSTEM_PORTS] = {};

static inline void ns16550_uart_init()
{
	/*
	 * This device information is specific to Quark UART
	 * for another device it may need to be changed
	 */
	struct pci_dev_info dev_info = {
		.class = PCI_CLASS_COMM_CTLR,
		.vendor_id = CONFIG_UART_PCI_VENDOR_ID,
		.device_id = CONFIG_UART_PCI_DEVICE_ID,
		.bar = CONFIG_UART_PCI_BAR,
	};
	int i;

	if (uart[0].port && uart[0].irq)
		return;

	pci_bus_scan_init();

	for (i = 0; pci_bus_scan(&dev_info) &&
				i < CONFIG_UART_NUM_SYSTEM_PORTS; i++) {
		uart[i].port = dev_info.addr;
		uart[i].irq = dev_info.irq;
	}
}

#else

#define ns16550_uart_init() \
	do {} while ((0))

UART_PORTS_CONFIGURE(struct ns16550, uart);

#endif /* UART_PORTS_CONFIGURE */

/*******************************************************************************
*
* uart_init - initialize the chip
*
* This routine is called to reset the chip in a quiescent state.
*
* RETURNS: N/A
*/

void uart_init(int port, /* UART channel to initialize */
	       const struct uart_init_info * const init_info
	       )
{
	int oldLevel;     /* old interrupt lock level */
	uint32_t divisor; /* baud rate divisor */

	ns16550_uart_init();

	uart[port].intPri = init_info->int_pri;
	uart[port].iirCache = 0;

	oldLevel = irq_lock();

	/* calculate baud rate divisor */
	divisor = (init_info->sys_clk_freq / init_info->baud_rate) >> 4;

	/* set the DLAB to access the baud rate divisor registers */
	OUTBYTE(LCR(port), LCR_DLAB);
	OUTBYTE(BRDL(port), (unsigned char)(divisor & 0xff));
	OUTBYTE(BRDH(port), (unsigned char)((divisor >> 8) & 0xff));

	/* 8 data bits, 1 stop bit, no parity, clear DLAB */
	OUTBYTE(LCR(port), LCR_CS8 | LCR_1_STB | LCR_PDIS);

	OUTBYTE(MDC(port), MCR_OUT2 | MCR_RTS | MCR_DTR);

	/*
	 * Program FIFO: enabled, mode 0 (set for compatibility with quark),
	 * generate the interrupt at 8th byte
	 * Clear TX and RX FIFO
	 */
	OUTBYTE(FCR(port),
		FCR_FIFO | FCR_MODE0 | FCR_FIFO_8 | FCR_RCVRCLR | FCR_XMITCLR);

	/* clear the port */
	INBYTE(RDR(port));

	/* disable interrupts  */
	OUTBYTE(IER(port), 0x00);

	irq_unlock(oldLevel);
}

/*******************************************************************************
*
* uart_poll_in - poll the device for input.
*
* RETURNS: 0 if a character arrived, -1 if the input buffer if empty.
*/

int uart_poll_in(int port,		/* UART channel to select for input */
		 unsigned char *pChar /* pointer to char */
		 )
{
	if ((INBYTE(LSR(port)) & LSR_RXRDY) == 0x00)
		return (-1);

	/* got a character */
	*pChar = INBYTE(RDR(port));

	return 0;
}

/*******************************************************************************
*
* uart_poll_out - output a character in polled mode.
*
* Checks if the transmitter is empty. If empty, a character is written to
* the data register.
*
* If the hardware flow control is enabled then the handshake signal CTS has to
* be asserted in order to send a character.
*
* RETURNS: sent character
*/
unsigned char uart_poll_out(
	int port,	    /* UART channel to select for output */
	unsigned char outChar /* char to send */
	)
{
	/* wait for transmitter to ready to accept a character */
	while ((INBYTE(LSR(port)) & LSR_TEMT) == 0)
		;

	OUTBYTE(THR(port), outChar);

	return outChar;
}

#if CONFIG_UART_INTERRUPT_DRIVEN
/*******************************************************************************
*
* uart_fifo_fill - fill FIFO with data
*
* RETURNS: number of bytes sent
*/

int uart_fifo_fill(int port, /* UART on port to send */
			    const uint8_t *txData, /* data to transmit */
			    int size /* number of bytes to send */
			    )
{
	int i;

	for (i = 0; i < size && (INBYTE(LSR(port)) & LSR_THRE) != 0; i++) {
		OUTBYTE(THR(port), txData[i]);
	}
	return i;
}

/*******************************************************************************
*
* uart_fifo_read - read data from FIFO
*
* RETURNS: number of bytes read
*/

int uart_fifo_read(int port, /* UART to receive from */
			    uint8_t *rxData, /* data container */
			    const int size   /* container size */
			    )
{
	int i;

	for (i = 0; i < size && (INBYTE(LSR(port)) & LSR_RXRDY) != 0; i++) {
		rxData[i] = INBYTE(RDR(port));
	}

	return i;
}

/*******************************************************************************
*
* uart_irq_tx_enable - enable TX interrupt in IER
*
* RETURNS: N/A
*/

void uart_irq_tx_enable(int port /* UART to enable Tx
					      interrupt */
				 )
{
	OUTBYTE(IER(port), INBYTE(IER(port)) | IER_TBE);
}

/*******************************************************************************
*
* uart_irq_tx_disable - disable TX interrupt in IER
*
* RETURNS: N/A
*/

void uart_irq_tx_disable(int port /* UART to disable Tx interrupt */
				  )
{
	OUTBYTE(IER(port), INBYTE(IER(port)) & (~IER_TBE));
}

/*******************************************************************************
*
* uart_irq_tx_ready - check if Tx IRQ has been raised
*
* RETURNS: N/A
*/

int uart_irq_tx_ready(int port /* UART to check */
			       )
{
	return ((IIRC(port) & IIR_ID) == IIR_THRE);
}

/*******************************************************************************
*
* _uart_irq_rx_enable - enable RX interrupt in IER
*
* RETURNS: N/A
*/

void uart_irq_rx_enable(int port /* UART to enable Rx
					      interrupt */
				 )
{
	OUTBYTE(IER(port), INBYTE(IER(port)) | IER_RXRDY);
}

/*******************************************************************************
*
* uart_irq_rx_disable - disable RX interrupt in IER
*
* RETURNS: N/A
*/

void uart_irq_rx_disable(int port /* UART to disable Rx interrupt */
				  )
{
	OUTBYTE(IER(port), INBYTE(IER(port)) & (~IER_RXRDY));
}

/*******************************************************************************
*
* uart_irq_rx_ready - check if Rx IRQ has been raised
*
* RETURNS: 1 if an IRQ is ready, 0 otherwise
*/

int uart_irq_rx_ready(int port /* UART to check */
			       )
{
	return ((IIRC(port) & IIR_ID) == IIR_RBRF);
}

/*******************************************************************************
*
* uart_irq_err_enable - enable error interrupt in IER
*
* RETURNS: N/A
*/

void uart_irq_err_enable(int port /* UART to enable Rx interrupt */
			 )
{
	OUTBYTE(IER(port), INBYTE(IER(port)) | IER_LSR);
}

/*******************************************************************************
*
* uart_irq_err_disable - disable error interrupt in IER
*
* RETURNS: 1 if an IRQ is ready, 0 otherwise
*/

void uart_irq_err_disable(int port /* UART to disable Rx interrupt */
			  )
{
	OUTBYTE(IER(port), INBYTE(IER(port)) & (~IER_LSR));
}

/*******************************************************************************
*
* uart_irq_is_pending - check if any IRQ is pending
*
* RETURNS: 1 if an IRQ is pending, 0 otherwise
*/

int uart_irq_is_pending(int port /* UART to check */
				 )
{
	return (!(IIRC(port) & IIR_IP));
}

/*******************************************************************************
*
* uart_irq_update - update cached contents of IIR
*
* RETURNS: always 1
*/

int uart_irq_update(int port /* UART to update */
			     )
{
	IIRC(port) = INBYTE(IIR(port));

	return 1;
}

/*******************************************************************************
*
* uart_int_connect - connect an ISR to an interrupt line
*
* The kernel configuration allows to setup an interrupt line for a particular
* DUART. This routine installs the ISR of a UART user to the interrupt line
* chosen for the hardware at configuration time.
*
* RETURNS: N/A
*/

void uart_int_connect(int port,	   /* UART to port to connect */
		      void (*isr)(void *), /* interrupt handler */
		      void *arg,	   /* argument to pass to handler */
		      void *stub	   /* ptr to interrupt stub code */
		      )
{
#if !defined(CONFIG_DYNAMIC_INT_STUBS)
	ARG_UNUSED(isr);
	ARG_UNUSED(arg);
	ARG_UNUSED(stub);
#else
	INT_CONNECT(port, isr, arg, stub);
#endif /* CONFIG_DYNAMIC_INT_STUBS */

	irq_enable((unsigned int)uart[port].irq);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
