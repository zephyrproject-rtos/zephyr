/* ns16550.c - NS16550D serial driver */

#define DT_DRV_COMPAT ns16550

/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
 * Copyright (c) 2020-2023 Intel Corp.
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
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_ns16550, CONFIG_UART_LOG_LEVEL);

#define UART_NS16550_PCP_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(pcp)
#define UART_NS16550_DLF_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(dlf)
#define UART_NS16550_DMAS_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(dmas)

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
BUILD_ASSERT(IS_ENABLED(CONFIG_PCIE), "NS16550(s) in DT need CONFIG_PCIE");
#include <zephyr/drivers/pcie/pcie.h>
#endif

/* Is UART module 'resets' line property defined */
#define UART_NS16550_RESET_ENABLED DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)

#if UART_NS16550_RESET_ENABLED
#include <zephyr/drivers/reset.h>
#endif

#if defined(CONFIG_UART_ASYNC_API)
#include <zephyr/drivers/dma.h>
#include <assert.h>

#if defined(CONFIG_UART_NS16550_INTEL_LPSS_DMA)
#include <zephyr/drivers/dma/dma_intel_lpss.h>
#endif

#endif

/* If any node has property io-mapped set, we need to support IO port
 * access in the code and device config struct.
 *
 * Note that DT_ANY_INST_HAS_PROP_STATUS_OKAY() always returns true
 * as io-mapped property is considered always exists and present,
 * even if its value is zero. Therefore we cannot use it, and has to
 * resort to the follow helper to see if any okay nodes have io-mapped
 * as 1.
 */
#define UART_NS16550_DT_PROP_IOMAPPED_HELPER(inst, prop, def) \
	DT_INST_PROP_OR(inst, prop, def) ||

#define UART_NS16550_IOPORT_ENABLED \
	(DT_INST_FOREACH_STATUS_OKAY_VARGS(UART_NS16550_DT_PROP_IOMAPPED_HELPER, io_mapped, 0) 0)

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
#define REG_USR 0x7C  /* UART status reg. (DW8250)      */
#define REG_DLF 0xC0  /* Divisor Latch Fraction         */
#define REG_PCP 0x200 /* PRV_CLOCK_PARAMS (Apollo Lake) */
#define REG_MDR1 0x08 /* Mode control reg. (TI_K3) */

#if defined(CONFIG_UART_NS16550_INTEL_LPSS_DMA)
#define REG_LPSS_SRC_TRAN 0xAF8 /* SRC Transfer LPSS DMA */
#define REG_LPSS_CLR_SRC_TRAN 0xB48 /* Clear SRC Tran LPSS DMA */
#define REG_LPSS_MST 0xB20 /* Mask SRC Transfer LPSS DMA */
#endif

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
#define IIR_BUSY  0x07 /* DesignWare APB busy detect */
#define IIR_FE    0xC0 /* FIFO mode enabled */
#define IIR_CH    0x0C /* Character timeout*/

/* equates for FIFO control register */

#define FCR_FIFO 0x01    /* enable XMIT and RCVR FIFO */
#define FCR_RCVRCLR 0x02 /* clear RCVR FIFO */
#define FCR_XMITCLR 0x04 /* clear XMIT FIFO */

/* equates for Apollo Lake clock control register (PRV_CLOCK_PARAMS) */

#define PCP_UPDATE 0x80000000 /* update clock */
#define PCP_EN 0x00000001     /* enable clock output */

/* Fields for TI K3 UART module */

#define MDR1_MODE_SELECT_FIELD_MASK		BIT_MASK(3)
#define MDR1_MODE_SELECT_FIELD_SHIFT		BIT_MASK(0)

/* Modes available for TI K3 UART module */

#define MDR1_STD_MODE					(0)
#define MDR1_SIR_MODE					(1)
#define MDR1_UART_16X					(2)
#define MDR1_UART_13X					(3)
#define MDR1_MIR_MODE					(4)
#define MDR1_FIR_MODE					(5)
#define MDR1_CIR_MODE					(6)
#define MDR1_DISABLE					(7)

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

#define THR(dev) (get_port(dev) + (REG_THR * reg_interval(dev)))
#define RDR(dev) (get_port(dev) + (REG_RDR * reg_interval(dev)))
#define BRDL(dev) (get_port(dev) + (REG_BRDL * reg_interval(dev)))
#define BRDH(dev) (get_port(dev) + (REG_BRDH * reg_interval(dev)))
#define IER(dev) (get_port(dev) + (REG_IER * reg_interval(dev)))
#define IIR(dev) (get_port(dev) + (REG_IIR * reg_interval(dev)))
#define FCR(dev) (get_port(dev) + (REG_FCR * reg_interval(dev)))
#define LCR(dev) (get_port(dev) + (REG_LCR * reg_interval(dev)))
#define MDC(dev) (get_port(dev) + (REG_MDC * reg_interval(dev)))
#define LSR(dev) (get_port(dev) + (REG_LSR * reg_interval(dev)))
#define MSR(dev) (get_port(dev) + (REG_MSR * reg_interval(dev)))
#define MDR1(dev) (get_port(dev) + (REG_MDR1 * reg_interval(dev)))
#define USR(dev) (get_port(dev) + REG_USR)
#define DLF(dev) (get_port(dev) + REG_DLF)
#define PCP(dev) (get_port(dev) + REG_PCP)

