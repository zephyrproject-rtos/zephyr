/* ns16550.c - NS16550D serial driver */

/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief NS16550 Serial Driver
 *
 * This is the driver for the Intel NS16550 UART Chip used on the PC 386.
 * It uses the SCCs in asynchronous mode only.
 *
 * Before individual UART port can be used, uart_ns16550_port_init() has to be
 * called to setup the port.
 *
 * - the following macro for the number of bytes between register addresses:
 *
 *  UART_REG_ADDR_INTERVAL
 */

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>
#include <zephyr/types.h>

#include <board.h>
#include <init.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <uart.h>
#include <sys_io.h>

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif /* CONFIG_PCI */

#include "uart_ns16550.h"

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
#define REG_DLF 0xC0  /* Divisor Latch Fraction   */

/* equates for interrupt enable register */

#define IER_RXRDY 0x01 /* receiver data ready */
#define IER_TBE 0x02   /* transmit bit enable */
#define IER_LSR 0x04   /* line status interrupts */
#define IER_MSI 0x08   /* modem status interrupts */

/* equates for interrupt identification register */

#define IIR_MSTAT 0x00 /* modem status interrupt  */
#define IIR_NIP   0x01 /* no interrupt pending    */
#define IIR_THRE  0x02 /* transmit holding register empty interrupt */
#define IIR_RBRF  0x04 /* receiver buffer register full interrupt */
#define IIR_LS    0x06 /* receiver line status interrupt */
#define IIR_MASK  0x07 /* interrupt id bits mask  */
#define IIR_ID    0x06 /* interrupt ID mask without NIP */

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

/*
 * UART NS16750 supports 64 bytes FIFO, which can be enabled
 * via the FCR register
 */
#define FCR_FIFO_64 0x20 /* Enable 64 bytes FIFO */

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
#define MCR_AFCE 0x20 /* auto flow control enable */

/* constants for line status register */

#define LSR_RXRDY 0x01 /* receiver data available */
#define LSR_OE 0x02    /* overrun error */
#define LSR_PE 0x04    /* parity error */
#define LSR_FE 0x08    /* framing error */
#define LSR_BI 0x10    /* break interrupt */
#define LSR_EOB_MASK 0x1E /* Error or Break mask */
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

#define DEV_CFG(dev) \
	((const struct uart_ns16550_device_config * const) \
	 (dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_ns16550_dev_data_t *)(dev)->driver_data)

#define THR(dev) (DEV_DATA(dev)->port + REG_THR * UART_REG_ADDR_INTERVAL)
#define RDR(dev) (DEV_DATA(dev)->port + REG_RDR * UART_REG_ADDR_INTERVAL)
#define BRDL(dev) (DEV_DATA(dev)->port + REG_BRDL * UART_REG_ADDR_INTERVAL)
#define BRDH(dev) (DEV_DATA(dev)->port + REG_BRDH * UART_REG_ADDR_INTERVAL)
#define IER(dev) (DEV_DATA(dev)->port + REG_IER * UART_REG_ADDR_INTERVAL)
#define IIR(dev) (DEV_DATA(dev)->port + REG_IIR * UART_REG_ADDR_INTERVAL)
#define FCR(dev) (DEV_DATA(dev)->port + REG_FCR * UART_REG_ADDR_INTERVAL)
#define LCR(dev) (DEV_DATA(dev)->port + REG_LCR * UART_REG_ADDR_INTERVAL)
#define MDC(dev) (DEV_DATA(dev)->port + REG_MDC * UART_REG_ADDR_INTERVAL)
#define LSR(dev) (DEV_DATA(dev)->port + REG_LSR * UART_REG_ADDR_INTERVAL)
#define MSR(dev) (DEV_DATA(dev)->port + REG_MSR * UART_REG_ADDR_INTERVAL)
#define DLF(dev) (DEV_DATA(dev)->port + REG_DLF)

#define IIRC(dev) (DEV_DATA(dev)->iir_cache)

#ifdef UART_NS16550_ACCESS_IOPORT
#define INBYTE(x) sys_in8(x)
#define OUTBYTE(x, d) sys_out8(d, x)
#define UART_REG_ADDR_INTERVAL 1 /* address diff of adjacent regs. */
#else
#define INBYTE(x) sys_read8(x)
#define OUTBYTE(x, d) sys_write8(d, x)
#define UART_REG_ADDR_INTERVAL 4 /* address diff of adjacent regs. */
#endif /* UART_NS16550_ACCESS_IOPORT */


