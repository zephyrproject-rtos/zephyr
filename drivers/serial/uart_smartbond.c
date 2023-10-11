/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_uart

#include <errno.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <DA1469xAB.h>
#include <zephyr/irq.h>

#define IIR_NO_INTR		1
#define IIR_THR_EMPTY		2
#define IIR_RX_DATA		4
#define IIR_LINE_STATUS		5
#define IIR_BUSY		7
#define IIR_TIMEOUT		12

#define STOP_BITS_1	0
#define STOP_BITS_2	1

#define DATA_BITS_5	0
#define DATA_BITS_6	1
#define DATA_BITS_7	2
#define DATA_BITS_8	3

#define RX_FIFO_TRIG_1_CHAR		0
#define RX_FIFO_TRIG_1_4_FULL		1
#define RX_FIFO_TRIG_1_2_FULL		2
#define RX_FIFO_TRIG_MINUS_2_CHARS	3

#define TX_FIFO_TRIG_EMPTY		0
#define TX_FIFO_TRIG_2_CHARS		1
#define TX_FIFO_TRIG_1_4_FULL		2
#define TX_FIFO_TRIG_1_2_FULL		3

#define BAUDRATE_CFG_DLH(cfg)		(((cfg) >> 16) & 0xff)
#define BAUDRATE_CFG_DLL(cfg)		(((cfg) >> 8) & 0xff)
#define BAUDRATE_CFG_DLF(cfg)		((cfg) & 0xff)

struct uart_smartbond_baudrate_cfg {
	uint32_t baudrate;
	/* DLH=cfg[23:16] DLL=cfg[15:8] DLF=cfg[7:0] */
	uint32_t cfg;
};

static const struct uart_smartbond_baudrate_cfg uart_smartbond_baudrate_table[] = {
	{ 2000000, 0x00000100 },
	{ 1000000, 0x00000200 },
	{  921600, 0x00000203 },
	{  500000, 0x00000400 },
	{  230400, 0x0000080b },
	{  115200, 0x00001106 },
	{   57600, 0x0000220c },
	{   38400, 0x00003401 },
	{   28800, 0x00004507 },
	{   19200, 0x00006803 },
	{   14400, 0x00008a0e },
	{    9600, 0x0000d005 },
	{    4800, 0x0001a00b },
};

struct uart_smartbond_cfg {
	UART2_Type *regs;
	int periph_clock_config;
	const struct pinctrl_dev_config *pcfg;
	bool hw_flow_control_supported;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_smartbond_data {
	struct uart_config current_config;
	struct k_spinlock lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *cb_data;
	uint32_t flags;
	uint8_t rx_enabled;
	uint8_t tx_enabled;
#endif
};

static int uart_smartbond_poll_in(const struct device *dev, unsigned char *p_char)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if ((config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_RFNE_Msk) == 0) {
		k_spin_unlock(&data->lock, key);
		return -1;
	}

	*p_char = config->regs->UART2_RBR_THR_DLL_REG;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static void uart_smartbond_poll_out(const struct device *dev, unsigned char out_char)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while (!(config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_TFNF_Msk)) {
		/* Wait until FIFO has free space */
	}

	config->regs->UART2_RBR_THR_DLL_REG = out_char;

	k_spin_unlock(&data->lock, key);
}