#if defined(CONFIG_UART_NS16550_INTEL_LPSS_DMA)
#define SRC_TRAN(dev) (get_port(dev) + REG_LPSS_SRC_TRAN)
#define CLR_SRC_TRAN(dev) (get_port(dev) + REG_LPSS_CLR_SRC_TRAN)
#define MST(dev) (get_port(dev) + REG_LPSS_MST)

#define UNMASK_LPSS_INT(chan) (BIT(chan) | (BIT(8) << chan)) /* unmask LPSS DMA Interrupt */
#endif

#define IIRC(dev) (((struct uart_ns16550_dev_data *)(dev)->data)->iir_cache)

#ifdef CONFIG_UART_NS16550_ITE_HIGH_SPEED_BAUDRATE
/* Register definitions (ITE_IT8XXX2) */
#define REG_ECSPMR 0x08 /* EC Serial port mode reg */

/* Fields for ITE IT8XXX2 UART module */
#define ECSPMR_ECHS 0x02  /* EC high speed select */

/* IT8XXX2 UART high speed baud rate settings */
#define UART_BAUDRATE_115200 115200
#define UART_BAUDRATE_230400 230400
#define UART_BAUDRATE_460800 460800
#define IT8XXX2_230400_DIVISOR 32770
#define IT8XXX2_460800_DIVISOR 32769

#define ECSPMR(dev) (get_port(dev) + REG_ECSPMR * reg_interval(dev))
#endif

#if defined(CONFIG_UART_ASYNC_API)
struct uart_ns16550_rx_dma_params {
	const struct device *dma_dev;
	uint8_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config active_dma_block;
	uint8_t *buf;
	size_t buf_len;
	size_t offset;
	size_t counter;
	struct k_work_delayable timeout_work;
	size_t timeout_us;
};

struct uart_ns16550_tx_dma_params {
	const struct device *dma_dev;
	uint8_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config active_dma_block;
	const uint8_t *buf;
	size_t buf_len;
	struct k_work_delayable timeout_work;
	size_t timeout_us;
};

struct uart_ns16550_async_data {
	const struct device *uart_dev;
	struct uart_ns16550_tx_dma_params tx_dma_params;
	struct uart_ns16550_rx_dma_params rx_dma_params;
	uint8_t *next_rx_buffer;
	size_t next_rx_buffer_len;
	uart_callback_t user_callback;
	void *user_data;
};

static void uart_ns16550_async_rx_timeout(struct k_work *work);
static void uart_ns16550_async_tx_timeout(struct k_work *work);
#endif

/** Device config structure */
struct uart_ns16550_dev_config {
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
#if UART_NS16550_IOPORT_ENABLED
	bool io_map;
#endif
#if UART_NS16550_RESET_ENABLED
	struct reset_dt_spec reset_spec;
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

#if defined(CONFIG_UART_ASYNC_API)
	uint64_t phys_addr;
	struct uart_ns16550_async_data async;
#endif
};

