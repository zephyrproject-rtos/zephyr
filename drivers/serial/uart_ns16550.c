/* ns16550.c - NS16550D serial driver */

#define DT_DRV_COMPAT ns16550

/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2020-2022 Intel Corp.
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
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/types.h>

#include <zephyr/init.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>

#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif

#include <zephyr/drivers/serial/uart_ns16550.h>

#define INST_HAS_PCP_HELPER(inst) DT_INST_NODE_HAS_PROP(inst, pcp) ||
#define INST_HAS_DLF_HELPER(inst) DT_INST_NODE_HAS_PROP(inst, dlf) ||

#define UART_NS16550_PCP_ENABLED \
	(DT_INST_FOREACH_STATUS_OKAY(INST_HAS_PCP_HELPER) 0)
#define UART_NS16550_DLF_ENABLED \
	(DT_INST_FOREACH_STATUS_OKAY(INST_HAS_DLF_HELPER) 0)

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
BUILD_ASSERT(IS_ENABLED(CONFIG_PCIE), "NS16550(s) in DT need CONFIG_PCIE");
#include <zephyr/drivers/pcie/pcie.h>
#endif

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
#define REG_SCR 0x07  /* Scratchpad.			*/
#define REG_DLF 0xC0  /* Divisor Latch Fraction         */
#define REG_PCP 0x200 /* PRV_CLOCK_PARAMS (Apollo Lake) */

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
#define IIR_FE    0xC0 /* FIFO mode enabled */
#define IIR_CH    0x0C /* Character timeout*/

/* equates for FIFO control register */

#define FCR_FIFO 0x01    /* enable XMIT and RCVR FIFO */
#define FCR_RCVRCLR 0x02 /* clear RCVR FIFO */
#define FCR_XMITCLR 0x04 /* clear XMIT FIFO */

/* equates for Apollo Lake clock control register (PRV_CLOCK_PARAMS) */

#define PCP_UPDATE 0x80000000 /* update clock */
#define PCP_EN 0x00000001     /* enable clock output */

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

/* FIFO Depth. */
#if defined(CONFIG_UART_NS16550_VARIANT_NS16750)
#define UART_FIFO_DEPTH (64)
#elif defined(CONFIG_UART_NS16550_VARIANT_NS16950)
#define UART_FIFO_DEPTH (128)
#else
#define UART_FIFO_DEPTH (16)
#endif

/* FIFO Half Depth. */
#define UART_FIFO_HALF_DEPTH (UART_FIFO_DEPTH / 2)

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

/* Transfer Error */
#define UART_PASS 0 /* Transfer completed */
#define UART_ERROR_CANCELED -ECANCELED /* Operation was canceled */
#define UART_DRIVER_ERROR  -ENOTSUP /* Unsupported error */

/* Transfer Status */
#define UART_TRANSFER_SUCCESS 0 /* Transfer success */
#define UART_TRANSFER_FAILED -EPERM /* Transfer failed */

/* SCR bit to indicate updated status for LSR. */
#define UART_SCR_STATUS_UPDATE BIT(0)

/* constants for modem status register */

#define MSR_DCTS 0x01 /* cts change */
#define MSR_DDSR 0x02 /* dsr change */
#define MSR_DRI 0x04  /* ring change */
#define MSR_DDCD 0x08 /* data carrier change */
#define MSR_CTS 0x10  /* complement of cts */
#define MSR_DSR 0x20  /* complement of dsr */
#define MSR_RI 0x40   /* complement of ring signal */
#define MSR_DCD 0x80  /* complement of dcd */

#define THR(dev) (get_port(dev) + REG_THR * reg_interval(dev))
#define RDR(dev) (get_port(dev) + REG_RDR * reg_interval(dev))
#define BRDL(dev) (get_port(dev) + REG_BRDL * reg_interval(dev))
#define BRDH(dev) (get_port(dev) + REG_BRDH * reg_interval(dev))
#define IER(dev) (get_port(dev) + REG_IER * reg_interval(dev))
#define IIR(dev) (get_port(dev) + REG_IIR * reg_interval(dev))
#define FCR(dev) (get_port(dev) + REG_FCR * reg_interval(dev))
#define LCR(dev) (get_port(dev) + REG_LCR * reg_interval(dev))
#define MDC(dev) (get_port(dev) + REG_MDC * reg_interval(dev))
#define LSR(dev) (get_port(dev) + REG_LSR * reg_interval(dev))
#define MSR(dev) (get_port(dev) + REG_MSR * reg_interval(dev))
#define SCR(dev) (get_port(dev) + REG_SCR * reg_interval(dev))
#define DLF(dev) (get_port(dev) + REG_DLF)
#define PCP(dev) (get_port(dev) + REG_PCP)

#define IIRC(dev) (((struct uart_ns16550_dev_data *)(dev)->data)->iir_cache)

#if CONFIG_UART_ASYNC_API
/* Macro to get tx semamphore */
#define GET_TX_SEM(dev)                           \
	(((const struct uart_ns16550_device_config *) \
	  dev->config)->tx_sem)

/* Macro to get rx sempahore */
#define GET_RX_SEM(dev)                           \
	(((const struct uart_ns16550_device_config *) \
	  dev->config)->rx_sem)
/**
 * UART asynchronous transfer structure.
 */
struct uart_ns16550_transfer_t {
	uint8_t *data;     /**< Pre-allocated write or read buffer. */
	uint32_t data_len; /**< Number of bytes to transfer. */

	/** Transfer callback
	 *
	 * @param[in] data Callback user data.
	 * @param[in] UART_PASS on success,negative value for possible
	 * errors.
	 * @param[in] status UART module status.To be interpreted with
	 * intel_uart_status_t for different error bits.
	 * @param[in] len Length of the UART transfer if successful, 0
	 * otherwise.
	 */
	void (*callback)(void *data, int error, uint32_t status, uint32_t len);
	void *callback_data; /**< Callback identifier. */
};
#endif /* CONFIG_UART_ASYNC_API */