static int uart_smartbond_configure(const struct device *dev,
				  const struct uart_config *cfg)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	uint32_t baudrate_cfg = 0;
	k_spinlock_key_t key;
	uint32_t reg_val;
	int err;
	int i;

	if ((cfg->parity != UART_CFG_PARITY_NONE && cfg->parity != UART_CFG_PARITY_ODD &&
	     cfg->parity != UART_CFG_PARITY_EVEN) ||
	    (cfg->stop_bits != UART_CFG_STOP_BITS_1 && cfg->stop_bits != UART_CFG_STOP_BITS_2) ||
	    (cfg->data_bits != UART_CFG_DATA_BITS_5 && cfg->data_bits != UART_CFG_DATA_BITS_6 &&
	     cfg->data_bits != UART_CFG_DATA_BITS_7 && cfg->data_bits != UART_CFG_DATA_BITS_8) ||
	    (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE &&
	     cfg->flow_ctrl != UART_CFG_FLOW_CTRL_RTS_CTS)) {
		return -ENOTSUP;
	}

	/* Flow control is not supported on UART */
	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS &&
	    !config->hw_flow_control_supported) {
		return -ENOTSUP;
	}

	/* Lookup configuration for baudrate */
	for (i = 0; i < ARRAY_SIZE(uart_smartbond_baudrate_table); i++) {
		if (uart_smartbond_baudrate_table[i].baudrate == cfg->baudrate) {
			baudrate_cfg = uart_smartbond_baudrate_table[i].cfg;
			break;
		}
	}

	if (baudrate_cfg == 0) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	CRG_COM->SET_CLK_COM_REG = config->periph_clock_config;

	config->regs->UART2_SRR_REG = UART2_UART2_SRR_REG_UART_UR_Msk |
				      UART2_UART2_SRR_REG_UART_RFR_Msk |
				      UART2_UART2_SRR_REG_UART_XFR_Msk;

	config->regs->UART2_LCR_REG |= UART2_UART2_LCR_REG_UART_DLAB_Msk;
	config->regs->UART2_IER_DLH_REG = BAUDRATE_CFG_DLH(baudrate_cfg);
	config->regs->UART2_RBR_THR_DLL_REG = BAUDRATE_CFG_DLL(baudrate_cfg);
	config->regs->UART2_DLF_REG = BAUDRATE_CFG_DLF(baudrate_cfg);
	config->regs->UART2_LCR_REG &= ~UART2_UART2_LCR_REG_UART_DLAB_Msk;

	/* Configure frame */

	reg_val = 0;

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_EVEN:
		reg_val |= UART2_UART2_LCR_REG_UART_EPS_Msk;
		/* no break */
	case UART_CFG_PARITY_ODD:
		reg_val |= UART2_UART2_LCR_REG_UART_PEN_Msk;
		break;
	}

	if (cfg->stop_bits == UART_CFG_STOP_BITS_2)  {
		reg_val |= STOP_BITS_2 << UART2_UART2_LCR_REG_UART_STOP_Pos;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_6:
		reg_val |= DATA_BITS_6 << UART2_UART2_LCR_REG_UART_DLS_Pos;
		break;
	case UART_CFG_DATA_BITS_7:
		reg_val |= DATA_BITS_7 << UART2_UART2_LCR_REG_UART_DLS_Pos;
		break;
	case UART_CFG_DATA_BITS_8:
		reg_val |= DATA_BITS_8 << UART2_UART2_LCR_REG_UART_DLS_Pos;
		break;
	}

	config->regs->UART2_LCR_REG = reg_val;

	/* Enable hardware FIFO */
	config->regs->UART2_SFE_REG = UART2_UART2_SFE_REG_UART_SHADOW_FIFO_ENABLE_Msk;

	config->regs->UART2_SRT_REG = RX_FIFO_TRIG_1_CHAR;
	config->regs->UART2_STET_REG = TX_FIFO_TRIG_1_2_FULL;

	data->current_config = *cfg;

	k_spin_unlock(&data->lock, key);

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_smartbond_config_get(const struct device *dev,
				   struct uart_config *cfg)
{
	struct uart_smartbond_data *data = dev->data;

	*cfg = data->current_config;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_smartbond_init(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;

	return uart_smartbond_configure(dev, &data->current_config);
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static inline void irq_tx_enable(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	config->regs->UART2_IER_DLH_REG |= UART2_UART2_IER_DLH_REG_PTIME_DLH7_Msk |
					   UART2_UART2_IER_DLH_REG_ETBEI_DLH1_Msk;
}

static inline void irq_tx_disable(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	config->regs->UART2_IER_DLH_REG &= ~(UART2_UART2_IER_DLH_REG_PTIME_DLH7_Msk |
					     UART2_UART2_IER_DLH_REG_ETBEI_DLH1_Msk);
}

static inline void irq_rx_enable(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	config->regs->UART2_IER_DLH_REG |= UART2_UART2_IER_DLH_REG_ERBFI_DLH0_Msk;
}

static inline void irq_rx_disable(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	config->regs->UART2_IER_DLH_REG &= ~UART2_UART2_IER_DLH_REG_ERBFI_DLH0_Msk;
}

static int uart_smartbond_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int len)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	int num_tx = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while ((len - num_tx > 0) &&
	       (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_TFNF_Msk)) {
		config->regs->UART2_RBR_THR_DLL_REG = tx_data[num_tx++];
	}

	if (data->tx_enabled) {
		irq_tx_enable(dev);
	}

	k_spin_unlock(&data->lock, key);

	return num_tx;
}

static int uart_smartbond_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	const struct uart_smartbond_cfg *config = dev->config;
	struct uart_smartbond_data *data = dev->data;
	int num_rx = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while ((size - num_rx > 0) &&
	       (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_RFNE_Msk)) {
		rx_data[num_rx++] = config->regs->UART2_RBR_THR_DLL_REG;
	}

	if (data->rx_enabled) {
		irq_rx_enable(dev);
	}

	k_spin_unlock(&data->lock, key);

	return num_rx;
}

static void uart_smartbond_irq_tx_enable(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->tx_enabled = 1;
	irq_tx_enable(dev);

	k_spin_unlock(&data->lock, key);
}

static void uart_smartbond_irq_tx_disable(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_tx_disable(dev);
	data->tx_enabled = 0;

	k_spin_unlock(&data->lock, key);
}

static int uart_smartbond_irq_tx_ready(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	return (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_TFNF_Msk) != 0;
}