static void ns16550_outbyte(const struct uart_ns16550_dev_config *cfg,
			    uintptr_t port, uint8_t val)
{
#if UART_NS16550_IOPORT_ENABLED
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

static uint8_t ns16550_inbyte(const struct uart_ns16550_dev_config *cfg,
			      uintptr_t port)
{
#if UART_NS16550_IOPORT_ENABLED
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

__maybe_unused
static void ns16550_outword(const struct uart_ns16550_dev_config *cfg,
			    uintptr_t port, uint32_t val)
{
#if UART_NS16550_IOPORT_ENABLED
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

__maybe_unused
static uint32_t ns16550_inword(const struct uart_ns16550_dev_config *cfg,
			      uintptr_t port)
{
#if UART_NS16550_IOPORT_ENABLED
	if (cfg->io_map) {
		return sys_in32(port);
	}
#endif
	/* MMIO mapped */
	return sys_read32(port);
}

static inline uint8_t reg_interval(const struct device *dev)
{
	const struct uart_ns16550_dev_config *config = dev->config;

	return config->reg_interval;
}

static inline uintptr_t get_port(const struct device *dev)
{
	uintptr_t port;
#if UART_NS16550_IOPORT_ENABLED
	const struct uart_ns16550_dev_config *config = dev->config;

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

static uint32_t get_uart_baudrate_divisor(const struct device *dev,
					  uint32_t baud_rate,
					  uint32_t pclk)
{
	ARG_UNUSED(dev);
	/*
	 * calculate baud rate divisor. a variant of
	 * (uint32_t)(pclk / (16.0 * baud_rate) + 0.5)
	 */
	return ((pclk + (baud_rate << 3)) / baud_rate) >> 4;
}

#ifdef CONFIG_UART_NS16550_ITE_HIGH_SPEED_BAUDRATE
static uint32_t get_ite_uart_baudrate_divisor(const struct device *dev,
					      uint32_t baud_rate,
					      uint32_t pclk)
{
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
	uint32_t divisor = 0;

	if (baud_rate > UART_BAUDRATE_115200) {
		/* Baud rate divisor for high speed */
		if (baud_rate == UART_BAUDRATE_230400) {
			divisor = IT8XXX2_230400_DIVISOR;
		} else if (baud_rate == UART_BAUDRATE_460800) {
			divisor = IT8XXX2_460800_DIVISOR;
		}
		/*
		 * This bit indicates that the supported baud rate of
		 * UART1/UART2 can be up to 230.4k and 460.8k.
		 * Other bits are reserved and have no setting, so we
		 * directly write the ECSPMR register.
		 */
		ns16550_outbyte(dev_cfg, ECSPMR(dev), ECSPMR_ECHS);
	} else {
		divisor = get_uart_baudrate_divisor(dev, baud_rate, pclk);
		/* Set ECSPMR register as default */
		ns16550_outbyte(dev_cfg, ECSPMR(dev), 0);
	}

	return divisor;
}
#endif

static inline int ns16550_read_char(const struct device *dev, unsigned char *c)
{
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;

	if ((ns16550_inbyte(dev_cfg, LSR(dev)) & LSR_RXRDY) != 0) {
		*c = ns16550_inbyte(dev_cfg, RDR(dev));
		return 0;
	}

	return -1;
}

static void set_baud_rate(const struct device *dev, uint32_t baud_rate, uint32_t pclk)
{
	struct uart_ns16550_dev_data * const dev_data = dev->data;
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
	uint32_t divisor; /* baud rate divisor */
	uint8_t lcr_cache;

	if ((baud_rate != 0U) && (pclk != 0U)) {
#ifdef CONFIG_UART_NS16550_ITE_HIGH_SPEED_BAUDRATE
		divisor = get_ite_uart_baudrate_divisor(dev, baud_rate, pclk);
#else
		divisor = get_uart_baudrate_divisor(dev, baud_rate, pclk);
#endif
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
	uint8_t mdc = 0U, c;
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

#ifdef CONFIG_UART_NS16550_TI_K3
	uint32_t mdr = ns16550_inbyte(dev_cfg, MDR1(dev));

	mdr = ((mdr & ~MDR1_MODE_SELECT_FIELD_MASK) | ((((MDR1_STD_MODE) <<
		MDR1_MODE_SELECT_FIELD_SHIFT)) & MDR1_MODE_SELECT_FIELD_MASK));
	ns16550_outbyte(dev_cfg, MDR1(dev), mdr);
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

		if (clock_control_get_rate(dev_cfg->clock_dev,
					   dev_cfg->clock_subsys,
					   &pclk) != 0) {
			ret = -EINVAL;
			goto out;
		}
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
	(void)ns16550_read_char(dev, &c);

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

#if UART_NS16550_RESET_ENABLED
/**
 * @brief Toggle the reset UART line
 *
 * This routine is called to bring UART IP out of reset state.
 *
 * @param reset_spec Reset controller device configuration struct
 *
 * @return 0 if successful, failed otherwise
 */
static int uart_reset_config(const struct reset_dt_spec *reset_spec)
{
	int ret;

	if (!device_is_ready(reset_spec->dev)) {
		LOG_ERR("Reset controller device is not ready");
		return -ENODEV;
	}

	ret = reset_line_toggle(reset_spec->dev, reset_spec->id);
	if (ret != 0) {
		LOG_ERR("UART toggle reset line failed");
		return ret;
	}

	return 0;
}
#endif /* UART_NS16550_RESET_ENABLED */

#if (IS_ENABLED(CONFIG_UART_ASYNC_API))
static inline void async_timer_start(struct k_work_delayable *work, size_t timeout_us)
{
	if ((timeout_us != SYS_FOREVER_US) && (timeout_us != 0)) {
		k_work_reschedule(work, K_USEC(timeout_us));
	}
}

#endif

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
	const struct uart_ns16550_dev_config *dev_cfg = dev->config;
	int ret;

	ARG_UNUSED(dev_cfg);

#if UART_NS16550_RESET_ENABLED
	/* Assert the UART reset line if it is defined. */
	if (dev_cfg->reset_spec.dev != NULL) {
		ret = uart_reset_config(&(dev_cfg->reset_spec));
		if (ret != 0) {
			return ret;
		}
	}
#endif

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
#if defined(CONFIG_UART_ASYNC_API)
		if (data->async.tx_dma_params.dma_dev != NULL) {
			pcie_set_cmd(dev_cfg->pcie->bdf, PCIE_CONF_CMDSTAT_MASTER, true);
			data->phys_addr = mbar.phys_addr;
		}
#endif
	} else
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie) */
	{
#if UART_NS16550_IOPORT_ENABLED
		/* Map directly from DTS */
		if (!dev_cfg->io_map) {
#else
		{
#endif
			DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
		}
	}
#if defined(CONFIG_UART_ASYNC_API)
	if (data->async.tx_dma_params.dma_dev != NULL) {
		data->async.next_rx_buffer = NULL;
		data->async.next_rx_buffer_len = 0;
		data->async.uart_dev = dev;
		k_work_init_delayable(&data->async.rx_dma_params.timeout_work,
				      uart_ns16550_async_rx_timeout);
		k_work_init_delayable(&data->async.tx_dma_params.timeout_work,
				      uart_ns16550_async_tx_timeout);
		data->async.rx_dma_params.dma_cfg.head_block =
			&data->async.rx_dma_params.active_dma_block;
		data->async.tx_dma_params.dma_cfg.head_block =
			&data->async.tx_dma_params.active_dma_block;
#if defined(CONFIG_UART_NS16550_INTEL_LPSS_DMA)
#if UART_NS16550_IOPORT_ENABLED
		if (!dev_cfg->io_map)
#endif
		{
			uintptr_t base;

			base = DEVICE_MMIO_GET(dev) + DMA_INTEL_LPSS_OFFSET;
			dma_intel_lpss_set_base(data->async.tx_dma_params.dma_dev, base);
			dma_intel_lpss_setup(data->async.tx_dma_params.dma_dev);
			sys_write32((uint32_t)data->phys_addr,
				    DEVICE_MMIO_GET(dev) + DMA_INTEL_LPSS_REMAP_LOW);
			sys_write32((uint32_t)(data->phys_addr >> DMA_INTEL_LPSS_ADDR_RIGHT_SHIFT),
				    DEVICE_MMIO_GET(dev) + DMA_INTEL_LPSS_REMAP_HI);
		}
#endif
	}
#endif
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
	int ret = -1;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = ns16550_read_char(dev, c);

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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	int i;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	for (i = 0; (i < size) && (ns16550_read_char(dev, &rx_data[i]) != -1); i++) {
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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
	const struct uart_ns16550_dev_config * const dev_cfg = dev->config;

	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	} else if ((IS_ENABLED(CONFIG_UART_NS16550_DW8250_DW_APB)) &&
	    ((ns16550_inbyte(dev_cfg, IIR(dev)) & IIR_MASK) == IIR_BUSY)) {
		/*
		 * The Synopsys DesignWare 8250 has an extra feature whereby
		 * it detects if the LCR is written whilst busy.
		 * If it is, then a busy detect interrupt is raised,
		 * the uart status register need to be read.
		 */
		ns16550_inword(dev_cfg, USR(dev));
	}

#if (IS_ENABLED(CONFIG_UART_ASYNC_API))
	if (dev_data->async.tx_dma_params.dma_dev != NULL) {
		const struct uart_ns16550_dev_config * const config = dev->config;
		uint8_t IIR_status = ns16550_inbyte(config, IIR(dev));
#if (IS_ENABLED(CONFIG_UART_NS16550_INTEL_LPSS_DMA))
		uint32_t dma_status = ns16550_inword(config, SRC_TRAN(dev));

		if (dma_status & BIT(dev_data->async.rx_dma_params.dma_channel)) {
			async_timer_start(&dev_data->async.rx_dma_params.timeout_work,
					  dev_data->async.rx_dma_params.timeout_us);
			ns16550_outword(config, CLR_SRC_TRAN(dev),
					BIT(dev_data->async.rx_dma_params.dma_channel));
			return;
		}
		dma_intel_lpss_isr(dev_data->async.rx_dma_params.dma_dev);
#endif
		if (IIR_status & IIR_RBRF) {
			async_timer_start(&dev_data->async.rx_dma_params.timeout_work,
					  dev_data->async.rx_dma_params.timeout_us);
			return;
		}
	}
#endif

#ifdef CONFIG_UART_NS16550_WA_ISR_REENABLE_INTERRUPT
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
	const struct uart_ns16550_dev_config *const dev_cfg = dev->config;
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
		const struct uart_ns16550_dev_config * const dev_cfg = dev->config;
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

#if (IS_ENABLED(CONFIG_UART_ASYNC_API))
static void async_user_callback(const struct device *dev, struct uart_event *evt)
{
	const struct uart_ns16550_dev_data *data = dev->data;

	if (data->async.user_callback) {
		data->async.user_callback(dev, evt, data->async.user_data);
	}
}

#if UART_NS16550_DMAS_ENABLED
static void async_evt_tx_done(struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	struct uart_ns16550_tx_dma_params *tx_params = &data->async.tx_dma_params;

	(void)k_work_cancel_delayable(&data->async.tx_dma_params.timeout_work);

	struct uart_event event = {
		.type = UART_TX_DONE,
		.data.tx.buf = tx_params->buf,
		.data.tx.len = tx_params->buf_len
	};

	tx_params->buf = NULL;
	tx_params->buf_len = 0U;

	async_user_callback(dev, &event);
}
#endif

static void async_evt_rx_rdy(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	struct uart_ns16550_rx_dma_params *dma_params = &data->async.rx_dma_params;

	struct uart_event event = {
		.type = UART_RX_RDY,
		.data.rx.buf = dma_params->buf,
		.data.rx.len = dma_params->counter - dma_params->offset,
		.data.rx.offset = dma_params->offset
	};

	dma_params->offset = dma_params->counter;

	if (event.data.rx.len > 0) {
		async_user_callback(dev, &event);
	}
}

static void async_evt_rx_buf_release(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = (struct uart_ns16550_dev_data *)dev->data;
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = data->async.rx_dma_params.buf
	};

	async_user_callback(dev, &evt);
	data->async.rx_dma_params.buf = NULL;
	data->async.rx_dma_params.buf_len = 0U;
	data->async.rx_dma_params.offset = 0U;
	data->async.rx_dma_params.counter = 0U;
}

static void async_evt_rx_buf_request(const struct device *dev)
{
	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST
	};
	async_user_callback(dev, &evt);
}

static void uart_ns16550_async_rx_flush(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	struct uart_ns16550_rx_dma_params *dma_params = &data->async.rx_dma_params;
	struct dma_status status;

	dma_get_status(dma_params->dma_dev,
		       dma_params->dma_channel,
		       &status);

	const int rx_count = dma_params->buf_len - status.pending_length;

	if (rx_count > dma_params->counter) {
		dma_params->counter = rx_count;
		async_evt_rx_rdy(dev);
	}
}

static int uart_ns16550_rx_disable(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = (struct uart_ns16550_dev_data *)dev->data;
	struct uart_ns16550_rx_dma_params *dma_params = &data->async.rx_dma_params;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = 0;

	if (!device_is_ready(dma_params->dma_dev)) {
		ret = -ENODEV;
		goto out;
	}

	(void)k_work_cancel_delayable(&data->async.rx_dma_params.timeout_work);

	if (dma_params->buf && (dma_params->buf_len > 0)) {
		uart_ns16550_async_rx_flush(dev);
		async_evt_rx_buf_release(dev);
		if (data->async.next_rx_buffer != NULL) {
			dma_params->buf = data->async.next_rx_buffer;
			dma_params->buf_len = data->async.next_rx_buffer_len;
			data->async.next_rx_buffer = NULL;
			data->async.next_rx_buffer_len = 0;
			async_evt_rx_buf_release(dev);
		}
	}
	ret = dma_stop(dma_params->dma_dev,
		       dma_params->dma_channel);

	struct uart_event event = {
		.type = UART_RX_DISABLED
	};

	async_user_callback(dev, &event);

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static void prepare_rx_dma_block_config(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = (struct uart_ns16550_dev_data *)dev->data;
	struct uart_ns16550_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;

	assert(rx_dma_params->buf != NULL);
	assert(rx_dma_params->buf_len > 0);

	struct dma_block_config *head_block_config = &rx_dma_params->active_dma_block;

	head_block_config->dest_address = (uintptr_t)rx_dma_params->buf;
	head_block_config->source_address = data->phys_addr;
	head_block_config->block_size = rx_dma_params->buf_len;
}

#if UART_NS16550_DMAS_ENABLED
static void dma_callback(const struct device *dev, void *user_data, uint32_t channel,
			 int status)
{
	struct device *uart_dev = (struct device *)user_data;
	struct uart_ns16550_dev_data *data = (struct uart_ns16550_dev_data *)uart_dev->data;
	struct uart_ns16550_rx_dma_params *rx_params = &data->async.rx_dma_params;
	struct uart_ns16550_tx_dma_params *tx_params = &data->async.tx_dma_params;

	if (channel == tx_params->dma_channel) {
		async_evt_tx_done(uart_dev);
	} else if (channel == rx_params->dma_channel) {

		rx_params->counter = rx_params->buf_len;

		async_evt_rx_rdy(uart_dev);
		async_evt_rx_buf_release(uart_dev);

		rx_params->buf = data->async.next_rx_buffer;
		rx_params->buf_len = data->async.next_rx_buffer_len;
		data->async.next_rx_buffer = NULL;
		data->async.next_rx_buffer_len = 0U;

		if (rx_params->buf != NULL &&
		    rx_params->buf_len > 0) {
			dma_reload(dev, rx_params->dma_channel, data->phys_addr,
				   (uintptr_t)rx_params->buf, rx_params->buf_len);
			dma_start(dev, rx_params->dma_channel);
			async_evt_rx_buf_request(uart_dev);
		} else {
			uart_ns16550_rx_disable(uart_dev);
		}
	}
}
#endif

static int uart_ns16550_callback_set(const struct device *dev, uart_callback_t callback,
				    void *user_data)
{
	struct uart_ns16550_dev_data *data = dev->data;

	data->async.user_callback = callback;
	data->async.user_data = user_data;

	return 0;
}

static int uart_ns16550_tx(const struct device *dev, const uint8_t *buf, size_t len,
			  int32_t timeout_us)
{
	struct uart_ns16550_dev_data *data = dev->data;
	struct uart_ns16550_tx_dma_params *tx_params = &data->async.tx_dma_params;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = 0;

	if (!device_is_ready(tx_params->dma_dev)) {
		ret = -ENODEV;
		goto out;
	}

	tx_params->buf = buf;
	tx_params->buf_len = len;
	tx_params->active_dma_block.source_address = (uintptr_t)buf;
	tx_params->active_dma_block.dest_address = data->phys_addr;
	tx_params->active_dma_block.block_size = len;
	tx_params->active_dma_block.next_block = NULL;

	ret = dma_config(tx_params->dma_dev,
			 tx_params->dma_channel,
			 (struct dma_config *)&tx_params->dma_cfg);

	if (ret == 0) {
		ret = dma_start(tx_params->dma_dev,
				tx_params->dma_channel);
		if (ret) {
			ret = -EIO;
			goto out;
		}
		async_timer_start(&data->async.tx_dma_params.timeout_work, timeout_us);
	}

out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int uart_ns16550_tx_abort(const struct device *dev)
{
	struct uart_ns16550_dev_data *data = dev->data;
	struct uart_ns16550_tx_dma_params *tx_params = &data->async.tx_dma_params;
	struct dma_status status;
	int ret = 0;
	size_t bytes_tx;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!device_is_ready(tx_params->dma_dev)) {
		ret = -ENODEV;
		goto out;
	}

	(void)k_work_cancel_delayable(&data->async.tx_dma_params.timeout_work);

	ret = dma_stop(tx_params->dma_dev, tx_params->dma_channel);
	dma_get_status(tx_params->dma_dev,
		       tx_params->dma_channel,
		       &status);
	bytes_tx = tx_params->buf_len - status.pending_length;

	if (ret == 0) {
		struct uart_event tx_aborted_event = {
			.type = UART_TX_ABORTED,
			.data.tx.buf = tx_params->buf,
			.data.tx.len = bytes_tx
		};
		async_user_callback(dev, &tx_aborted_event);
	}
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int uart_ns16550_rx_enable(const struct device *dev, uint8_t *buf, const size_t len,
				  const int32_t timeout_us)
{
	struct uart_ns16550_dev_data *data = dev->data;
	const struct uart_ns16550_dev_config *config = dev->config;
	struct uart_ns16550_rx_dma_params *rx_dma_params = &data->async.rx_dma_params;
	int ret = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!device_is_ready(rx_dma_params->dma_dev)) {
		ret = -ENODEV;
		goto out;
	}

	rx_dma_params->timeout_us = timeout_us;
	rx_dma_params->buf = buf;
	rx_dma_params->buf_len = len;

#if defined(CONFIG_UART_NS16550_INTEL_LPSS_DMA)
	ns16550_outword(config, MST(dev), UNMASK_LPSS_INT(rx_dma_params->dma_channel));
#else
	ns16550_outbyte(config, IER(dev),
			(ns16550_inbyte(config, IER(dev)) | IER_RXRDY));
	ns16550_outbyte(config, FCR(dev), FCR_FIFO);
#endif
	prepare_rx_dma_block_config(dev);
	dma_config(rx_dma_params->dma_dev,
		   rx_dma_params->dma_channel,
		   (struct dma_config *)&rx_dma_params->dma_cfg);
	dma_start(rx_dma_params->dma_dev, rx_dma_params->dma_channel);
	async_evt_rx_buf_request(dev);
out:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int uart_ns16550_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_ns16550_dev_data *data = dev->data;

	assert(data->async.next_rx_buffer == NULL);
	assert(data->async.next_rx_buffer_len == 0);
	data->async.next_rx_buffer = buf;
	data->async.next_rx_buffer_len = len;

	return 0;
}

static void uart_ns16550_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *work_delay = CONTAINER_OF(work, struct k_work_delayable, work);
	struct uart_ns16550_rx_dma_params *rx_params =
			CONTAINER_OF(work_delay, struct uart_ns16550_rx_dma_params,
				     timeout_work);
	struct uart_ns16550_async_data *async_data =
			CONTAINER_OF(rx_params, struct uart_ns16550_async_data,
				     rx_dma_params);
	const struct device *dev = async_data->uart_dev;

	uart_ns16550_async_rx_flush(dev);

}

static void uart_ns16550_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *work_delay = CONTAINER_OF(work, struct k_work_delayable, work);
	struct uart_ns16550_tx_dma_params *tx_params =
			CONTAINER_OF(work_delay, struct uart_ns16550_tx_dma_params,
				     timeout_work);
	struct uart_ns16550_async_data *async_data =
			CONTAINER_OF(tx_params, struct uart_ns16550_async_data,
				     tx_dma_params);
	const struct device *dev = async_data->uart_dev;

	(void)uart_ns16550_tx_abort(dev);
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
	.callback_set = uart_ns16550_callback_set,
	.tx = uart_ns16550_tx,
	.tx_abort = uart_ns16550_tx_abort,
	.rx_enable = uart_ns16550_rx_enable,
	.rx_disable = uart_ns16550_rx_disable,
	.rx_buf_rsp = uart_ns16550_rx_buf_rsp,
#endif

#ifdef CONFIG_UART_NS16550_LINE_CTRL
	.line_ctrl_set = uart_ns16550_line_ctrl_set,
#endif

#ifdef CONFIG_UART_NS16550_DRV_CMD
	.drv_cmd = uart_ns16550_drv_cmd,
#endif
};

#define UART_NS16550_IRQ_FLAGS(n) \
	COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, sense),                           \
		    (DT_INST_IRQ(n, sense)),                                  \
		    (0))

