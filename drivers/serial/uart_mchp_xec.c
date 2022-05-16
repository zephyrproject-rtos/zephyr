/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2020 Intel Corp.
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Microchip XEC UART Serial Driver
 *
 * This is the driver for the Microchip XEC MCU UART. It is NS16550 compatible.
 *
 */

#define DT_DRV_COMPAT microchip_xec_uart

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/types.h>
#include <soc.h>

#include <zephyr/init.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/spinlock.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_SOC_SERIES_MEC172X),
	     "XEC UART driver only support MEC172x at this time");

/* Clock source is 1.8432 MHz derived from PLL 48 MHz */
#define XEC_UART_CLK_SRC_1P8M		0
/* Clock source is PLL 48 MHz output */
#define XEC_UART_CLK_SRC_48M		1
/* Clock source is the UART_CLK alternate pin function. */
#define XEC_UART_CLK_SRC_EXT_PIN	2

/* register definitions */

#define REG_THR 0x00  /* Transmitter holding reg.       */
#define REG_RDR 0x00  /* Receiver data reg.             */
#define REG_BRDL 0x00 /* Baud rate divisor (LSB)        */
#define REG_BRDH 0x01 /* Baud rate divisor (MSB)        */
#define REG_IER 0x01  /* Interrupt enable reg.          */
#define REG_IIR 0x02  /* Interrupt ID reg.              */
#define REG_FCR 0x02  /* FIFO control reg.              */
#define REG_LCR 0x03  /* Line control reg.              */
#define REG_MDC 0x04  /* Modem control reg.             */
#define REG_LSR 0x05  /* Line status reg.               */
#define REG_MSR 0x06  /* Modem status reg.              */
#define REG_SCR 0x07  /* scratch register               */
#define REG_LD_ACTV 0x330 /* Logical Device activate    */
#define REG_LD_CFG 0x3f0 /* Logical Device configuration */

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

#define IIRC(dev) (((struct uart_xec_dev_data *)(dev)->data)->iir_cache)

/* device config */
struct uart_xec_device_config {
	struct uart_regs *regs;
	uint32_t sys_clk_freq;
	uint8_t girq_id;
	uint8_t girq_pos;
	uint8_t pcr_idx;
	uint8_t pcr_bitpos;
	const struct pinctrl_dev_config *pcfg;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t	irq_config_func;
#endif
};

/** Device data structure */
struct uart_xec_dev_data {
	struct uart_config uart_config;
	struct k_spinlock lock;

	uint8_t fcr_cache;	/**< cache of FCR write only register */
	uint8_t iir_cache;	/**< cache of IIR since it clears when read */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;  /**< Callback function pointer */
	void *cb_data;	/**< Callback function arg */
#endif
};

static const struct uart_driver_api uart_xec_driver_api;

static void set_baud_rate(const struct device *dev, uint32_t baud_rate)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data * const dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	uint32_t divisor; /* baud rate divisor */
	uint8_t lcr_cache;

	if ((baud_rate != 0U) && (dev_cfg->sys_clk_freq != 0U)) {
		/*
		 * calculate baud rate divisor. a variant of
		 * (uint32_t)(dev_cfg->sys_clk_freq / (16.0 * baud_rate) + 0.5)
		 */
		divisor = ((dev_cfg->sys_clk_freq + (baud_rate << 3))
					/ baud_rate) >> 4;

		/* set the DLAB to access the baud rate divisor registers */
		lcr_cache = regs->LCR;
		regs->LCR = LCR_DLAB | lcr_cache;
		regs->RTXB = (unsigned char)(divisor & 0xff);
		/* bit[7]=0 1.8MHz clock source, =1 48MHz clock source */
		regs->IER = (unsigned char)((divisor >> 8) & 0x7f);

		/* restore the DLAB to access the baud rate divisor registers */
		regs->LCR = lcr_cache;

		dev_data->uart_config.baudrate = baud_rate;
	}
}

/*
 * Configure UART.
 * MCHP XEC UART defaults to reset if external Host VCC_PWRGD is inactive.
 * We must change the UART reset signal to XEC VTR_PWRGD. Make sure UART
 * clock source is an internal clock and UART pins are not inverted.
 */