struct uart_ns16550_device_config {
	u32_t sys_clk_freq;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t	irq_config_func;
#endif
};

/** Device data structure */
struct uart_ns16550_dev_data_t {
	u32_t port;
	u32_t baud_rate;	/**< Baud rate */
	u8_t options;	/**< Serial port options */

#ifdef CONFIG_PCI
	struct pci_dev_info  pci_dev;
#endif /* CONFIG_PCI */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	u8_t iir_cache;	/**< cache of IIR since it clears when read */
	uart_irq_callback_t	cb;	/**< Callback function pointer */
#endif

#ifdef CONFIG_UART_NS16550_DLF
	u8_t dlf;		/**< DLF value */
#endif
};

static const struct uart_driver_api uart_ns16550_driver_api;

#ifdef CONFIG_UART_NS16550_DLF
static inline void set_dlf(struct device *dev, u32_t val)
{
	struct uart_ns16550_dev_data_t * const dev_data = DEV_DATA(dev);

	OUTBYTE(DLF(dev), val);
	dev_data->dlf = val;
}
#endif

static void set_baud_rate(struct device *dev, u32_t baud_rate)
{
	const struct uart_ns16550_device_config * const dev_cfg = DEV_CFG(dev);
	struct uart_ns16550_dev_data_t * const dev_data = DEV_DATA(dev);
	u32_t divisor; /* baud rate divisor */
	u8_t lcr_cache;

	if ((baud_rate != 0) && (dev_cfg->sys_clk_freq != 0)) {
		/* calculate baud rate divisor */
		divisor = (dev_cfg->sys_clk_freq / baud_rate) >> 4;

		/* set the DLAB to access the baud rate divisor registers */
		lcr_cache = INBYTE(LCR(dev));
		OUTBYTE(LCR(dev), LCR_DLAB);
		OUTBYTE(BRDL(dev), (unsigned char)(divisor & 0xff));
		OUTBYTE(BRDH(dev), (unsigned char)((divisor >> 8) & 0xff));

		/* restore the DLAB to access the baud rate divisor registers */
		OUTBYTE(LCR(dev), lcr_cache);

		dev_data->baud_rate = baud_rate;
	}
}

#if defined(CONFIG_UART_NS16550_PCI)
static inline int ns16550_pci_uart_scan(struct device *dev)
{
	struct uart_ns16550_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->pci_dev.vendor_id == 0x0000) {
		return -EINVAL;
	}

	pci_bus_scan_init();

	if (!pci_bus_scan(&dev_data->pci_dev)) {
		return 0;
	}

#ifdef CONFIG_PCI_ENUMERATION
	dev_data->port = dev_data->pci_dev.addr;
#endif

	pci_enable_regs(&dev_data->pci_dev);

	return 1;
}

#else

#define ns16550_pci_uart_scan(_unused_) (1)

#endif /* CONFIG_UART_NS16550_PCI */