/* IO-port or MMIO based UART */
#define UART_NS16550_IRQ_CONFIG(n)                                            \
	static void uart_ns16550_irq_config_func##n(const struct device *dev) \
	{                                                                     \
		ARG_UNUSED(dev);                                              \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	      \
			    uart_ns16550_isr, DEVICE_DT_INST_GET(n),	      \
			    UART_NS16550_IRQ_FLAGS(n));			      \
		irq_enable(DT_INST_IRQN(n));                                  \
	}

/* PCI(e) with auto IRQ detection */
#define UART_NS16550_IRQ_CONFIG_PCIE(n)                                       \
	static void uart_ns16550_irq_config_func##n(const struct device *dev) \
	{                                                                     \
		BUILD_ASSERT(DT_INST_IRQN(n) == PCIE_IRQ_DETECT,              \
			     "Only runtime IRQ configuration is supported");  \
		BUILD_ASSERT(IS_ENABLED(CONFIG_DYNAMIC_INTERRUPTS),           \
			     "NS16550 PCIe requires dynamic interrupts");     \
		const struct uart_ns16550_dev_config *dev_cfg = dev->config;  \
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

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define DEV_CONFIG_IRQ_FUNC_INIT(n) \
	.irq_config_func = uart_ns16550_irq_config_func##n,
#define UART_NS16550_IRQ_FUNC_DECLARE(n) \
	static void uart_ns16550_irq_config_func##n(const struct device *dev);
#define UART_NS16550_IRQ_FUNC_DEFINE(n) \
	UART_NS16550_IRQ_CONFIG(n)

#define DEV_CONFIG_PCIE_IRQ_FUNC_INIT(n) \
	.irq_config_func = uart_ns16550_irq_config_func##n,
#define UART_NS16550_PCIE_IRQ_FUNC_DECLARE(n) \
	static void uart_ns16550_irq_config_func##n(const struct device *dev);
#define UART_NS16550_PCIE_IRQ_FUNC_DEFINE(n) \
	UART_NS16550_IRQ_CONFIG_PCIE(n)
#else
/* !CONFIG_UART_INTERRUPT_DRIVEN */
#define DEV_CONFIG_IRQ_FUNC_INIT(n)
#define UART_NS16550_IRQ_FUNC_DECLARE(n)
#define UART_NS16550_IRQ_FUNC_DEFINE(n)

#define DEV_CONFIG_PCIE_IRQ_FUNC_INIT(n)
#define UART_NS16550_PCIE_IRQ_FUNC_DECLARE(n)
#define UART_NS16550_PCIE_IRQ_FUNC_DEFINE(n)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
#define DMA_PARAMS(n)								\
	.async.tx_dma_params = {						\
		.dma_dev =							\
			DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),	\
		.dma_channel =							\
			DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),		\
		.dma_cfg = {							\
			.source_burst_length = 1,				\
			.dest_burst_length = 1,					\
			.source_data_size = 1,					\
			.dest_data_size = 1,					\
			.complete_callback_en = 0,				\
			.error_callback_dis = 1,				\
			.block_count = 1,					\
			.channel_direction = MEMORY_TO_PERIPHERAL,		\
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),	\
			.dma_callback = dma_callback,				\
			.user_data = (void *)DEVICE_DT_INST_GET(n)		\
		},								\
	},									\
	.async.rx_dma_params = {						\
		.dma_dev =							\
			DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),	\
		.dma_channel =							\
			DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),		\
		.dma_cfg = {							\
			.source_burst_length = 1,				\
			.dest_burst_length = 1,					\
			.source_data_size = 1,					\
			.dest_data_size = 1,					\
			.complete_callback_en = 0,				\
			.error_callback_dis = 1,				\
			.block_count = 1,					\
			.channel_direction = PERIPHERAL_TO_MEMORY,		\
			.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),	\
			.dma_callback = dma_callback,				\
			.user_data = (void *)DEVICE_DT_INST_GET(n)		\
		},								\
	},									\
	COND_CODE_0(DT_INST_ON_BUS(n, pcie),					\
			(.phys_addr = DT_INST_REG_ADDR(n),), ())