static int uart_xec_configure(const struct device *dev,
			      const struct uart_config *cfg)
{
	struct uart_xec_dev_data * const dev_data = dev->data;
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_regs *regs = dev_cfg->regs;
	uint8_t lcr_cache;

	/* temp for return value if error occurs in this locked region */
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	ARG_UNUSED(dev_data);

	dev_data->fcr_cache = 0U;
	dev_data->iir_cache = 0U;

	/* XEC UART specific configuration and enable */
	regs->CFG_SEL &= ~(MCHP_UART_LD_CFG_RESET_VCC |
			   MCHP_UART_LD_CFG_EXTCLK | MCHP_UART_LD_CFG_INVERT);
	/* set activate to enable clocks */
	regs->ACTV |= MCHP_UART_LD_ACTIVATE;

	set_baud_rate(dev, cfg->baudrate);

	/* Local structure to hold temporary values */
	struct uart_config uart_cfg;

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		uart_cfg.data_bits = LCR_CS5;
		break;
	case UART_CFG_DATA_BITS_6:
		uart_cfg.data_bits = LCR_CS6;
		break;
	case UART_CFG_DATA_BITS_7:
		uart_cfg.data_bits = LCR_CS7;
		break;
	case UART_CFG_DATA_BITS_8:
		uart_cfg.data_bits = LCR_CS8;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uart_cfg.stop_bits = LCR_1_STB;
		break;
	case UART_CFG_STOP_BITS_2:
		uart_cfg.stop_bits = LCR_2_STB;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uart_cfg.parity = LCR_PDIS;
		break;
	case UART_CFG_PARITY_EVEN:
		uart_cfg.parity = LCR_EPS;
		break;
	default:
		ret = -ENOTSUP;
		goto out;
	}

	dev_data->uart_config = *cfg;

	/* data bits, stop bits, parity, clear DLAB */
	regs->LCR = uart_cfg.data_bits | uart_cfg.stop_bits | uart_cfg.parity;

	regs->MCR = MCR_OUT2 | MCR_RTS | MCR_DTR;

	/*
	 * Program FIFO: enabled, mode 0
	 * generate the interrupt at 8th byte
	 * Clear TX and RX FIFO
	 */
	dev_data->fcr_cache = FCR_FIFO | FCR_MODE0 | FCR_FIFO_8 | FCR_RCVRCLR |
			      FCR_XMITCLR;
	regs->IIR_FCR = dev_data->fcr_cache;

	/* clear the port */
	lcr_cache = regs->LCR;
	regs->LCR = LCR_DLAB | lcr_cache;
	regs->SCR = regs->RTXB;
	regs->LCR = lcr_cache;

	/* disable interrupts  */
	regs->IER = 0;

out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
};

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_xec_config_get(const struct device *dev,
			       struct uart_config *cfg)
{
	struct uart_xec_dev_data *data = dev->data;

	cfg->baudrate = data->uart_config.baudrate;
	cfg->parity = data->uart_config.parity;
	cfg->stop_bits = data->uart_config.stop_bits;
	cfg->data_bits = data->uart_config.data_bits;
	cfg->flow_ctrl = data->uart_config.flow_ctrl;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

/**
 * @brief Initialize individual UART port
 *
 * This routine is called to reset the chip in a quiescent state.
 *
 * @param dev UART device struct
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_xec_init(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	int ret;

	ret = z_mchp_xec_pcr_periph_sleep(dev_cfg->pcr_idx,
					  dev_cfg->pcr_bitpos, 0);
	if (ret != 0) {
		return ret;
	}

	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = uart_xec_configure(dev, &dev_data->uart_config);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_cfg->irq_config_func(dev);
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
static int uart_xec_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	int ret = -1;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if ((regs->LSR & LSR_RXRDY) != 0) {
		/* got a character */
		*c = regs->RTXB;
		ret = 0;
	}

	k_spin_unlock(&dev_data->lock, key);

	return ret;
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
 */
static void uart_xec_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	while ((regs->LSR & LSR_THRE) == 0) {
		;
	}

	regs->RTXB = c;

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Check if an error was received
 *
 * @param dev UART device struct
 *
 * @return one of UART_ERROR_OVERRUN, UART_ERROR_PARITY, UART_ERROR_FRAMING,
 * UART_BREAK if an error was detected, 0 otherwise.
 */
