/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gaisler_apbuart

#include <zephyr/drivers/uart.h>
#include <errno.h>

/* APBUART registers
 *
 * Offset | Name   | Description
 * ------ | ------ | ----------------------------------------
 * 0x0000 | data   | UART data register
 * 0x0004 | status | UART status register
 * 0x0008 | ctrl   | UART control register
 * 0x000c | scaler | UART scaler register
 * 0x0010 | debug  | UART FIFO debug register
 */

struct apbuart_regs {
	/** @brief UART data register
	 *
	 * Bit    | Name   | Description
	 * ------ | ------ | ----------------------------------------
	 * 7-0    | data   | Holding register or FIFO
	 */
	uint32_t data;          /* 0x0000 */

	/** @brief UART status register
	 *
	 * Bit    | Name   | Description
	 * ------ | ------ | ----------------------------------------
	 * 31-26  | RCNT   | Receiver FIFO count
	 * 25-20  | TCNT   | Transmitter FIFO count
	 * 10     | RF     | Receiver FIFO full
	 * 9      | TF     | Transmitter FIFO full
	 * 8      | RH     | Receiver FIFO half-full
	 * 7      | TH     | Transmitter FIFO half-full
	 * 6      | FE     | Framing error
	 * 5      | PE     | Parity error
	 * 4      | OV     | Overrun
	 * 3      | BR     | Break received
	 * 2      | TE     | Transmitter FIFO empty
	 * 1      | TS     | Transmitter shift register empty
	 * 0      | DR     | Data ready
	 */
	uint32_t status;        /* 0x0004 */

	/** @brief UART control register
	 *
	 * Bit    | Name   | Description
	 * ------ | ------ | ----------------------------------------
	 * 31     | FA     | FIFOs available
	 * 14     | SI     | Transmitter shift register empty interrupt enable
	 * 13     | DI     | Delayed interrupt enable
	 * 12     | BI     | Break interrupt enable
	 * 11     | DB     | FIFO debug mode enable
	 * 10     | RF     | Receiver FIFO interrupt enable
	 * 9      | TF     | Transmitter FIFO interrupt enable
	 * 8      | EC     | External clock
	 * 7      | LB     | Loop back
	 * 6      | FL     | Flow control
	 * 5      | PE     | Parity enable
	 * 4      | PS     | Parity select
	 * 3      | TI     | Transmitter interrupt enable
	 * 2      | RI     | Receiver interrupt enable
	 * 1      | TE     | Transmitter enable
	 * 0      | RE     | Receiver enable
	 */
	uint32_t ctrl;          /* 0x0008 */

	/** @brief UART scaler register
	 *
	 * Bit    | Name   | Description
	 * ------ | ------ | ----------------------------------------
	 * 11-0   | RELOAD | Scaler reload value
	 */
	uint32_t scaler;        /* 0x000c */

	/** @brief UART FIFO debug register
	 *
	 * Bit    | Name   | Description
	 * ------ | ------ | ----------------------------------------
	 * 7-0    | data   | Holding register or FIFO
	 */
	uint32_t debug;         /* 0x0010 */
};

/* APBUART register bits. */

/* Control register */
#define APBUART_CTRL_FA         (1 << 31)
#define APBUART_CTRL_DB         (1 << 11)
#define APBUART_CTRL_RF         (1 << 10)
#define APBUART_CTRL_TF         (1 <<  9)
#define APBUART_CTRL_LB         (1 <<  7)
#define APBUART_CTRL_FL         (1 <<  6)
#define APBUART_CTRL_PE         (1 <<  5)
#define APBUART_CTRL_PS         (1 <<  4)
#define APBUART_CTRL_TI         (1 <<  3)
#define APBUART_CTRL_RI         (1 <<  2)
#define APBUART_CTRL_TE         (1 <<  1)
#define APBUART_CTRL_RE         (1 <<  0)