/* device config */
struct uart_ns16550_device_config {
	union {
		DEVICE_MMIO_ROM;
		uint32_t port;
	};
	uint32_t sys_clk_freq;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t	irq_config_func;
#endif
#if UART_NS16550_PCP_ENABLED
	uint32_t pcp;
#endif
	uint8_t reg_interval;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	struct pcie_dev *pcie;
#endif
#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pincfg;
#endif

#ifdef CONFIG_UART_ASYNC_API
	struct k_sem *tx_sem;
	struct k_sem *rx_sem;
#endif

#if defined(CONFIG_UART_NS16550_ACCESS_IOPORT) || defined(CONFIG_UART_NS16550_SIMULT_ACCESS)
	bool io_map;
#endif
};

/** Device data structure */
struct uart_ns16550_dev_data {
	DEVICE_MMIO_RAM;
	struct uart_config uart_config;
	struct k_spinlock lock;
	uint8_t fifo_size;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uint8_t iir_cache;	/**< cache of IIR since it clears when read */
	uart_irq_callback_user_data_t cb;  /**< Callback function pointer */
	void *cb_data;	/**< Callback function arg */
#endif

#if UART_NS16550_DLF_ENABLED
	uint8_t dlf;		/**< DLF value */
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_PM)
	bool tx_stream_on;
#endif


#ifdef CONFIG_UART_ASYNC_API
	struct uart_event evt;
	uart_callback_t async_cb;
	void *async_user_data;
	struct uart_ns16550_transfer_t *rx_transfer;
	struct uart_ns16550_transfer_t *tx_transfer;
	uint32_t write_pos;
	uint32_t read_pos;
#endif
};