static int uart_xec_err_check(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);
	int check = regs->LSR & LSR_EOB_MASK;

	k_spin_unlock(&dev_data->lock, key);

	return check >> 1;
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
static int uart_xec_fifo_fill(const struct device *dev, const uint8_t *tx_data,
			      int size)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	int i;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	for (i = 0; (i < size) && (regs->LSR & LSR_THRE) != 0; i++) {
		regs->RTXB = tx_data[i];
	}

	k_spin_unlock(&dev_data->lock, key);

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
static int uart_xec_fifo_read(const struct device *dev, uint8_t *rx_data,
			      const int size)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	int i;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	for (i = 0; (i < size) && (regs->LSR & LSR_RXRDY) != 0; i++) {
		rx_data[i] = regs->RTXB;
	}

	k_spin_unlock(&dev_data->lock, key);

	return i;
}

/**
 * @brief Enable TX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_xec_irq_tx_enable(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	regs->IER |= IER_TBE;

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_xec_irq_tx_disable(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	regs->IER &= ~(IER_TBE);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_xec_irq_tx_ready(const struct device *dev)
{
	struct uart_xec_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	int ret = ((IIRC(dev) & IIR_ID) == IIR_THRE) ? 1 : 0;

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_xec_irq_tx_complete(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	int ret = ((regs->LSR & (LSR_TEMT | LSR_THRE))
				== (LSR_TEMT | LSR_THRE)) ? 1 : 0;

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_xec_irq_rx_enable(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	regs->IER |= IER_RXRDY;

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_xec_irq_rx_disable(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	regs->IER &= ~(IER_RXRDY);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_xec_irq_rx_ready(const struct device *dev)
{
	struct uart_xec_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	int ret = ((IIRC(dev) & IIR_ID) == IIR_RBRF) ? 1 : 0;

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/**
 * @brief Enable error interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_xec_irq_err_enable(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	regs->IER |= IER_LSR;

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Disable error interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static void uart_xec_irq_err_disable(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	regs->IER &= ~(IER_LSR);

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_xec_irq_is_pending(const struct device *dev)
{
	struct uart_xec_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	int ret = (!(IIRC(dev) & IIR_NIP)) ? 1 : 0;

	k_spin_unlock(&dev_data->lock, key);

	return ret;
}

/**
 * @brief Update cached contents of IIR
 *
 * @param dev UART device struct
 *
 * @return Always 1
 */
static int uart_xec_irq_update(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	IIRC(dev) = regs->IIR_FCR;

	k_spin_unlock(&dev_data->lock, key);

	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 */
static void uart_xec_irq_callback_set(const struct device *dev,
				      uart_irq_callback_user_data_t cb,
				      void *cb_data)
{
	struct uart_xec_dev_data * const dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;

	k_spin_unlock(&dev_data->lock, key);
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 */
static void uart_xec_isr(const struct device *dev)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data * const dev_data = dev->data;

	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}

	/* clear ECIA GIRQ R/W1C status bit after UART status cleared */
	mchp_xec_ecia_girq_src_clr(dev_cfg->girq_id, dev_cfg->girq_pos);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_XEC_LINE_CTRL

/**
 * @brief Manipulate line control for UART.
 *
 * @param dev UART device struct
 * @param ctrl The line control to be manipulated
 * @param val Value to set the line control
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_xec_line_ctrl_set(const struct device *dev,
				  uint32_t ctrl, uint32_t val)
{
	const struct uart_xec_device_config * const dev_cfg = dev->config;
	struct uart_xec_dev_data *dev_data = dev->data;
	struct uart_regs *regs = dev_cfg->regs;
	uint32_t mdc, chg;
	k_spinlock_key_t key;

	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		set_baud_rate(dev, val);
		return 0;

	case UART_LINE_CTRL_RTS:
	case UART_LINE_CTRL_DTR:
		key = k_spin_lock(&dev_data->lock);
		mdc = regs->MCR;

		if (ctrl == UART_LINE_CTRL_RTS) {
			chg = MCR_RTS;
		} else {
			chg = MCR_DTR;
		}

		if (val) {
			mdc |= chg;
		} else {
			mdc &= ~(chg);
		}
		regs->MCR = mdc;
		k_spin_unlock(&dev_data->lock, key);
		return 0;
	}

	return -ENOTSUP;
}

#endif /* CONFIG_UART_XEC_LINE_CTRL */

static const struct uart_driver_api uart_xec_driver_api = {
	.poll_in = uart_xec_poll_in,
	.poll_out = uart_xec_poll_out,
	.err_check = uart_xec_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_xec_configure,
	.config_get = uart_xec_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN

	.fifo_fill = uart_xec_fifo_fill,
	.fifo_read = uart_xec_fifo_read,
	.irq_tx_enable = uart_xec_irq_tx_enable,
	.irq_tx_disable = uart_xec_irq_tx_disable,
	.irq_tx_ready = uart_xec_irq_tx_ready,
	.irq_tx_complete = uart_xec_irq_tx_complete,
	.irq_rx_enable = uart_xec_irq_rx_enable,
	.irq_rx_disable = uart_xec_irq_rx_disable,
	.irq_rx_ready = uart_xec_irq_rx_ready,
	.irq_err_enable = uart_xec_irq_err_enable,
	.irq_err_disable = uart_xec_irq_err_disable,
	.irq_is_pending = uart_xec_irq_is_pending,
	.irq_update = uart_xec_irq_update,
	.irq_callback_set = uart_xec_irq_callback_set,

#endif

#ifdef CONFIG_UART_XEC_LINE_CTRL
	.line_ctrl_set = uart_xec_line_ctrl_set,
#endif
};

#define DEV_CONFIG_REG_INIT(n)						\
	.regs = (struct uart_regs *)(DT_INST_REG_ADDR(n)),

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define DEV_CONFIG_IRQ_FUNC_INIT(n) \
	.irq_config_func = irq_config_func##n,
#define UART_XEC_IRQ_FUNC_DECLARE(n) \
	static void irq_config_func##n(const struct device *dev);
#define UART_XEC_IRQ_FUNC_DEFINE(n)					\
	static void irq_config_func##n(const struct device *dev)	\
	{								\
		ARG_UNUSED(dev);					\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			    uart_xec_isr, DEVICE_DT_INST_GET(n),	\
			    0);						\
		irq_enable(DT_INST_IRQN(n));				\
		mchp_xec_ecia_girq_src_en(DT_INST_PROP_BY_IDX(n, girqs, 0), \
					  DT_INST_PROP_BY_IDX(n, girqs, 1)); \
	}
