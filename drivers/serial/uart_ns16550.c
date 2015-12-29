/* ns16550.c - NS16550D serial driver */

/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
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

#include <nanokernel.h>
#include <arch/cpu.h>
#include <stdint.h>

#include <board.h>
#include <init.h>
#include <toolchain.h>
#include <sections.h>
#include <uart.h>
#include <sys_io.h>

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
#define IIR_ID 0x06    /* interrupt ID mask without IP */
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
#define MCR_AFCE 0x20 /* auto flow control enable */

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

#define DEV_CFG(dev) \
	((struct uart_device_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_ns16550_dev_data_t *)(dev)->driver_data)

#define THR(dev)	(DEV_CFG(dev)->port + REG_THR * UART_REG_ADDR_INTERVAL)
#define RDR(dev)	(DEV_CFG(dev)->port + REG_RDR * UART_REG_ADDR_INTERVAL)
#define BRDL(dev)	(DEV_CFG(dev)->port + REG_BRDL * UART_REG_ADDR_INTERVAL)
#define BRDH(dev)	(DEV_CFG(dev)->port + REG_BRDH * UART_REG_ADDR_INTERVAL)
#define IER(dev)	(DEV_CFG(dev)->port + REG_IER * UART_REG_ADDR_INTERVAL)
#define IIR(dev)	(DEV_CFG(dev)->port + REG_IIR * UART_REG_ADDR_INTERVAL)
#define FCR(dev)	(DEV_CFG(dev)->port + REG_FCR * UART_REG_ADDR_INTERVAL)
#define LCR(dev)	(DEV_CFG(dev)->port + REG_LCR * UART_REG_ADDR_INTERVAL)
#define MDC(dev)	(DEV_CFG(dev)->port + REG_MDC * UART_REG_ADDR_INTERVAL)
#define LSR(dev)	(DEV_CFG(dev)->port + REG_LSR * UART_REG_ADDR_INTERVAL)
#define MSR(dev)	(DEV_CFG(dev)->port + REG_MSR * UART_REG_ADDR_INTERVAL)

#define IIRC(dev)	(DEV_DATA(dev)->iir_cache)

#ifdef CONFIG_UART_NS16550_ACCESS_IOPORT
#define INBYTE(x) sys_in8(x)
#define OUTBYTE(x, d) sys_out8(d, x)
#define UART_REG_ADDR_INTERVAL 1 /* address diff of adjacent regs. */
#endif /* CONFIG_UART_NS16550_ACCESS_IOPORT */

#ifdef CONFIG_UART_NS16550_ACCESS_MMIO
#define INBYTE(x) sys_read8(x)
#define OUTBYTE(x, d) sys_write8(d, x)
#define UART_REG_ADDR_INTERVAL 4 /* address diff of adjacent regs. */
#endif /* CONFIG_UART_NS16550_ACCESS_MMIO */

/** Device data structure */
struct uart_ns16550_dev_data_t {
	uint8_t iir_cache;	/**< cache of IIR since it clears when read */
};

static struct uart_driver_api uart_ns16550_driver_api;

#if defined(CONFIG_UART_NS16550_PCI)