static void ns16550_outbyte(const struct uart_ns16550_device_config *cfg,
			    uintptr_t port, uint8_t val)
{
#if defined(CONFIG_UART_NS16550_ACCESS_IOPORT) || defined(CONFIG_UART_NS16550_SIMULT_ACCESS)
	if (cfg->io_map) {
		if (IS_ENABLED(CONFIG_UART_NS16550_ACCESS_WORD_ONLY)) {
			sys_out32(val, port);
		} else {
			sys_out8(val, port);
		}
	} else {
#else
	{
#endif
		/* MMIO mapped */
		if (IS_ENABLED(CONFIG_UART_NS16550_ACCESS_WORD_ONLY)) {
			sys_write32(val, port);
		} else {
			sys_write8(val, port);
		}
	}
}

static uint8_t ns16550_inbyte(const struct uart_ns16550_device_config *cfg,
			      uintptr_t port)
{
#if defined(CONFIG_UART_NS16550_ACCESS_IOPORT) || defined(CONFIG_UART_NS16550_SIMULT_ACCESS)
	if (cfg->io_map) {
		if (IS_ENABLED(CONFIG_UART_NS16550_ACCESS_WORD_ONLY)) {
			return sys_in32(port);
		} else {
			return sys_in8(port);
		}
	} else {
#else
	{
#endif
		/* MMIO mapped */
		if (IS_ENABLED(CONFIG_UART_NS16550_ACCESS_WORD_ONLY)) {
			return sys_read32(port);
		} else {
			return sys_read8(port);
		}
	}
	return 0;
}

#if UART_NS16550_PCP_ENABLED
static void ns16550_outword(const struct uart_ns16550_device_config *cfg,
			    uintptr_t port, uint32_t val)
{
#if defined(CONFIG_UART_NS16550_ACCESS_IOPORT) || defined(CONFIG_UART_NS16550_SIMULT_ACCESS)
	if (cfg->io_map) {
		sys_out32(val, port);
	} else {
#else
	{
#endif
		/* MMIO mapped */
		sys_write32(val, port);
	}
}

static uint32_t ns16550_inword(const struct uart_ns16550_device_config *cfg,
			      uintptr_t port)
{
#if defined(CONFIG_UART_NS16550_ACCESS_IOPORT) || defined(CONFIG_UART_NS16550_SIMULT_ACCESS)
	if (cfg->io_map) {
		return sys_in32(port);
	}
#endif
	/* MMIO mapped */
	return sys_read32(port);
}
#endif

static inline uint8_t reg_interval(const struct device *dev)
{
	const struct uart_ns16550_device_config *config = dev->config;

	return config->reg_interval;
}

static const struct uart_driver_api uart_ns16550_driver_api;

static inline uintptr_t get_port(const struct device *dev)
{
	uintptr_t port;
#if defined(CONFIG_UART_NS16550_ACCESS_IOPORT) || defined(CONFIG_UART_NS16550_SIMULT_ACCESS)
	const struct uart_ns16550_device_config *config = dev->config;

	if (config->io_map) {
		port = config->port;
	} else {
#else
	{
#endif
		port = DEVICE_MMIO_GET(dev);
	}

	return port;
}

static void set_baud_rate(const struct device *dev, uint32_t baud_rate, uint32_t pclk)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	uint32_t divisor; /* baud rate divisor */
	uint8_t lcr_cache;

	if ((baud_rate != 0U) && (pclk != 0U)) {
		/*
		 * calculate baud rate divisor. a variant of
		 * (uint32_t)(pclk / (16.0 * baud_rate) + 0.5)
		 */
		divisor = ((pclk + (baud_rate << 3))
					/ baud_rate) >> 4;

		/* set the DLAB to access the baud rate divisor registers */
		lcr_cache = ns16550_inbyte(dev_cfg, LCR(dev));
		ns16550_outbyte(dev_cfg, LCR(dev), LCR_DLAB | lcr_cache);
		ns16550_outbyte(dev_cfg, BRDL(dev), (unsigned char)(divisor & 0xff));
		ns16550_outbyte(dev_cfg, BRDH(dev), (unsigned char)((divisor >> 8) & 0xff));

		/* restore the DLAB to access the baud rate divisor registers */
		ns16550_outbyte(dev_cfg, LCR(dev), lcr_cache);

		dev_data->uart_config.baudrate = baud_rate;
	}
}

static int uart_ns16550_configure(const struct device *dev,
				  const struct uart_config *cfg)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	uint8_t mdc = 0U;
	uint32_t pclk = 0U;

	/* temp for return value if error occurs in this locked region */
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

#if defined(CONFIG_PINCTRL)
	if (dev_cfg->pincfg != NULL) {
		pinctrl_apply_state(dev_cfg->pincfg, PINCTRL_STATE_DEFAULT);
	}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_data->iir_cache = 0U;
#endif

#if UART_NS16550_DLF_ENABLED
	ns16550_outbyte(dev_cfg, DLF(dev), dev_data->dlf);
#endif

#if UART_NS16550_PCP_ENABLED
	uint32_t pcp = dev_cfg->pcp;

	if (pcp) {
		pcp |= PCP_EN;
		ns16550_outbyte(dev_cfg, PCP(dev), pcp & ~PCP_UPDATE);
		ns16550_outbyte(dev_cfg, PCP(dev), pcp | PCP_UPDATE);
	}
#endif

	/*
	 * set clock frequency from clock_frequency property if valid,
	 * otherwise, get clock frequency from clock manager
	 */
	if (dev_cfg->sys_clk_freq != 0U) {
		pclk = dev_cfg->sys_clk_freq;
	} else {
		if (!device_is_ready(dev_cfg->clock_dev)) {
			ret = -EINVAL;
			goto out;
		}

		clock_control_get_rate(dev_cfg->clock_dev, dev_cfg->clock_subsys,
				       &pclk);
	}

	set_baud_rate(dev, cfg->baudrate, pclk);

	/* Local structure to hold temporary values to pass to ns16550_outbyte() */
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
	ns16550_outbyte(dev_cfg, LCR(dev),
			uart_cfg.data_bits | uart_cfg.stop_bits | uart_cfg.parity);

	mdc = MCR_OUT2 | MCR_RTS | MCR_DTR;
#if defined(CONFIG_UART_NS16550_VARIANT_NS16750) || \
	defined(CONFIG_UART_NS16550_VARIANT_NS16950)
	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		mdc |= MCR_AFCE;
	}
#endif

	ns16550_outbyte(dev_cfg, MDC(dev), mdc);

	/*
	 * Program FIFO: enabled, mode 0 (set for compatibility with quark),
	 * generate the interrupt at 8th byte
	 * Clear TX and RX FIFO
	 */
	ns16550_outbyte(dev_cfg, FCR(dev),
			FCR_FIFO | FCR_MODE0 | FCR_FIFO_8 | FCR_RCVRCLR | FCR_XMITCLR
#ifdef CONFIG_UART_NS16550_VARIANT_NS16750
			| FCR_FIFO_64
#endif
			);

	if ((ns16550_inbyte(dev_cfg, IIR(dev)) & IIR_FE) == IIR_FE) {
#ifdef CONFIG_UART_NS16550_VARIANT_NS16750
		dev_data->fifo_size = 64;
#elif defined(CONFIG_UART_NS16550_VARIANT_NS16950)
		dev_data->fifo_size = 128;
#else
		dev_data->fifo_size = 16;
#endif
	} else {
		dev_data->fifo_size = 1;
	}

	/* clear the port */
	ns16550_inbyte(dev_cfg, RDR(dev));

	/* disable interrupts  */
	ns16550_outbyte(dev_cfg, IER(dev), 0x00);

out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
};

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_ns16550_config_get(const struct device *dev,
				   struct uart_config *cfg)
{
	struct uart_ns16550_dev_data *data = dev->data;

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
static int uart_ns16550_init(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config *dev_cfg = dev->config;
	int ret;

	ARG_UNUSED(dev_cfg);

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	if (dev_cfg->pcie) {
		struct pcie_bar mbar;

		if (dev_cfg->pcie->bdf == PCIE_BDF_NONE) {
			return -EINVAL;
		}

		pcie_probe_mbar(dev_cfg->pcie->bdf, 0, &mbar);
		pcie_set_cmd(dev_cfg->pcie->bdf, PCIE_CONF_CMDSTAT_MEM, true);

		device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr, mbar.size,
			   K_MEM_CACHE_NONE);
	} else
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie) */
	{
#if defined(CONFIG_UART_NS16550_ACCESS_IOPORT) || defined(CONFIG_UART_NS16550_SIMULT_ACCESS)
		/* Map directly from DTS */
		if (!dev_cfg->io_map) {
#else
		{
#endif
			DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
		}
	}

	ret = uart_ns16550_configure(dev, &data->uart_config);
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
static int uart_ns16550_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	int ret = -1;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if ((ns16550_inbyte(dev_cfg, LSR(dev)) & LSR_RXRDY) != 0) {
		/* got a character */
		*c = ns16550_inbyte(dev_cfg, RDR(dev));
		ret = 0;
	}

	k_spin_unlock(&data->lock, key);

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
static void uart_ns16550_poll_out(const struct device *dev,
					   unsigned char c)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while ((ns16550_inbyte(dev_cfg, LSR(dev)) & LSR_THRE) == 0) {
	}

	ns16550_outbyte(dev_cfg, THR(dev), c);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if an error was received
 *
 * @param dev UART device struct
 *
 * @return one of UART_ERROR_OVERRUN, UART_ERROR_PARITY, UART_ERROR_FRAMING,
 * UART_BREAK if an error was detected, 0 otherwise.
 */
static int uart_ns16550_err_check(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int check = (ns16550_inbyte(dev_cfg, LSR(dev)) & LSR_EOB_MASK);

	k_spin_unlock(&data->lock, key);

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
static int uart_ns16550_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int size)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	int i;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	for (i = 0; (i < size) && (i < data->fifo_size); i++) {
		ns16550_outbyte(dev_cfg, THR(dev), tx_data[i]);
	}

	k_spin_unlock(&data->lock, key);

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
static int uart_ns16550_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	int i;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	for (i = 0; (i < size) && (ns16550_inbyte(dev_cfg, LSR(dev)) & LSR_RXRDY) != 0; i++) {
		rx_data[i] = ns16550_inbyte(dev_cfg, RDR(dev));
	}

	k_spin_unlock(&data->lock, key);

	return i;
}

/**
 * @brief Enable TX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_ns16550_irq_tx_enable(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_PM)
	struct uart_ns16550_dev_data *const dev_data = dev->data;

	if (!dev_data->tx_stream_on) {
		dev_data->tx_stream_on = true;
		uint8_t num_cpu_states;
		const struct pm_state_info *cpu_states;

		num_cpu_states = pm_state_cpu_get_all(0U, &cpu_states);

		/*
		 * Power state to be disabled. Some platforms have multiple
		 * states and need to be given a constraint set according to
		 * different states.
		 */
		for (uint8_t i = 0U; i < num_cpu_states; i++) {
			pm_policy_state_lock_get(cpu_states[i].state, PM_ALL_SUBSTATES);
		}
	}