#else
/* !CONFIG_UART_INTERRUPT_DRIVEN */
#define DEV_CONFIG_IRQ_FUNC_INIT(n)
#define UART_XEC_IRQ_FUNC_DECLARE(n)
#define UART_XEC_IRQ_FUNC_DEFINE(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define DEV_DATA_FLOW_CTRL(n)						\
	DT_INST_PROP_OR(n, hw_flow_control, UART_CFG_FLOW_CTRL_NONE)

#define UART_XEC_DEVICE_INIT(n)						\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	UART_XEC_IRQ_FUNC_DECLARE(n);					\
									\
	static const struct uart_xec_device_config uart_xec_dev_cfg_##n = { \
		DEV_CONFIG_REG_INIT(n)					\
		.sys_clk_freq = DT_INST_PROP(n, clock_frequency),	\
		.girq_id = DT_INST_PROP_BY_IDX(n, girqs, 0),		\
		.girq_pos = DT_INST_PROP_BY_IDX(n, girqs, 1),		\
		.pcr_idx = DT_INST_PROP_BY_IDX(n, pcrs, 0),		\
		.pcr_bitpos = DT_INST_PROP_BY_IDX(n, pcrs, 1),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
		DEV_CONFIG_IRQ_FUNC_INIT(n)				\
	};								\
	static struct uart_xec_dev_data uart_xec_dev_data_##n = {	\
		.uart_config.baudrate = DT_INST_PROP_OR(n, current_speed, 0), \
		.uart_config.parity = UART_CFG_PARITY_NONE,		\
		.uart_config.stop_bits = UART_CFG_STOP_BITS_1,		\
		.uart_config.data_bits = UART_CFG_DATA_BITS_8,		\
		.uart_config.flow_ctrl = DEV_DATA_FLOW_CTRL(n),		\
	};								\
	DEVICE_DT_INST_DEFINE(n, &uart_xec_init, NULL,			\
			      &uart_xec_dev_data_##n,			\
			      &uart_xec_dev_cfg_##n,			\
			      PRE_KERNEL_1,				\
			      CONFIG_SERIAL_INIT_PRIORITY,		\
			      &uart_xec_driver_api);			\
	UART_XEC_IRQ_FUNC_DEFINE(n)

DT_INST_FOREACH_STATUS_OKAY(UART_XEC_DEVICE_INIT)