static inline int ns16550_pci_uart_scan(struct device *dev)
{
	struct uart_device_config * const dev_cfg = DEV_CFG(dev);

	if (dev_cfg->pci_dev.vendor_id == 0x0000) {
		return DEV_INVALID_CONF;
	}

	pci_bus_scan_init();

	if (!pci_bus_scan(&dev_cfg->pci_dev)) {
		return 0;
	}

#ifdef CONFIG_PCI_ENUMERATION
	dev_cfg->port = dev_cfg->pci_dev.addr;
	dev_cfg->irq = dev_cfg->pci_dev.irq;
#endif

	pci_enable_regs(&dev_cfg->pci_dev);

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
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return DEV_OK if successful, failed othersie
 */
static int uart_ns16550_init(struct device *dev)
{
	struct uart_device_config * const dev_cfg = DEV_CFG(dev);
	struct uart_ns16550_dev_data_t * const dev_data = DEV_DATA(dev);
	struct uart_init_info * const init_info = &dev_cfg->init_info;

	int old_level;     /* old interrupt lock level */
	uint32_t divisor; /* baud rate divisor */
	uint8_t mdc = 0;

	if (!ns16550_pci_uart_scan(dev)) {
		return DEV_INVALID_OP;
	}

	dev_data->iir_cache = 0;

	old_level = irq_lock();

	if ((init_info->baud_rate != 0) && (init_info->sys_clk_freq != 0)) {
		/* calculate baud rate divisor */
		divisor = (init_info->sys_clk_freq / init_info->baud_rate) >> 4;

		/* set the DLAB to access the baud rate divisor registers */
		OUTBYTE(LCR(dev), LCR_DLAB);
		OUTBYTE(BRDL(dev), (unsigned char)(divisor & 0xff));
		OUTBYTE(BRDH(dev), (unsigned char)((divisor >> 8) & 0xff));
	}

	/* 8 data bits, 1 stop bit, no parity, clear DLAB */
	OUTBYTE(LCR(dev), LCR_CS8 | LCR_1_STB | LCR_PDIS);

	mdc = MCR_OUT2 | MCR_RTS | MCR_DTR;
	if ((init_info->options & UART_OPTION_AFCE) == UART_OPTION_AFCE)
		mdc |= MCR_AFCE;

	OUTBYTE(MDC(dev), mdc);

	/*
	 * Program FIFO: enabled, mode 0 (set for compatibility with quark),
	 * generate the interrupt at 8th byte
	 * Clear TX and RX FIFO
	 */
	OUTBYTE(FCR(dev),
		FCR_FIFO | FCR_MODE0 | FCR_FIFO_8 | FCR_RCVRCLR | FCR_XMITCLR);

	/* clear the port */
	INBYTE(RDR(dev));

	/* disable interrupts  */
	OUTBYTE(IER(dev), 0x00);

	irq_unlock(old_level);

	dev->driver_api = &uart_ns16550_driver_api;

	return DEV_OK;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct (of type struct uart_device_config)
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
 * @param dev UART device struct (of type struct uart_device_config)
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

#if CONFIG_UART_INTERRUPT_DRIVEN

/**
 * @brief Fill FIFO with data
 *
 * @param dev UART device struct (of type struct uart_device_config)
 * @param tx_data Data to transmit
 * @param size Number of bytes to send
 *
 * @return Number of bytes sent
 */
static int uart_ns16550_fifo_fill(struct device *dev, const uint8_t *tx_data,
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
 * @param dev UART device struct (of type struct uart_device_config)
 * @param rxData Data container
 * @param size Container size
 *
 * @return Number of bytes read
 */
static int uart_ns16550_fifo_read(struct device *dev, uint8_t *rx_data,
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
 * @param dev UART device struct (of type struct uart_device_config)
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
 * @param dev UART device struct (of type struct uart_device_config)
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
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_ns16550_irq_tx_ready(struct device *dev)
{
	return ((IIRC(dev) & IIR_ID) == IIR_THRE);
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct (of type struct uart_device_config)
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
 * @param dev UART device struct (of type struct uart_device_config)
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
 * @param dev UART device struct (of type struct uart_device_config)
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
 * @param dev UART device struct (of type struct uart_device_config)
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
 * @param dev UART device struct (of type struct uart_device_config)
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
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_ns16550_irq_is_pending(struct device *dev)
{
	return (!(IIRC(dev) & IIR_IP));
}

/**
 * @brief Update cached contents of IIR
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return Always 1
 */
static int uart_ns16550_irq_update(struct device *dev)
{
	IIRC(dev) = INBYTE(IIR(dev));

	return 1;
}

/**
 * @brief Returns UART interrupt number
 *
 * Returns the IRQ number used by the specified UART port
 *
 * @param dev UART device struct (of type struct uart_device_config)
 *
 * @return IRQ number
 */
static unsigned int uart_ns16550_irq_get(struct device *dev)
{
	return (unsigned int)DEV_CFG(dev)->irq;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


static struct uart_driver_api uart_ns16550_driver_api = {
	.poll_in = uart_ns16550_poll_in,
	.poll_out = uart_ns16550_poll_out,

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	.fifo_fill = uart_ns16550_fifo_fill,
	.fifo_read = uart_ns16550_fifo_read,
	.irq_tx_enable = uart_ns16550_irq_tx_enable,
	.irq_tx_disable = uart_ns16550_irq_tx_disable,
	.irq_tx_ready = uart_ns16550_irq_tx_ready,
	.irq_rx_enable = uart_ns16550_irq_rx_enable,
	.irq_rx_disable = uart_ns16550_irq_rx_disable,
	.irq_rx_ready = uart_ns16550_irq_rx_ready,
	.irq_err_enable = uart_ns16550_irq_err_enable,
	.irq_err_disable = uart_ns16550_irq_err_disable,
	.irq_is_pending = uart_ns16550_irq_is_pending,
	.irq_update = uart_ns16550_irq_update,
	.irq_get = uart_ns16550_irq_get,

#endif
};

#ifdef CONFIG_UART_NS16550_PORT_0

struct uart_device_config uart_ns16550_dev_cfg_0 = {
	.port = CONFIG_UART_NS16550_PORT_0_BASE_ADDR,
	.irq = CONFIG_UART_NS16550_PORT_0_IRQ,
	.irq_pri = CONFIG_UART_NS16550_PORT_0_IRQ_PRI,

	.init_info.baud_rate = CONFIG_UART_NS16550_PORT_0_BAUD_RATE,
	.init_info.sys_clk_freq = CONFIG_UART_NS16550_PORT_0_CLK_FREQ,
	.init_info.options = CONFIG_UART_NS16550_PORT_0_OPTIONS,

#ifdef CONFIG_UART_NS16550_PORT_0_PCI
	.pci_dev.class_type = CONFIG_UART_NS16550_PORT_0_PCI_CLASS,
	.pci_dev.bus = CONFIG_UART_NS16550_PORT_0_PCI_BUS,
	.pci_dev.dev = CONFIG_UART_NS16550_PORT_0_PCI_DEV,
	.pci_dev.vendor_id = CONFIG_UART_NS16550_PORT_0_PCI_VENDOR_ID,
	.pci_dev.device_id = CONFIG_UART_NS16550_PORT_0_PCI_DEVICE_ID,
	.pci_dev.function = CONFIG_UART_NS16550_PORT_0_PCI_FUNC,
	.pci_dev.bar = CONFIG_UART_NS16550_PORT_0_PCI_BAR,
#endif /* CONFIG_UART_NS16550_PORT_0_PCI */
};

static struct uart_ns16550_dev_data_t uart_ns16550_dev_data_0;

DECLARE_DEVICE_INIT_CONFIG(uart_ns16550_0,
			   CONFIG_UART_NS16550_PORT_0_NAME,
			   &uart_ns16550_init,
			   &uart_ns16550_dev_cfg_0);

SYS_DEFINE_DEVICE(uart_ns16550_0, &uart_ns16550_dev_data_0,
		  PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_UART_NS16550_PORT_0 */

#ifdef CONFIG_UART_NS16550_PORT_1

struct uart_device_config uart_ns16550_dev_cfg_1 = {
	.port = CONFIG_UART_NS16550_PORT_1_BASE_ADDR,
	.irq = CONFIG_UART_NS16550_PORT_1_IRQ,
	.irq_pri = CONFIG_UART_NS16550_PORT_1_IRQ_PRI,

	.init_info.baud_rate = CONFIG_UART_NS16550_PORT_1_BAUD_RATE,
	.init_info.sys_clk_freq = CONFIG_UART_NS16550_PORT_1_CLK_FREQ,
	.init_info.options = CONFIG_UART_NS16550_PORT_1_OPTIONS,

#ifdef CONFIG_UART_NS16550_PORT_1_PCI
	.pci_dev.class_type = CONFIG_UART_NS16550_PORT_1_PCI_CLASS,
	.pci_dev.bus = CONFIG_UART_NS16550_PORT_1_PCI_BUS,
	.pci_dev.dev = CONFIG_UART_NS16550_PORT_1_PCI_DEV,
	.pci_dev.vendor_id = CONFIG_UART_NS16550_PORT_1_PCI_VENDOR_ID,
	.pci_dev.device_id = CONFIG_UART_NS16550_PORT_1_PCI_DEVICE_ID,
	.pci_dev.function = CONFIG_UART_NS16550_PORT_1_PCI_FUNC,
	.pci_dev.bar = CONFIG_UART_NS16550_PORT_1_PCI_BAR,
#endif /* CONFIG_UART_NS16550_PORT_1_PCI */
};

static struct uart_ns16550_dev_data_t uart_ns16550_dev_data_1;

DECLARE_DEVICE_INIT_CONFIG(uart_ns16550_1,
			   CONFIG_UART_NS16550_PORT_1_NAME,
			   &uart_ns16550_init,
			   &uart_ns16550_dev_cfg_1);

SYS_DEFINE_DEVICE(uart_ns16550_1, &uart_ns16550_dev_data_1,
		  PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_UART_NS16550_PORT_1 */