#endif
	ns16550_outbyte(dev_cfg, IER(dev), ns16550_inbyte(dev_cfg, IER(dev)) | IER_TBE);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable TX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_ns16550_irq_tx_disable(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ns16550_outbyte(dev_cfg, IER(dev),
			ns16550_inbyte(dev_cfg, IER(dev)) & (~IER_TBE));

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) && defined(CONFIG_PM)
	struct uart_ns16550_dev_data *const dev_data = dev->data;

	if (dev_data->tx_stream_on) {
		dev_data->tx_stream_on = false;
		uint8_t num_cpu_states;
		const struct pm_state_info *cpu_states;

		num_cpu_states = pm_state_cpu_get_all(0U, &cpu_states);

		/*
		 * Power state to be enabled. Some platforms have multiple
		 * states and need to be given a constraint release according
		 * to different states.
		 */
		for (uint8_t i = 0U; i < num_cpu_states; i++) {
			pm_policy_state_lock_put(cpu_states[i].state, PM_ALL_SUBSTATES);
		}
	}
#endif
	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if Tx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_ns16550_irq_tx_ready(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = ((IIRC(dev) & IIR_ID) == IIR_THRE) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Check if nothing remains to be transmitted
 *
 * @param dev UART device struct
 *
 * @return 1 if nothing remains to be transmitted, 0 otherwise
 */
static int uart_ns16550_irq_tx_complete(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = ((ns16550_inbyte(dev_cfg, LSR(dev)) & (LSR_TEMT | LSR_THRE))
				== (LSR_TEMT | LSR_THRE)) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Enable RX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_ns16550_irq_rx_enable(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ns16550_outbyte(dev_cfg, IER(dev), ns16550_inbyte(dev_cfg, IER(dev)) | IER_RXRDY);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable RX interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_ns16550_irq_rx_disable(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ns16550_outbyte(dev_cfg, IER(dev),
			ns16550_inbyte(dev_cfg, IER(dev)) & (~IER_RXRDY));

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if Rx IRQ has been raised
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static int uart_ns16550_irq_rx_ready(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = ((IIRC(dev) & IIR_ID) == IIR_RBRF) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Enable error interrupt in IER
 *
 * @param dev UART device struct
 */
static void uart_ns16550_irq_err_enable(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ns16550_outbyte(dev_cfg, IER(dev),
			ns16550_inbyte(dev_cfg, IER(dev)) | IER_LSR);

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Disable error interrupt in IER
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is ready, 0 otherwise
 */
static void uart_ns16550_irq_err_disable(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ns16550_outbyte(dev_cfg, IER(dev),
			ns16550_inbyte(dev_cfg, IER(dev)) & (~IER_LSR));

	k_spin_unlock(&data->lock, key);
}

/**
 * @brief Check if any IRQ is pending
 *
 * @param dev UART device struct
 *
 * @return 1 if an IRQ is pending, 0 otherwise
 */
static int uart_ns16550_irq_is_pending(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	int ret = (!(IIRC(dev) & IIR_NIP)) ? 1 : 0;

	k_spin_unlock(&data->lock, key);

	return ret;
}

/**
 * @brief Update cached contents of IIR
 *
 * @param dev UART device struct
 *
 * @return Always 1
 */
static int uart_ns16550_irq_update(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	IIRC(dev) = ns16550_inbyte(dev_cfg, IIR(dev));

	k_spin_unlock(&data->lock, key);

	return 1;
}

/**
 * @brief Set the callback function pointer for IRQ.
 *
 * @param dev UART device struct
 * @param cb Callback function pointer.
 */
static void uart_ns16550_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb,
					  void *cb_data)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;

	k_spin_unlock(&dev_data->lock, key);
}

#ifdef CONFIG_UART_ASYNC_API

static bool is_async_write_complete(const struct device *dev)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;
	struct uart_ns16550_transfer_t *transfer = dev_data->tx_transfer;

	return dev_data->write_pos >= transfer->data_len;
}

static bool is_async_read_complete(const struct device *dev)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;
	struct uart_ns16550_transfer_t *transfer = dev_data->rx_transfer;

	return dev_data->read_pos >= transfer->data_len;
}

static void ns16550_write_transfer(const struct device *dev,
				   struct uart_ns16550_transfer_t *write_transfer)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	int count;

	if (!write_transfer) {
		return;
	}
	if (is_async_write_complete(dev)) {
		ns16550_outbyte(dev_cfg, IER(dev),
				ns16550_inbyte(dev_cfg, IER(dev)) & (~IER_TBE));
		/*
		 * At this point the FIFOs are empty, but the shift
		 * register still is transmitting the last 8 bits.
		 * So if we were to read LSR, it would say the device
		 * is still busy.
		 * Use the SCR Bit 0 to indicate an irq tx is complete.
		 */
		ns16550_outbyte(dev_cfg, SCR(dev), ns16550_inbyte(dev_cfg, SCR(dev))
				| UART_SCR_STATUS_UPDATE);
		if (write_transfer->callback) {
			write_transfer->callback(write_transfer->callback_data,
						 UART_PASS, UART_TRANSFER_SUCCESS,
						 dev_data->write_pos);
			write_transfer->callback = NULL;
		}
		return;
	}
	/*
	 * If we are starting the transfer then the TX FIFO is empty.
	 * In that case we set 'count' variable to UART_FIFO_DEPTH
	 * in order to take advantage of the whole FIFO capacity.
	 */
	count = (dev_data->write_pos == 0)
		 ? UART_FIFO_DEPTH
		 : UART_FIFO_HALF_DEPTH;
	while (count-- && !is_async_write_complete(dev)) {
		ns16550_outbyte(dev_cfg, THR(dev),
				write_transfer->data[dev_data->write_pos++]);
	}
	/*
	 * Change the threshold level to trigger an interrupt when the
	 * TX buffer is empty.
	 */
	if (is_async_write_complete(dev)) {
		ns16550_outbyte(dev_cfg, FCR(dev), ns16550_inbyte(dev_cfg, FCR(dev)) |
				(ns16550_inbyte(dev_cfg, IER(dev)) | IER_TBE));
	}

}

static void ns16550_read_transfer(const struct device *dev,
				  struct uart_ns16550_transfer_t *read_transfer)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	int lsr;

	if (!read_transfer) {
		return;
	}
	/*
	 * Copy data from RX FIFO to xfer buffer as long as the xfer
	 * has not completed and we have data in the RX FIFO.
	 */
	while (!is_async_read_complete(dev)) {
		lsr = ns16550_inbyte(dev_cfg, LSR(dev));
		/*
		 * A break condition may cause a line status
		 * interrupt to follow very closely after a
		 * char timeout interrupt, but reading the lsr
		 * effectively clears the pending interrupts so
		 * we issue here the callback
		 * instead, otherwise we would miss it.
		 * NOTE: Returned len is 0 for now, this might
		 * change in the future.
		 */
		if (lsr & LSR_EOB_MASK) {
			ns16550_outbyte(dev_cfg, IER(dev),
					ns16550_inbyte(dev_cfg, IER(dev)) &
					~(IER_RXRDY | IER_LSR));
			if (read_transfer->callback) {
				read_transfer->callback(read_transfer
							->callback_data,
							UART_DRIVER_ERROR,
							lsr & LSR_EOB_MASK, 0);
				read_transfer->callback = NULL;
			}
			return;
		}
		if (lsr & LSR_RXRDY) {
			read_transfer->data[dev_data->read_pos++] =
					    ns16550_inbyte(dev_cfg, THR(dev));
		} else {
			/* No more data in the RX FIFO. */
			break;
		}
	}
	if (is_async_read_complete(dev)) {
		/*
		 * Disable both 'Receiver Data Available' and
		 * 'Receiver Line Status' interrupts.
		 */
		ns16550_outbyte(dev_cfg, IER(dev), ns16550_inbyte(dev_cfg, IER(dev)) &
				~(IER_RXRDY | IER_LSR));
		if (read_transfer->callback) {
			read_transfer->callback(read_transfer->callback_data,
						UART_PASS, UART_TRANSFER_SUCCESS,
						dev_data->read_pos);
			read_transfer->callback = NULL;
		}
	}
}