/**
 * @brief Initialize individual UART port
 *
 * This routine is called to reset the chip in a quiescent state.
 *
 * @param dev UART device struct
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_ns16550_init(struct device *dev)
{
	struct uart_ns16550_dev_data_t * const dev_data = DEV_DATA(dev);

	int old_level;     /* old interrupt lock level */
	u8_t mdc = 0;

	if (!ns16550_pci_uart_scan(dev)) {
		dev->driver_api = NULL;
		return -ENOTSUP;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_data->iir_cache = 0;
#endif

	old_level = irq_lock();

	set_baud_rate(dev, dev_data->baud_rate);

#ifdef CONFIG_UART_NS16550_DLF
	set_dlf(dev, dev_data->dlf);
#endif

	/* 8 data bits, 1 stop bit, no parity, clear DLAB */
	OUTBYTE(LCR(dev), LCR_CS8 | LCR_1_STB | LCR_PDIS);

	mdc = MCR_OUT2 | MCR_RTS | MCR_DTR;
	if ((dev_data->options & UART_OPTION_AFCE) == UART_OPTION_AFCE)
		mdc |= MCR_AFCE;

	OUTBYTE(MDC(dev), mdc);

	/*
	 * Program FIFO: enabled, mode 0 (set for compatibility with quark),
	 * generate the interrupt at 8th byte
	 * Clear TX and RX FIFO
	 */
	OUTBYTE(FCR(dev),
		FCR_FIFO | FCR_MODE0 | FCR_FIFO_8 | FCR_RCVRCLR | FCR_XMITCLR
#ifdef CONFIG_UART_NS16750
		| FCR_FIFO_64
#endif
		);

	/* clear the port */
	INBYTE(RDR(dev));

	/* disable interrupts  */
	OUTBYTE(IER(dev), 0x00);

	irq_unlock(old_level);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	DEV_CFG(dev)->irq_config_func(dev);
#endif

	return 0;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_ns16550_poll_in(struct device *dev, unsigned char *c)
{
	if ((INBYTE(LSR(dev)) & LSR_RXRDY) == 0x00)
		return (-1);

	/* got a character */
	*c = INBYTE(RDR(dev));

	return 0;
}

/**
 * @brief Output a character in polled mode.
 *
 * Checks if the transmitter is empty. If empty, a character is written to
 * the data register.
 *
 * If the hardware flow control is enabled then the handshake signal CTS has to
 * be asserted in order to send a character.
 *
 * @param dev UART device struct
 * @param c Character to send
 *
 * @return Sent character
 */
static unsigned char uart_ns16550_poll_out(struct device *dev,
					   unsigned char c)
{
	/* wait for transmitter to ready to accept a character */
	while ((INBYTE(LSR(dev)) & LSR_TEMT) == 0)
		;

	OUTBYTE(THR(dev), c);

	return c;
}

/**
 * @brief Check if an error was received
 *
 * @param dev UART device struct
 *
 * @return one of UART_ERROR_OVERRUN, UART_ERROR_PARITY, UART_ERROR_FRAMING,
 * UART_ERROR_BREAK if an error was detected, 0 otherwise.
 */
static int uart_ns16550_err_check(struct device *dev)
{
	return (INBYTE(LSR(dev)) & LSR_EOB_MASK) >> 1;
}

#if CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int uart_ns16550_fifo_fill(struct device *dev, const u8_t *tx_data,
				  int size)
{
	int i;

	for (i = 0; i < size && (INBYTE(LSR(dev)) & LSR_THRE) != 0; i++) {
		OUTBYTE(THR(dev), tx_data[i]);
	}
	return i;
}

/**
 * @brief Read data from FIFO
 *
 * @param dev UART device struct
 * @param rxData Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int uart_ns16550_fifo_read(struct device *dev, u8_t *rx_data,
				  const int size)
{
	int i;

	for (i = 0; i < size && (INBYTE(LSR(dev)) & LSR_RXRDY) != 0; i++) {
		rx_data[i] = INBYTE(RDR(dev));
	}

	return i;
}

/**
 * @brief Enable TX interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_ns16550_irq_tx_enable(struct device *dev)
{
	OUTBYTE(IER(dev), INBYTE(IER(dev)) | IER_TBE);
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_ns16550_irq_tx_disable(struct device *dev)
{
	OUTBYTE(IER(dev), INBYTE(IER(dev)) & (~IER_TBE));
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_ns16550_irq_tx_ready(struct device *dev)
{
	return ((IIRC(dev) & IIR_ID) == IIR_THRE);
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_ns16550_irq_tx_complete(struct device *dev)
{
	return (INBYTE(LSR(dev)) & (LSR_TEMT | LSR_THRE)) == (LSR_TEMT | LSR_THRE);
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_ns16550_irq_rx_enable(struct device *dev)
{
	OUTBYTE(IER(dev), INBYTE(IER(dev)) | IER_RXRDY);
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_ns16550_irq_rx_disable(struct device *dev)
{
	OUTBYTE(IER(dev), INBYTE(IER(dev)) & (~IER_RXRDY));
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_ns16550_irq_rx_ready(struct device *dev)
{
	return ((IIRC(dev) & IIR_ID) == IIR_RBRF);
}

/**
 * @brief Enable error interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return N/A
 */
static void uart_ns16550_irq_err_enable(struct device *dev)
{
	OUTBYTE(IER(dev), INBYTE(IER(dev)) | IER_LSR);
}