/* Status register */
#define APBUART_STATUS_RF       (1 << 10)
#define APBUART_STATUS_TF       (1 <<  9)
#define APBUART_STATUS_RH       (1 <<  8)
#define APBUART_STATUS_TH       (1 <<  7)
#define APBUART_STATUS_FE       (1 <<  6)
#define APBUART_STATUS_PE       (1 <<  5)
#define APBUART_STATUS_OV       (1 <<  4)
#define APBUART_STATUS_BR       (1 <<  3)
#define APBUART_STATUS_TE       (1 <<  2)
#define APBUART_STATUS_TS       (1 <<  1)
#define APBUART_STATUS_DR       (1 <<  0)

/* For APBUART implemented without FIFO */
#define APBUART_STATUS_HOLD_REGISTER_EMPTY (1 << 2)

struct apbuart_dev_cfg {
	struct apbuart_regs *regs;
	int interrupt;
};

struct apbuart_dev_data {
	int usefifo;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb;
	void *cb_data;
#endif
};

/*
 * This routine waits for the TX holding register or TX FIFO to be ready and
 * then it writes a character to the data register.
 */
static void apbuart_poll_out(const struct device *dev, unsigned char x)
{
	const struct apbuart_dev_cfg *config = dev->config;
	struct apbuart_dev_data *data = dev->data;
	volatile struct apbuart_regs *regs = (void *) config->regs;

	if (data->usefifo) {
		/* Transmitter FIFO full flag is available. */
		while (regs->status & APBUART_STATUS_TF) {
			;
		}
	} else {
		/*
		 * Transmitter "hold register empty" AKA "FIFO empty" flag is
		 * available.
		 */
		while (!(regs->status & APBUART_STATUS_HOLD_REGISTER_EMPTY)) {
			;
		}
	}

	regs->data = x & 0xff;
}

static int apbuart_poll_in(const struct device *dev, unsigned char *c)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;

	if ((regs->status & APBUART_STATUS_DR) == 0) {
		return -1;
	}
	*c = regs->data & 0xff;

	return 0;
}

static int apbuart_err_check(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;
	const uint32_t status = regs->status;
	int err = 0;

	if (status & APBUART_STATUS_FE) {
		err |= UART_ERROR_FRAMING;
	}
	if (status & APBUART_STATUS_PE) {
		err |= UART_ERROR_PARITY;
	}
	if (status & APBUART_STATUS_OV) {
		err |= UART_ERROR_OVERRUN;
	}
	if (status & APBUART_STATUS_BR) {
		err |= UART_BREAK;
	}

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int get_baud(volatile struct apbuart_regs *const regs)
{
	unsigned int core_clk_hz;
	unsigned int scaler;

	scaler = regs->scaler;
	core_clk_hz = sys_clock_hw_cycles_per_sec();

	/* Calculate baud rate from generator "scaler" number */
	return core_clk_hz / ((scaler + 1) * 8);
}

static void set_baud(volatile struct apbuart_regs *const regs, uint32_t baud)
{
	unsigned int core_clk_hz;
	unsigned int scaler;

	if (baud == 0) {
		return;
	}

	core_clk_hz = sys_clock_hw_cycles_per_sec();

	/* Calculate Baud rate generator "scaler" number */
	scaler = (((core_clk_hz * 10) / (baud * 8)) - 5) / 10;

	/* Set new baud rate by setting scaler */
	regs->scaler = scaler;
}

static int apbuart_configure(const struct device *dev,
			     const struct uart_config *cfg)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;
	uint32_t ctrl = 0;
	uint32_t newctrl = 0;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_EVEN:
		newctrl |= APBUART_CTRL_PE;
		break;
	case UART_CFG_PARITY_ODD:
		newctrl |= APBUART_CTRL_PE | APBUART_CTRL_PS;
		break;
	default:
		return -ENOTSUP;
	}

	if (cfg->stop_bits != UART_CFG_STOP_BITS_1) {
		return -ENOTSUP;
	}

	if (cfg->data_bits != UART_CFG_DATA_BITS_8) {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		newctrl |= APBUART_CTRL_FL;
		break;
	default:
		return -ENOTSUP;
	}

	set_baud(regs, cfg->baudrate);

	ctrl = regs->ctrl;
	ctrl &= ~(APBUART_CTRL_PE | APBUART_CTRL_PS | APBUART_CTRL_FL);
	regs->ctrl = ctrl | newctrl;

	return 0;
}