static void ns16550_line_status(const struct device *dev, uint32_t line_status,
				struct uart_ns16550_transfer_t *read_transfer)
{
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;

	if (!line_status) {
		return;
	}
	if (read_transfer) {
		ns16550_outbyte(dev_cfg, IER(dev), ns16550_inbyte(dev_cfg, IER(dev)) &
				~(IER_RXRDY | IER_LSR));
		if (read_transfer->callback) {
			/*
			 * Return the number of bytes read
			 * a zero as a line status error
			 * was detected.
			 */
			read_transfer->callback(read_transfer->callback_data,
						UART_DRIVER_ERROR,
						line_status, 0);
			read_transfer->callback = NULL;
		}
	}
}


static void uart_ns16550_callback(const struct device *dev)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	struct uart_ns16550_transfer_t *write_transfer = dev_data->tx_transfer;
	struct uart_ns16550_transfer_t *read_transfer = dev_data->rx_transfer;
	uint8_t interrupt_id = ns16550_inbyte(dev_cfg, IIR(dev)) & IIR_MASK;
	uint32_t line_status;

	/*
	 * Interrupt ID priority levels (from highest to lowest):
	 * 1: IIR_LS
	 * 2: IIR_RBRF and IIR_CH
	 * 3: IIR_THRE
	 */
	switch (interrupt_id) {
	/* Spurious interrupt */
	case IIR_NIP:
		break;
	case IIR_LS:
		line_status = ns16550_inbyte(dev_cfg, LSR(dev)) & LSR_EOB_MASK;
		ns16550_line_status(dev, line_status, read_transfer);
		break;
	case IIR_CH:
	case IIR_RBRF:
		ns16550_read_transfer(dev, read_transfer);
		break;
	case IIR_THRE:
		ns16550_write_transfer(dev, write_transfer);
		break;
	default:
		/* Unhandled interrupt occurred,disable uart interrupts.
		 * and report error.
		 */
		if (read_transfer && read_transfer->callback) {
			ns16550_outbyte(dev_cfg, IER(dev), ns16550_inbyte(dev_cfg, IER(dev)) &
				~(IER_RXRDY | IER_LSR));
			read_transfer->callback(read_transfer->callback_data,
						UART_DRIVER_ERROR,
						UART_TRANSFER_FAILED, 0);
			read_transfer->callback = NULL;
		}

		if (write_transfer && write_transfer->callback) {
			ns16550_outbyte(dev_cfg, IER(dev),
					ns16550_inbyte(dev_cfg, IER(dev)) & (~IER_TBE));
			write_transfer->callback(write_transfer->callback_data,
						 UART_DRIVER_ERROR,
						 UART_TRANSFER_FAILED, 0);
			write_transfer->callback = NULL;
		}
	}
}
#endif /* CONFIG_UART_ASYNC_API */

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 */
static void uart_ns16550_isr(const struct device *dev)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;

	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
#ifdef CONFIG_UART_ASYNC_API
	} else {
		uart_ns16550_callback(dev);
#endif
	}

#ifdef CONFIG_UART_NS16550_WA_ISR_REENABLE_INTERRUPT
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	uint8_t cached_ier = ns16550_inbyte(dev_cfg, IER(dev));

	ns16550_outbyte(dev_cfg, IER(dev), 0U);
	ns16550_outbyte(dev_cfg, IER(dev), cached_ier);