static void uart_smartbond_irq_rx_enable(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->rx_enabled = 1;
	irq_rx_enable(dev);

	k_spin_unlock(&data->lock, key);
}

static void uart_smartbond_irq_rx_disable(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_rx_disable(dev);
	data->rx_enabled = 0;

	k_spin_unlock(&data->lock, key);
}

static int uart_smartbond_irq_tx_complete(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	return (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_TFE_Msk) != 0;
}

static int uart_smartbond_irq_rx_ready(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;

	return (config->regs->UART2_USR_REG & UART2_UART2_USR_REG_UART_RFNE_Msk) != 0;
}

static void uart_smartbond_irq_err_enable(const struct device *dev)
{
	k_panic();
}

static void uart_smartbond_irq_err_disable(const struct device *dev)
{
	k_panic();
}

static int uart_smartbond_irq_is_pending(const struct device *dev)
{
	k_panic();

	return 0;
}

static int uart_smartbond_irq_update(const struct device *dev)
{
	const struct uart_smartbond_cfg *config = dev->config;
	bool no_intr = false;

	while (!no_intr) {
		switch (config->regs->UART2_IIR_FCR_REG & 0x0F) {
		case IIR_NO_INTR:
			no_intr = true;
			break;
		case IIR_THR_EMPTY:
			irq_tx_disable(dev);
			break;
		case IIR_RX_DATA:
			irq_rx_disable(dev);
			break;
		case IIR_LINE_STATUS:
		case IIR_TIMEOUT:
			/* ignore */
			break;
		case IIR_BUSY:
			/* busy detect */
			/* fall-through */
		default:
			k_panic();
			break;
		}
	}

	return 1;
}

static void uart_smartbond_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb,
					  void *cb_data)
{
	struct uart_smartbond_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

static void uart_smartbond_isr(const struct device *dev)
{
	struct uart_smartbond_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_smartbond_driver_api = {
	.poll_in = uart_smartbond_poll_in,
	.poll_out = uart_smartbond_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_smartbond_configure,
	.config_get = uart_smartbond_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_smartbond_fifo_fill,
	.fifo_read = uart_smartbond_fifo_read,
	.irq_tx_enable = uart_smartbond_irq_tx_enable,
	.irq_tx_disable = uart_smartbond_irq_tx_disable,
	.irq_tx_ready = uart_smartbond_irq_tx_ready,
	.irq_rx_enable = uart_smartbond_irq_rx_enable,
	.irq_rx_disable = uart_smartbond_irq_rx_disable,
	.irq_tx_complete = uart_smartbond_irq_tx_complete,
	.irq_rx_ready = uart_smartbond_irq_rx_ready,
	.irq_err_enable = uart_smartbond_irq_err_enable,
	.irq_err_disable = uart_smartbond_irq_err_disable,
	.irq_is_pending = uart_smartbond_irq_is_pending,
	.irq_update = uart_smartbond_irq_update,
	.irq_callback_set = uart_smartbond_irq_callback_set,
#endif  /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_SMARTBOND_CONFIGURE(id)			\
	do {						\
		IRQ_CONNECT(DT_INST_IRQN(id),		\
			    DT_INST_IRQ(id, priority),	\
			    uart_smartbond_isr,		\
			    DEVICE_DT_INST_GET(id), 0);	\
							\
		irq_enable(DT_INST_IRQN(id));		\
	} while (0)
#else
#define UART_SMARTBOND_CONFIGURE(id)
#endif

#define UART_SMARTBOND_DEVICE(id)								\
	PINCTRL_DT_INST_DEFINE(id);								\
	static const struct uart_smartbond_cfg uart_smartbond_##id##_cfg = {			\
		.regs = (UART2_Type *)DT_INST_REG_ADDR(id),					\
		.periph_clock_config = DT_INST_PROP(id, periph_clock_config),			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),					\
		.hw_flow_control_supported = DT_INST_PROP(id, hw_flow_control_supported),	\
	};											\
	static struct uart_smartbond_data uart_smartbond_##id##_data = {			\
		.current_config = {								\
			.baudrate = DT_INST_PROP(id, current_speed),				\
			.parity = UART_CFG_PARITY_NONE,						\
			.stop_bits = UART_CFG_STOP_BITS_1,					\
			.data_bits = UART_CFG_DATA_BITS_8,					\
			.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,					\
		},										\
	};											\
	static int uart_smartbond_##id##_init(const struct device *dev)				\
	{											\
		UART_SMARTBOND_CONFIGURE(id);							\
		return uart_smartbond_init(dev);						\
	}											\
	DEVICE_DT_INST_DEFINE(id,								\
			      uart_smartbond_##id##_init,					\
			      NULL,								\
			      &uart_smartbond_##id##_data,					\
			      &uart_smartbond_##id##_cfg,					\
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,			\
			      &uart_smartbond_driver_api);					\

DT_INST_FOREACH_STATUS_OKAY(UART_SMARTBOND_DEVICE)