static int apbuart_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;
	const uint32_t ctrl = regs->ctrl;

	cfg->parity = UART_CFG_PARITY_NONE;
	if (ctrl & APBUART_CTRL_PE) {
		if (ctrl & APBUART_CTRL_PS) {
			cfg->parity = UART_CFG_PARITY_ODD;
		} else {
			cfg->parity = UART_CFG_PARITY_EVEN;
		}
	}

	cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	if (ctrl & APBUART_CTRL_FL) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	cfg->baudrate = get_baud(regs);

	cfg->data_bits = UART_CFG_DATA_BITS_8;
	cfg->stop_bits = UART_CFG_STOP_BITS_1;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void apbuart_isr(const struct device *dev);

static int apbuart_fifo_fill(const struct device *dev, const uint8_t *tx_data,
			     int size)
{
	const struct apbuart_dev_cfg *config = dev->config;
	struct apbuart_dev_data *data = dev->data;
	volatile struct apbuart_regs *regs = (void *) config->regs;
	int i;

	if (data->usefifo) {
		/* Transmitter FIFO full flag is available. */
		for (
			i = 0;
			(i < size) && !(regs->status & APBUART_STATUS_TF);
			i++
		) {
			regs->data = tx_data[i];
		}
		return i;
	}
	for (i = 0; (i < size) && (regs->status & APBUART_STATUS_TE); i++) {
		regs->data = tx_data[i];
	}

	return i;
}

static int apbuart_fifo_read(const struct device *dev, uint8_t *rx_data,
			     const int size)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;
	int i;

	for (i = 0; (i < size) && (regs->status & APBUART_STATUS_DR); i++) {
		rx_data[i] = regs->data & 0xff;
	}

	return i;
}

static void apbuart_irq_tx_enable(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	struct apbuart_dev_data *data = dev->data;
	volatile struct apbuart_regs *regs = (void *) config->regs;
	unsigned int key;

	if (data->usefifo) {
		/* Enable the FIFO level interrupt */
		regs->ctrl |= APBUART_CTRL_TF;
		return;
	}

	regs->ctrl |= APBUART_CTRL_TI;
	/*
	 * The "TI" interrupt is an edge interrupt.  It fires each time the TX
	 * holding register (or FIFO if implemented) moves from non-empty to
	 * empty.
	 *
	 * When the APBUART is implemented _without_ FIFO, the TI interrupt is
	 * the only TX interrupt we have. When the APBUART is implemented
	 * _with_ FIFO, the TI will fire on each TX byte.
	 */
	regs->ctrl |= APBUART_CTRL_TI;
	/* Fire the first "TI" edge interrupt to get things going. */
	key = irq_lock();
	apbuart_isr(dev);
	irq_unlock(key);
}

static void apbuart_irq_tx_disable(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;

	regs->ctrl &= ~(APBUART_CTRL_TF | APBUART_CTRL_TI);
}

static int apbuart_irq_tx_ready(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	struct apbuart_dev_data *data = dev->data;
	volatile struct apbuart_regs *regs = (void *) config->regs;

	if (data->usefifo) {
		return !(regs->status & APBUART_STATUS_TF);
	}
	return !!(regs->status & APBUART_STATUS_TE);
}

static int apbuart_irq_tx_complete(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;

	return !!(regs->status & APBUART_STATUS_TS);
}

static void apbuart_irq_rx_enable(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;

	regs->ctrl |= APBUART_CTRL_RI;
}

static void apbuart_irq_rx_disable(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;

	regs->ctrl &= ~APBUART_CTRL_RI;
}

static int apbuart_irq_rx_ready(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	volatile struct apbuart_regs *regs = (void *) config->regs;

	return !!(regs->status & APBUART_STATUS_DR);
}