#endif
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
static int uart_ns16550_line_ctrl_set(const struct device *dev,
				      uint32_t ctrl, uint32_t val)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_device_config *const dev_cfg = dev->config;
	uint32_t mdc, chg, pclk = 0U;
	k_spinlock_key_t key;

	if (dev_cfg->sys_clk_freq != 0U) {
		pclk = dev_cfg->sys_clk_freq;
	} else {
		if (device_is_ready(dev_cfg->clock_dev)) {
			clock_control_get_rate(dev_cfg->clock_dev, dev_cfg->clock_subsys, &pclk);
		}
	}

	switch (ctrl) {
	case UART_LINE_CTRL_BAUD_RATE:
		set_baud_rate(dev, val, pclk);
		return 0;

	case UART_LINE_CTRL_RTS:
	case UART_LINE_CTRL_DTR:
		key = k_spin_lock(&data->lock);
		mdc = ns16550_inbyte(dev_cfg, MDC(dev));

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
		ns16550_outbyte(dev_cfg, MDC(dev), mdc);
		k_spin_unlock(&data->lock, key);
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
static int uart_ns16550_drv_cmd(const struct device *dev, uint32_t cmd,
				uint32_t p)
{
#if UART_NS16550_DLF_ENABLED
	if (cmd == CMD_SET_DLF) {
		struct uart_ns16550_dev_data * const dev_data = dev->data;
		const struct uart_ns16550_device_config * const dev_cfg = dev->config;
		k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

		dev_data->dlf = p;
		ns16550_outbyte(dev_cfg, DLF(dev), dev_data->dlf);
		k_spin_unlock(&dev_data->lock, key);
		return 0;
	}
#endif

	return -ENOTSUP;
}

#endif /* CONFIG_UART_NS16550_DRV_CMD */

#ifdef CONFIG_UART_ASYNC_API
static uint32_t uart_ns16550_decode_line_err(uint32_t line_err)
{
	uint32_t zephyr_line_err = 0;

	if (line_err  & LSR_OE) {
		zephyr_line_err = UART_ERROR_OVERRUN;
	}

	if (line_err & LSR_PE) {
		zephyr_line_err = UART_ERROR_PARITY;
	}

	if (line_err  & LSR_FE) {
		zephyr_line_err = UART_ERROR_FRAMING;
	}

	if (line_err & LSR_BI) {
		zephyr_line_err = UART_BREAK;
	}

	return zephyr_line_err;
}


static void uart_ns16550_irq_tx_common_cb(void *data, int err, uint32_t status,
				       uint32_t len)
{
	const struct device *dev = data;
	struct uart_ns16550_dev_data *dev_data = dev->data;

	if (dev_data->async_cb) {
		dev_data->async_user_data = (void *)(uintptr_t)
					     uart_ns16550_decode_line_err(status);
		dev_data->async_cb(dev, &dev_data->evt, dev_data->async_user_data);
	}
	k_sem_give(GET_TX_SEM(dev));
}

static void uart_ns16550_irq_rx_common_cb(void *data, int err, uint32_t status,
				       uint32_t len)
{
	const struct device *dev = data;
	struct uart_ns16550_dev_data *dev_data = dev->data;

	if (dev_data->async_cb) {
		dev_data->async_user_data = (void *)(uintptr_t)
					     uart_ns16550_decode_line_err(status);
		dev_data->async_cb(dev, &dev_data->evt, dev_data->async_user_data);
	}
	k_sem_give(GET_RX_SEM(dev));

}

static int uart_ns16550_async_callback_set(const struct device *dev,
					 uart_callback_t cb,
					 void *user_data)
{
	struct uart_ns16550_dev_data *dev_data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->async_cb = cb;
	dev_data->async_user_data = user_data;

	k_spin_unlock(&dev_data->lock, key);
	return 0;
}

static int uart_ns16550_write_buffer_async(const struct device *dev,
					   const uint8_t *tx_buf,
					   size_t tx_buf_size, int32_t timeout)
{
	struct uart_ns16550_dev_data *dev_data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	int ret;

	__ASSERT(tx_buf != NULL, "");
	__ASSERT(tx_buf_size != 0, "");


	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (k_sem_take(GET_TX_SEM(dev), K_NO_WAIT)) {
		ret = -EBUSY;
		goto out;
	}
	if (dev_data->async_cb) {
		dev_data->evt.type = UART_TX_DONE;
	}

	dev_data->tx_transfer->data = (uint8_t *)tx_buf;
	dev_data->tx_transfer->data_len = tx_buf_size;
	dev_data->tx_transfer->callback = uart_ns16550_irq_tx_common_cb;
	dev_data->tx_transfer->callback_data = (void *)dev;

	dev_data->write_pos = 0;

	/* Set threshold. */
	ns16550_outbyte(dev_cfg, FCR(dev), (FCR_FIFO | FCR_FIFO_8));

	/* Enable TX holding reg empty interrupt. */
	ns16550_outbyte(dev_cfg, IER(dev), (ns16550_inbyte(dev_cfg, IER(dev)) | IER_TBE));
	ret = 0;
out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
}

static int uart_ns16550_write_abort_async(const struct device *dev)
{
	struct uart_ns16550_dev_data *dev_data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	struct uart_ns16550_transfer_t *transfer = dev_data->tx_transfer;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (dev_data->async_cb) {
		dev_data->evt.type = UART_TX_ABORTED;
	}
	/* No ongoing write transaction to be terminated. */
	if (transfer == NULL) {
		ret = -EIO;
		goto out;
	}
	/* Disable TX holding reg empty interrupt. */
	ns16550_outbyte(dev_cfg, IER(dev), ns16550_inbyte(dev_cfg, IER(dev)) & (~IER_TBE));

	if (transfer) {
		if (transfer->callback) {
			transfer->callback(transfer->callback_data,
					   UART_ERROR_CANCELED,
					   UART_TRANSFER_FAILED,
					   dev_data->write_pos);
		}
		dev_data->tx_transfer->callback = NULL;
		dev_data->write_pos = 0;
	}
	ret = 0;
out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
}

static int uart_ns16550_read_buffer_async(const struct device *dev,
					  uint8_t *rx_buf,
					  size_t rx_buf_size,
					  int32_t timeout)
{
	__ASSERT(rx_buf != NULL, "");
	__ASSERT(rx_buf_size != 0, "");
	struct uart_ns16550_dev_data *dev_data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (k_sem_take(GET_RX_SEM(dev), K_NO_WAIT)) {
		ret = -EBUSY;
		goto out;
	}

	if (dev_data->async_cb) {
		dev_data->evt.type = UART_RX_RDY;
	}

	dev_data->rx_transfer->data = rx_buf;
	dev_data->rx_transfer->data_len = rx_buf_size;
	dev_data->rx_transfer->callback = uart_ns16550_irq_rx_common_cb;
	dev_data->rx_transfer->callback_data = (void *)dev;

	dev_data->read_pos = 0;

	/* Set threshold. */
	ns16550_outbyte(dev_cfg, FCR(dev), (FCR_FIFO | FCR_FIFO_8));

	/*
	 * Enable both 'Receiver Data Available' and 'Receiver
	 * Line Status' interrupts.
	 */
	ns16550_outbyte(dev_cfg, IER(dev),
			(ns16550_inbyte(dev_cfg, IER(dev)) | IER_RXRDY | IER_LSR));
	ret = 0;
out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
}

static int uart_ns16550_read_disable_async(const struct device *dev)
{
	struct uart_ns16550_dev_data *dev_data = dev->data;
	const struct uart_ns16550_device_config * const dev_cfg = dev->config;
	struct uart_ns16550_transfer_t *transfer = dev_data->rx_transfer;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (dev_data->async_cb) {
		dev_data->evt.type = UART_RX_DISABLED;
	}

	/* No ongoing read transaction to be terminated. */
	if (transfer == NULL) {
		ret = -EIO;
		goto out;
	}

	/*
	 * Disable both 'Receiver Data Available' and 'Receiver Line Status'
	 * interrupts.
	 */
	ns16550_outbyte(dev_cfg, IER(dev),
			ns16550_inbyte(dev_cfg, IER(dev)) & ~(IER_RXRDY | IER_LSR));

	if (transfer) {
		if (transfer->callback) {
			transfer->callback(transfer->callback_data,
					   UART_ERROR_CANCELED,
					   UART_TRANSFER_FAILED,
					   dev_data->read_pos);
			transfer->callback = NULL;
		}
		dev_data->read_pos = 0;
	}
	ret = 0;

out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
}

static int uart_ns16550_read_buf_rsp(const struct device *dev, uint8_t *buf,
				   size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	return -ENOTSUP;
}
#endif /* CONFIG_UART_ASYNC_API */

static const struct uart_driver_api uart_ns16550_driver_api = {
	.poll_in = uart_ns16550_poll_in,
	.poll_out = uart_ns16550_poll_out,
	.err_check = uart_ns16550_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_ns16550_configure,
	.config_get = uart_ns16550_config_get,
#endif
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

#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_ns16550_async_callback_set,
	.tx = uart_ns16550_write_buffer_async,
	.tx_abort = uart_ns16550_write_abort_async,
	.rx_enable = uart_ns16550_read_buffer_async,
	.rx_disable = uart_ns16550_read_disable_async,
	.rx_buf_rsp = uart_ns16550_read_buf_rsp,
#endif

#ifdef CONFIG_UART_NS16550_LINE_CTRL
	.line_ctrl_set = uart_ns16550_line_ctrl_set,
#endif

#ifdef CONFIG_UART_NS16550_DRV_CMD
	.drv_cmd = uart_ns16550_drv_cmd,
#endif
};

#define UART_NS16550_IRQ_FLAGS_SENSE0(n) 0
#define UART_NS16550_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define UART_NS16550_IRQ_FLAGS(n) \
	_CONCAT(UART_NS16550_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))(n)