/**
 * @brief Disable error interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static void uart_ns16550_irq_err_disable(struct device *dev)
{
	OUTBYTE(IER(dev), INBYTE(IER(dev)) & (~IER_LSR));
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_ns16550_irq_is_pending(struct device *dev)
{
	return (!(IIRC(dev) & IIR_NIP));
}

/**
 * @brief Update cached contents of IIR
 *
 * @param dev UART device struct
 *
 * @return Always 1
 */
static int uart_ns16550_irq_update(struct device *dev)
{
	IIRC(dev) = INBYTE(IIR(dev));

	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 *
 * @return N/A
 */
static void uart_ns16550_irq_callback_set(struct device *dev,
					  uart_irq_callback_t cb)
{
	struct uart_ns16550_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 *
 * @return N/A
 */
static void uart_ns16550_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_ns16550_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->cb) {
		dev_data->cb(dev);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_NS16550_LINE_CTRL

/**
 * @brief Manipulate line control for UART.
 *
 * @param dev UART device struct
 * @param ctrl The line control to be manipulated
 * @param val Value to set the line control
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_ns16550_line_ctrl_set(struct device *dev,
				      u32_t ctrl, u32_t val)
{
	u32_t mdc, chg;

	switch (ctrl) {
	case LINE_CTRL_BAUD_RATE:
		set_baud_rate(dev, val);
		return 0;

	case LINE_CTRL_RTS:
	case LINE_CTRL_DTR:
		mdc = INBYTE(MDC(dev));

		if (ctrl == LINE_CTRL_RTS) {
			chg = MCR_RTS;
		} else {
			chg = MCR_DTR;
		}

		if (val) {
			mdc |= chg;
		} else {
			mdc &= ~(chg);
		}
		OUTBYTE(MDC(dev), mdc);
		return 0;
	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_NS16550_LINE_CTRL */

#ifdef CONFIG_UART_NS16550_DRV_CMD

/**
 * @brief Send extra command to driver
 *
 * @param dev UART device struct
 * @param cmd Command to driver
 * @param p Parameter to the command
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_ns16550_drv_cmd(struct device *dev, u32_t cmd, u32_t p)
{
	switch (cmd) {

#ifdef CONFIG_UART_NS16550_DLF
	case CMD_SET_DLF:
		set_dlf(dev, p);
		return 0;
#endif

	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_NS16550_DRV_CMD */


static const struct uart_driver_api uart_ns16550_driver_api = {
	.poll_in = uart_ns16550_poll_in,
	.poll_out = uart_ns16550_poll_out,
	.err_check = uart_ns16550_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	.fifo_fill = uart_ns16550_fifo_fill,
	.fifo_read = uart_ns16550_fifo_read,
	.irq_tx_enable = uart_ns16550_irq_tx_enable,
	.irq_tx_disable = uart_ns16550_irq_tx_disable,
	.irq_tx_ready = uart_ns16550_irq_tx_ready,
	.irq_tx_complete = uart_ns16550_irq_tx_complete,
	.irq_rx_enable = uart_ns16550_irq_rx_enable,
	.irq_rx_disable = uart_ns16550_irq_rx_disable,
	.irq_rx_ready = uart_ns16550_irq_rx_ready,
	.irq_err_enable = uart_ns16550_irq_err_enable,
	.irq_err_disable = uart_ns16550_irq_err_disable,
	.irq_is_pending = uart_ns16550_irq_is_pending,
	.irq_update = uart_ns16550_irq_update,
	.irq_callback_set = uart_ns16550_irq_callback_set,

#endif

#ifdef CONFIG_UART_NS16550_LINE_CTRL
	.line_ctrl_set = uart_ns16550_line_ctrl_set,
#endif

#ifdef CONFIG_UART_NS16550_DRV_CMD
	.drv_cmd = uart_ns16550_drv_cmd,
#endif
};

#ifdef CONFIG_UART_NS16550_PORT_0

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_0(struct device *port);
#endif

static const struct uart_ns16550_device_config uart_ns16550_dev_cfg_0 = {
	.sys_clk_freq = UART_NS16550_PORT_0_CLK_FREQ,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_0,
#endif
};

static struct uart_ns16550_dev_data_t uart_ns16550_dev_data_0 = {
#ifdef CONFIG_UART_NS16550_PORT_0_PCI
	.pci_dev.class_type = UART_NS16550_PORT_0_PCI_CLASS,
	.pci_dev.bus = UART_NS16550_PORT_0_PCI_BUS,
	.pci_dev.dev = UART_NS16550_PORT_0_PCI_DEV,
	.pci_dev.vendor_id = UART_NS16550_PORT_0_PCI_VENDOR_ID,
	.pci_dev.device_id = UART_NS16550_PORT_0_PCI_DEVICE_ID,
	.pci_dev.function = UART_NS16550_PORT_0_PCI_FUNC,
	.pci_dev.bar = UART_NS16550_PORT_0_PCI_BAR,
#endif /* CONFIG_UART_NS16550_PORT_0_PCI */

	.port = UART_NS16550_PORT_0_BASE_ADDR,
	.baud_rate = CONFIG_UART_NS16550_PORT_0_BAUD_RATE,
	.options = CONFIG_UART_NS16550_PORT_0_OPTIONS,

#ifdef CONFIG_UART_NS16550_PORT_0_DLF
	.dlf = CONFIG_UART_NS16550_PORT_0_DLF,
#endif
};

DEVICE_AND_API_INIT(uart_ns16550_0, CONFIG_UART_NS16550_PORT_0_NAME, &uart_ns16550_init,
		    &uart_ns16550_dev_data_0, &uart_ns16550_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_ns16550_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_0(struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(UART_NS16550_PORT_0_IRQ,
		    CONFIG_UART_NS16550_PORT_0_IRQ_PRI,
		    uart_ns16550_isr, DEVICE_GET(uart_ns16550_0),
		    UART_IRQ_FLAGS);
	irq_enable(UART_NS16550_PORT_0_IRQ);
}
#endif

#endif /* CONFIG_UART_NS16550_PORT_0 */

#ifdef CONFIG_UART_NS16550_PORT_1

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_1(struct device *port);
#endif

static const struct uart_ns16550_device_config uart_ns16550_dev_cfg_1 = {
	.sys_clk_freq = UART_NS16550_PORT_1_CLK_FREQ,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = irq_config_func_1,
#endif
};

static struct uart_ns16550_dev_data_t uart_ns16550_dev_data_1 = {
#ifdef CONFIG_UART_NS16550_PORT_1_PCI
	.pci_dev.class_type = UART_NS16550_PORT_1_PCI_CLASS,
	.pci_dev.bus = UART_NS16550_PORT_1_PCI_BUS,
	.pci_dev.dev = UART_NS16550_PORT_1_PCI_DEV,
	.pci_dev.vendor_id = UART_NS16550_PORT_1_PCI_VENDOR_ID,
	.pci_dev.device_id = UART_NS16550_PORT_1_PCI_DEVICE_ID,
	.pci_dev.function = UART_NS16550_PORT_1_PCI_FUNC,
	.pci_dev.bar = UART_NS16550_PORT_1_PCI_BAR,
#endif /* CONFIG_UART_NS16550_PORT_1_PCI */

	.port = UART_NS16550_PORT_1_BASE_ADDR,
	.baud_rate = CONFIG_UART_NS16550_PORT_1_BAUD_RATE,
	.options = CONFIG_UART_NS16550_PORT_1_OPTIONS,

#ifdef CONFIG_UART_NS16550_PORT_1_DLF
	.dlf = CONFIG_UART_NS16550_PORT_1_DLF,
#endif
};

DEVICE_AND_API_INIT(uart_ns16550_1, CONFIG_UART_NS16550_PORT_1_NAME, &uart_ns16550_init,
		    &uart_ns16550_dev_data_1, &uart_ns16550_dev_cfg_1,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &uart_ns16550_driver_api);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void irq_config_func_1(struct device *dev)
{
	ARG_UNUSED(dev);

	IRQ_CONNECT(UART_NS16550_PORT_1_IRQ,
		    CONFIG_UART_NS16550_PORT_1_IRQ_PRI,
		    uart_ns16550_isr, DEVICE_GET(uart_ns16550_1),
		    UART_IRQ_FLAGS);
	irq_enable(UART_NS16550_PORT_1_IRQ);
}
#endif

#endif /* CONFIG_UART_NS16550_PORT_1 */