#define DMA_PARAMS_NULL(n)							\
	.async.tx_dma_params = {						\
		.dma_dev = NULL							\
	},									\
	.async.rx_dma_params = {						\
		.dma_dev = NULL							\
	},									\

#define DEV_DATA_ASYNC(n)							\
	COND_CODE_0(DT_INST_PROP(n, io_mapped),					\
			(COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),		\
				(DMA_PARAMS(n)), (DMA_PARAMS_NULL(n)))),	\
				(DMA_PARAMS_NULL(n)))
#else
#define DEV_DATA_ASYNC(n)
#endif /* CONFIG_UART_ASYNC_API */


#define UART_NS16550_COMMON_DEV_CFG_INITIALIZER(n)                                   \
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
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, pcp),                            \
			(.pcp = DT_INST_PROP_OR(n, pcp, 0),))                        \
		.reg_interval = (1 << DT_INST_PROP(n, reg_shift)),                   \
		IF_ENABLED(CONFIG_PINCTRL,                                           \
			(.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),))              \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, resets),                         \
			(.reset_spec = RESET_DT_SPEC_INST_GET(n),))

#define UART_NS16550_COMMON_DEV_DATA_INITIALIZER(n)                                  \
		.uart_config.baudrate = DT_INST_PROP_OR(n, current_speed, 0),        \
		.uart_config.parity = UART_CFG_PARITY_NONE,                          \
		.uart_config.stop_bits = UART_CFG_STOP_BITS_1,                       \
		.uart_config.data_bits = UART_CFG_DATA_BITS_8,                       \
		.uart_config.flow_ctrl =                                             \
			COND_CODE_1(DT_INST_PROP_OR(n, hw_flow_control, 0),          \
				    (UART_CFG_FLOW_CTRL_RTS_CTS),                    \
				    (UART_CFG_FLOW_CTRL_NONE)),                      \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(n, dlf),                            \
			(.dlf = DT_INST_PROP_OR(n, dlf, 0),))			     \
		DEV_DATA_ASYNC(n)						     \