/* not PCI(e) */
#define UART_NS16550_IRQ_CONFIG_PCIE0(n)                                      \
	static void irq_config_func##n(const struct device *dev)              \
	{                                                                     \
		ARG_UNUSED(dev);                                              \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	      \
			    uart_ns16550_isr, DEVICE_DT_INST_GET(n),	      \
			    UART_NS16550_IRQ_FLAGS(n));			      \
		irq_enable(DT_INST_IRQN(n));                                  \
	}

/* PCI(e) with auto IRQ detection */
#define UART_NS16550_IRQ_CONFIG_PCIE1(n)                                      \
	static void irq_config_func##n(const struct device *dev)              \
	{                                                                     \
		BUILD_ASSERT(DT_INST_IRQN(n) == PCIE_IRQ_DETECT,              \
			     "Only runtime IRQ configuration is supported");  \
		BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),           \
			     "NS16550 PCIe requires dynamic interrupts");     \
		const struct uart_ns16550_device_config *dev_cfg = dev->config;\
		unsigned int irq = pcie_alloc_irq(dev_cfg->pcie->bdf);        \
		if (irq == PCIE_CONF_INTR_IRQ_NONE) {                         \
			return;                                               \
		}                                                             \
		pcie_connect_dynamic_irq(dev_cfg->pcie->bdf, irq,	      \
				     DT_INST_IRQ(n, priority),		      \
				    (void (*)(const void *))uart_ns16550_isr, \
				    DEVICE_DT_INST_GET(n),                    \
				    UART_NS16550_IRQ_FLAGS(n));               \
		pcie_irq_enable(dev_cfg->pcie->bdf, irq);                    \
	}

#ifdef CONFIG_UART_NS16550_ACCESS_IOPORT
#define REG_INIT(n) \
	.port = DT_INST_REG_ADDR(n), \
	.io_map = true,
#else
#define REG_INIT_PCIE1(n)
#define REG_INIT_PCIE0(n) DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),

#define DEV_REG_PORT_IO_1(n) \
	.port = DT_INST_REG_ADDR(n),
#define DEV_REG_PORT_IO_0(n) \
	_CONCAT(REG_INIT_PCIE, DT_INST_ON_BUS(n, pcie))(n)
#ifdef CONFIG_UART_NS16550_SIMULT_ACCESS
#define DEV_IO_INIT(n) \
	.io_map = DT_INST_PROP(n, io_mapped),
#else
#define DEV_IO_INIT(n)
#endif

#define REG_INIT(n) \
	_CONCAT(DEV_REG_PORT_IO_, DT_INST_PROP(n, io_mapped))(n) \
	DEV_IO_INIT(n)
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define DEV_CONFIG_IRQ_FUNC_INIT(n) \
	.irq_config_func = irq_config_func##n,
#define UART_NS16550_IRQ_FUNC_DECLARE(n) \
	static void irq_config_func##n(const struct device *dev);
#define UART_NS16550_IRQ_FUNC_DEFINE(n) \
	_CONCAT(UART_NS16550_IRQ_CONFIG_PCIE, DT_INST_ON_BUS(n, pcie))(n)