static int apbuart_irq_is_pending(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	struct apbuart_dev_data *data = dev->data;
	volatile struct apbuart_regs *regs = (void *) config->regs;
	uint32_t status = regs->status;
	uint32_t ctrl = regs->ctrl;

	if ((ctrl & APBUART_CTRL_RI) && (status & APBUART_STATUS_DR)) {
		return 1;
	}

	if (data->usefifo) {
		/* TH is the TX FIFO half-empty flag */
		if (status & APBUART_STATUS_TH) {
			return 1;
		}
	} else {
		if ((ctrl & APBUART_CTRL_TI) && (status & APBUART_STATUS_TE)) {
			return 1;
		}
	}

	return 0;
}

static int apbuart_irq_update(const struct device *dev)
{
	return 1;
}

static void apbuart_irq_callback_set(const struct device *dev,
				     uart_irq_callback_user_data_t cb,
				     void *cb_data)
{
	struct apbuart_dev_data *const dev_data = dev->data;

	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

static void apbuart_isr(const struct device *dev)
{
	struct apbuart_dev_data *const dev_data = dev->data;

	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int apbuart_init(const struct device *dev)
{
	const struct apbuart_dev_cfg *config = dev->config;
	struct apbuart_dev_data *data = dev->data;
	volatile struct apbuart_regs *regs = (void *) config->regs;
	const uint32_t APBUART_DEBUG_MASK = APBUART_CTRL_DB | APBUART_CTRL_FL;
	uint32_t dm;
	uint32_t ctrl;

	ctrl = regs->ctrl;
	data->usefifo = !!(ctrl & APBUART_CTRL_FA);
	/* NOTE: CTRL_FL has reset value 0. CTRL_DB has no reset value. */
	dm = ctrl & APBUART_DEBUG_MASK;
	if (dm == APBUART_DEBUG_MASK) {
		/* Debug mode enabled so assume APBUART already initialized. */
		;
	} else {
		regs->ctrl = APBUART_CTRL_TE | APBUART_CTRL_RE;
	}

	regs->status = 0;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	irq_connect_dynamic(config->interrupt,
			    0, (void (*)(const void *))apbuart_isr, dev, 0);
	irq_enable(config->interrupt);
#endif

	return 0;
}

/* Driver API defined in uart.h */
static const struct uart_driver_api apbuart_driver_api = {
	.poll_in                = apbuart_poll_in,
	.poll_out               = apbuart_poll_out,
	.err_check              = apbuart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure              = apbuart_configure,
	.config_get             = apbuart_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill              = apbuart_fifo_fill,
	.fifo_read              = apbuart_fifo_read,
	.irq_tx_enable          = apbuart_irq_tx_enable,
	.irq_tx_disable         = apbuart_irq_tx_disable,
	.irq_tx_ready           = apbuart_irq_tx_ready,
	.irq_rx_enable          = apbuart_irq_rx_enable,
	.irq_rx_disable         = apbuart_irq_rx_disable,
	.irq_tx_complete        = apbuart_irq_tx_complete,
	.irq_rx_ready           = apbuart_irq_rx_ready,
	.irq_is_pending         = apbuart_irq_is_pending,
	.irq_update             = apbuart_irq_update,
	.irq_callback_set       = apbuart_irq_callback_set,
#endif
};

#define APBUART_INIT(index)						\
	static const struct apbuart_dev_cfg apbuart##index##_config = {	\
		.regs           = (struct apbuart_regs *)		\
				  DT_INST_REG_ADDR(index),		\
		.interrupt      = DT_INST_IRQN(index),			\
	};								\
									\
	static struct apbuart_dev_data apbuart##index##_data = {	\
		.usefifo        = 0,					\
	};								\
									\
	DEVICE_DT_INST_DEFINE(index,					\
			    &apbuart_init,				\
			    NULL,					\
			    &apbuart##index##_data,			\
			    &apbuart##index##_config,			\
			    PRE_KERNEL_1,				\
			    CONFIG_SERIAL_INIT_PRIORITY,		\
			    &apbuart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(APBUART_INIT)