#define UART_NS16550_DEVICE_IO_MMIO_INIT(n)                                          \
	UART_NS16550_IRQ_FUNC_DECLARE(n);                                            \
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_INST_DEFINE(n)));                     \
	static const struct uart_ns16550_dev_config uart_ns16550_dev_cfg_##n = {     \
		COND_CODE_1(DT_INST_PROP_OR(n, io_mapped, 0),                        \
			    (.port = DT_INST_REG_ADDR(n),),                          \
			    (DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),))                 \
		IF_ENABLED(DT_INST_PROP_OR(n, io_mapped, 0),                         \
			   (.io_map = true,))                                        \
		UART_NS16550_COMMON_DEV_CFG_INITIALIZER(n)                           \
		DEV_CONFIG_IRQ_FUNC_INIT(n)                                          \
	};                                                                           \
	static struct uart_ns16550_dev_data uart_ns16550_dev_data_##n = {            \
		UART_NS16550_COMMON_DEV_DATA_INITIALIZER(n)                          \
	};                                                                           \
	DEVICE_DT_INST_DEFINE(n, uart_ns16550_init, NULL,                            \
			      &uart_ns16550_dev_data_##n, &uart_ns16550_dev_cfg_##n, \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,             \
			      &uart_ns16550_driver_api);                             \
	UART_NS16550_IRQ_FUNC_DEFINE(n)