#else
/* !CONFIG_UART_INTERRUPT_DRIVEN */
#define DEV_CONFIG_IRQ_FUNC_INIT(n)
#define UART_NS16550_IRQ_FUNC_DECLARE(n)
#define UART_NS16550_IRQ_FUNC_DEFINE(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if UART_NS16550_PCP_ENABLED
#define DEV_CONFIG_PCP_INIT(n) .pcp = DT_INST_PROP_OR(n, pcp, 0),
#else
#define DEV_CONFIG_PCP_INIT(n)
#endif

#define DEV_CONFIG_PCIE0(n)
#define DEV_CONFIG_PCIE1(n) DEVICE_PCIE_INST_INIT(n, pcie)
#define DEV_CONFIG_PCIE_INIT(n) \
	_CONCAT(DEV_CONFIG_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define DEV_DECLARE_PCIE0(n)
#define DEV_DECLARE_PCIE1(n) DEVICE_PCIE_INST_DECLARE(n)
#define DEV_PCIE_DECLARE(n) \
	_CONCAT(DEV_DECLARE_PCIE, DT_INST_ON_BUS(n, pcie))(n)

#define DEV_DATA_FLOW_CTRL0 UART_CFG_FLOW_CTRL_NONE
#define DEV_DATA_FLOW_CTRL1 UART_CFG_FLOW_CTRL_RTS_CTS
#define DEV_DATA_FLOW_CTRL(n) \
	_CONCAT(DEV_DATA_FLOW_CTRL, DT_INST_PROP_OR(n, hw_flow_control, 0))

#define DEV_DATA_DLF0(n)
#define DEV_DATA_DLF1(n) \
	.dlf = DT_INST_PROP(n, dlf),
#define DEV_DATA_DLF_INIT(n) \
	_CONCAT(DEV_DATA_DLF, DT_INST_NODE_HAS_PROP(n, dlf))(n)

/* UART on PCIe should be initialized POST_KERNEL as PCIe loads
 * as PRE_KERNEL_1 and these UART instances should not load before PCIe.
 * In some platforms legacy UART instance is used for console and
 * shell so it should load as PRE_KERNEL_1.
 */
#define DEV_BOOT_PRIO0(n) PRE_KERNEL_1
#define DEV_BOOT_PRIO1(n) POST_KERNEL

#define NS16550_BOOT_PRIO(n) \
	_CONCAT(DEV_BOOT_PRIO, DT_INST_ON_BUS(n, pcie))(n)
#ifdef CONFIG_UART_ASYNC_API
#define DEV_ASYNC_DECLARE(n)                                                         \
	static K_SEM_DEFINE(uart_##n##_tx_sem, 1, 1);                                \
	static K_SEM_DEFINE(uart_##n##_rx_sem, 1, 1);                                \
	static struct uart_ns16550_transfer_t tx_##n;                                \
	static struct uart_ns16550_transfer_t rx_##n;
#else
#define DEV_ASYNC_DECLARE(n)
#endif

#ifdef CONFIG_UART_ASYNC_API
#define DEV_CONFIG_ASYNC_INIT(n)                                                     \
	.tx_sem = &uart_##n##_tx_sem,                                                \
	.rx_sem = &uart_##n##_rx_sem,
#else
#define DEV_CONFIG_ASYNC_INIT(n)
#endif

#ifdef CONFIG_UART_ASYNC_API
#define DEV_DATA_ASYNC_INIT(n)                                                       \
	.tx_transfer = &tx_##n,                                                      \
	.rx_transfer = &rx_##n,
#else
#define DEV_DATA_ASYNC_INIT(n)
#endif

#define UART_NS16550_DEVICE_INIT(n)                                                  \
	UART_NS16550_IRQ_FUNC_DECLARE(n);                                            \
	DEV_PCIE_DECLARE(n);                                                         \
	DEV_ASYNC_DECLARE(n);							     \
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_INST_DEFINE(n)));                     \
	static const struct uart_ns16550_device_config uart_ns16550_dev_cfg_##n = {  \
		REG_INIT(n)							     \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clock_frequency), (             \
				.sys_clk_freq = DT_INST_PROP(n, clock_frequency),    \
				.clock_dev = NULL,                                   \
				.clock_subsys = NULL,                                \
			), (                                                         \
				.sys_clk_freq = 0,                                   \
				.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),  \
				.clock_subsys = (clock_control_subsys_t) DT_INST_PHA(\
								0, clocks, clkid),   \
			)                                                            \
		)                                                                    \
		DEV_CONFIG_IRQ_FUNC_INIT(n)                                          \
		DEV_CONFIG_PCP_INIT(n)                                               \
		.reg_interval = (1 << DT_INST_PROP(n, reg_shift)),                   \
		DEV_CONFIG_ASYNC_INIT(n)					     \
		DEV_CONFIG_PCIE_INIT(n)                                              \
		IF_ENABLED(CONFIG_PINCTRL,                                           \
			(.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),))              \
	};                                                                           \
	static struct uart_ns16550_dev_data uart_ns16550_dev_data_##n = {            \
		.uart_config.baudrate = DT_INST_PROP_OR(n, current_speed, 0),        \
		.uart_config.parity = UART_CFG_PARITY_NONE,                          \
		.uart_config.stop_bits = UART_CFG_STOP_BITS_1,                       \
		.uart_config.data_bits = UART_CFG_DATA_BITS_8,                       \
		.uart_config.flow_ctrl = DEV_DATA_FLOW_CTRL(n),                      \
		DEV_DATA_DLF_INIT(n)                                                 \
		DEV_DATA_ASYNC_INIT(n)						     \
	};                                                                           \
	DEVICE_DT_INST_DEFINE(n, &uart_ns16550_init, NULL,                           \
			      &uart_ns16550_dev_data_##n, &uart_ns16550_dev_cfg_##n, \
			      NS16550_BOOT_PRIO(n), CONFIG_SERIAL_INIT_PRIORITY,             \
			      &uart_ns16550_driver_api);                             \
	UART_NS16550_IRQ_FUNC_DEFINE(n)

DT_INST_FOREACH_STATUS_OKAY(UART_NS16550_DEVICE_INIT)