#define UART_NS16550_DEVICE_PCIE_INIT(n)                                             \
	UART_NS16550_PCIE_IRQ_FUNC_DECLARE(n);                                       \
	DEVICE_PCIE_INST_DECLARE(n);                                                 \
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_INST_DEFINE(n)));                     \
	static const struct uart_ns16550_dev_config uart_ns16550_dev_cfg_##n = {     \
		UART_NS16550_COMMON_DEV_CFG_INITIALIZER(n)                           \
		DEV_CONFIG_PCIE_IRQ_FUNC_INIT(n)                                     \
		DEVICE_PCIE_INST_INIT(n, pcie)                                       \
	};                                                                           \
	static struct uart_ns16550_dev_data uart_ns16550_dev_data_##n = {            \
		UART_NS16550_COMMON_DEV_DATA_INITIALIZER(n)                          \
	};                                                                           \
	DEVICE_DT_INST_DEFINE(n, uart_ns16550_init, NULL,                            \
			      &uart_ns16550_dev_data_##n, &uart_ns16550_dev_cfg_##n, \
			      PRE_KERNEL_1,            \
			      CONFIG_SERIAL_INIT_PRIORITY,                           \
			      &uart_ns16550_driver_api);                             \
	UART_NS16550_PCIE_IRQ_FUNC_DEFINE(n)

#define UART_NS16550_DEVICE_INIT(n)                                                  \
	COND_CODE_1(DT_INST_ON_BUS(n, pcie),                                         \
		    (UART_NS16550_DEVICE_PCIE_INIT(n)),                              \
		    (UART_NS16550_DEVICE_IO_MMIO_INIT(n)))

DT_INST_FOREACH_STATUS_OKAY(UART_NS16550_DEVICE_INIT)
