/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra6b1_uart

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/dma.h>
#include "r_uart_w_b.h"
#include "bsp_pd.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ra6b1_uart);

#include <system.h>

const static uart_api_t *fsp_uart = &g_uart_on_uart_w_b;

struct uart_ra6b1_config {
	UART_Type * const regs;
	const struct pinctrl_dev_config *pcfg;
};

struct uart_ra6b1_data {
	const struct device *dev;
	struct st_uart_w_b_instance_ctrl uart;
	struct uart_config uart_config;
	struct st_uart_cfg fsp_config;
	struct st_uart_w_b_extended_cfg fsp_config_extend;
	struct st_uart_w_b_baud_setting fsp_baud_setting;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_callback_user_data_t user_cb;
	void *user_cb_data;
	uint8_t irq_event;
	bool irq_event_from_update;
#endif
};

struct uart_ra6b1_baudrate_cfg {
	uint32_t baudrate;
	/* DLH=cfg[23:16] DLL=cfg[15:8] DLF=cfg[7:0] */
	uint32_t cfg;
};

static const struct uart_ra6b1_baudrate_cfg uart_ra6b1_baudrate_table[] = {
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

static int uart_ra6b1_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_ra6b1_config *cfg = dev->config;

	if ((cfg->regs->UART_LSR_REG & UART_UART_LSR_REG_UART_DR_Msk) == 0) {
		/* There are no characters available to read. */
		return -1;
	}

	/* got a character */
	*c = (unsigned char)cfg->regs->UART_RBR_THR_DLL_REG;

	return 0;
}

static void uart_ra6b1_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_ra6b1_config *cfg = dev->config;

#if defined(CONFIG_UART_RA6B1_UART_FIFO_ENABLE)
	while ((cfg->regs->UART_USR_REG & UART_UART_USR_REG_UART_TFNF_Msk) == 0) {
	}
#else
	while ((cfg->regs->UART_LSR_REG & UART_UART_LSR_REG_UART_THRE_Msk) == 0) {
	}
#endif

	cfg->regs->UART_RBR_THR_DLL_REG = (uint32_t)c;
}

static int uart_ra6b1_apply_config(const struct uart_config *config, struct st_uart_cfg *fsp_config,
				   struct st_uart_w_b_extended_cfg *fsp_config_extend,
				   struct st_uart_w_b_baud_setting *fsp_baud_setting)
{
	uint32_t baudrate_cfg = 0;
	fsp_err_t ret;

	/* Lookup configuration for baudrate */
	for (uint8_t i = 0; i < ARRAY_SIZE(uart_ra6b1_baudrate_table); i++) {
		if (uart_ra6b1_baudrate_table[i].baudrate == config->baudrate) {
			baudrate_cfg = uart_ra6b1_baudrate_table[i].cfg;
			break;
		}
	}

	if (baudrate_cfg == 0) {
		return -ENOTSUP;
	}

	ret = R_UART_W_B_BaudCalculate(baudrate_cfg, fsp_baud_setting);
	if (ret != FSP_SUCCESS) {
		LOG_ERR("Invalid baudrate settings");
		return -EINVAL;
	}

	switch (config->parity) {
	case UART_CFG_PARITY_NONE:
		fsp_config->parity = UART_PARITY_OFF;
		break;
	case UART_CFG_PARITY_ODD:
		fsp_config->parity = UART_PARITY_ODD;
		break;
	case UART_CFG_PARITY_EVEN:
		fsp_config->parity = UART_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_MARK:
		return -ENOTSUP;
	case UART_CFG_PARITY_SPACE:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (config->stop_bits) {
	case UART_CFG_STOP_BITS_0_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_1:
		fsp_config->stop_bits = UART_STOP_BITS_1;
		break;
	case UART_CFG_STOP_BITS_1_5:
		return -ENOTSUP;
	case UART_CFG_STOP_BITS_2:
		fsp_config->stop_bits = UART_STOP_BITS_2;
		break;
	default:
		return -EINVAL;
	}

	switch (config->data_bits) {
	case UART_CFG_DATA_BITS_5:
		fsp_config->data_bits = UART_DATA_BITS_5;
		break;
	case UART_CFG_DATA_BITS_6:
		fsp_config->data_bits = UART_DATA_BITS_6;
		break;
	case UART_CFG_DATA_BITS_7:
		fsp_config->data_bits = UART_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		fsp_config->data_bits = UART_DATA_BITS_8;
		break;
	case UART_CFG_DATA_BITS_9:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	switch (config->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		fsp_config_extend->flow_control = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		fsp_config_extend->flow_control = true;
		break;
	case UART_CFG_FLOW_CTRL_DTR_DSR:
		return -ENOTSUP;
	case UART_CFG_FLOW_CTRL_RS485:
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	return 0;
}

#if defined(CONFIG_UART_USE_RUNTIME_CONFIGURE)

static int uart_ra6b1_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_ra6b1_data *data = dev->data;
	int ret;

	ret = uart_ra6b1_apply_config(cfg, &data->fsp_config, &data->fsp_config_extend,
					&data->fsp_baud_setting);
	if (ret) {
		return ret;
	}

	if (data->uart.open) {
		fsp_uart->close(&data->uart);
	}

	ret = fsp_uart->open(&data->uart, &data->fsp_config);
	if ((fsp_err_t)ret != FSP_SUCCESS) {
		LOG_ERR("Failed to open UART instance with status %d", ret);
		return -EIO;
	}

	memcpy(&data->uart_config, cfg, sizeof(struct uart_config));

	return 0;
}

static int uart_ra6b1_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_ra6b1_data *data = dev->data;

	memcpy(cfg, &data->uart_config, sizeof(*cfg));
	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */



static inline void irq_tx_disable(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;

	cfg->regs->UART_IER_DLH_REG &= ~(UART_UART_IER_DLH_REG_PTIME_DLH7_Msk |
					UART_UART_IER_DLH_REG_ETBEI_DLH1_Msk);
}

static inline void irq_rx_disable(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;

	cfg->regs->UART_IER_DLH_REG &= ~UART_UART_IER_DLH_REG_ERBFI_DLH0_Msk;
}

static inline void irq_err_disable(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;

	cfg->regs->UART_IER_DLH_REG &= ~UART_UART_IER_DLH_REG_ELSI_DLH2_Msk;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
static inline void irq_tx_enable(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;

	cfg->regs->UART_IER_DLH_REG |= UART_UART_IER_DLH_REG_PTIME_DLH7_Msk |
					UART_UART_IER_DLH_REG_ETBEI_DLH1_Msk;
}

static inline void irq_rx_enable(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;

	cfg->regs->UART_IER_DLH_REG |= UART_UART_IER_DLH_REG_ERBFI_DLH0_Msk;
}

static inline void irq_err_enable(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;

	cfg->regs->UART_IER_DLH_REG |= UART_UART_IER_DLH_REG_ELSI_DLH2_Msk;
}

static int uart_ra6b1_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_ra6b1_config *cfg = dev->config;
	int num_tx = size;

	if (size <= 0 || tx_data == NULL) {
		return 0;
	}

	while ((num_tx > 0) &&
#if defined(CONFIG_UART_RA6B1_UART_FIFO_ENABLE)
		(cfg->regs->UART_USR_REG & UART_UART_USR_REG_UART_TFNF_Msk)) {
#else
		(cfg->regs->UART_LSR_REG & UART_UART_LSR_REG_UART_THRE_Msk)) {
#endif
		cfg->regs->UART_RBR_THR_DLL_REG = (uint32_t)*tx_data;
		++tx_data;
		--num_tx;
	}

	return size - num_tx;
}

static int uart_ra6b1_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_ra6b1_config *cfg = dev->config;
	int num_rx = size;

	if (size <= 0 || rx_data == NULL) {
		return 0;
	}

	while ((num_rx > 0) &&
		(cfg->regs->UART_LSR_REG & UART_UART_LSR_REG_UART_DR_Msk)) {
		*rx_data = (uint8_t)cfg->regs->UART_RBR_THR_DLL_REG;
		++rx_data;
		--num_rx;
	}

	return size - num_rx;
}

static void uart_ra6b1_irq_tx_enable(const struct device *dev)
{
	uint32_t key = irq_lock();

	irq_tx_enable(dev);

	irq_unlock(key);
}

static void uart_ra6b1_irq_tx_disable(const struct device *dev)
{
	uint32_t key = irq_lock();

	irq_tx_disable(dev);

	irq_unlock(key);
}

static int uart_ra6b1_irq_tx_ready(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;
	struct uart_ra6b1_data *data = dev->data;
	bool irq_event_flag = (data->irq_event == UART_W_B_INT_THR_EMPTY);
	int ret = irq_event_flag &&
#if defined(CONFIG_UART_RA6B1_UART_FIFO_ENABLE)
		  (cfg->regs->UART_USR_REG & UART_UART_USR_REG_UART_TFNF_Msk) != 0;
#else
		  (cfg->regs->UART_LSR_REG & UART_UART_LSR_REG_UART_THRE_Msk) != 0;
#endif

	return ret;
}

static int uart_ra6b1_irq_tx_complete(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;
	int ret = (cfg->regs->UART_USR_REG & UART_UART_USR_REG_UART_TFE_Msk) != 0;

	return ret;
}

static void uart_ra6b1_irq_rx_enable(const struct device *dev)
{
	uint32_t key = irq_lock();

	irq_rx_enable(dev);

	irq_unlock(key);
}

static void uart_ra6b1_irq_rx_disable(const struct device *dev)
{
	uint32_t key = irq_lock();

	irq_rx_disable(dev);

	irq_unlock(key);
}

static int uart_ra6b1_irq_rx_ready(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;
	struct uart_ra6b1_data *data = dev->data;
	bool irq_event_flag = (data->irq_event == UART_W_B_INT_RECEIVED_AVAILABLE) ||
			      (data->irq_event == UART_W_B_INT_TIMEOUT);
	int ret = irq_event_flag && (cfg->regs->UART_LSR_REG & UART_UART_LSR_REG_UART_DR_Msk) != 0;

	return ret;
}

static void uart_ra6b1_irq_err_enable(const struct device *dev)
{
	uint32_t key = irq_lock();

	irq_err_enable(dev);

	irq_unlock(key);
}

static void uart_ra6b1_irq_err_disable(const struct device *dev)
{
	uint32_t key = irq_lock();

	irq_err_disable(dev);

	irq_unlock(key);
}

static int uart_ra6b1_irq_is_pending(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;
	struct uart_ra6b1_data *data = dev->data;

	if (data->irq_event_from_update) {
		data->irq_event_from_update = false;
	} else {
		data->irq_event = cfg->regs->UART_IIR_FCR_REG & 0x0F;
	}

	return (data->irq_event != UART_W_B_INT_NO_INT_PEND);
}

static void uart_ra6b1_irq_update(const struct device *dev)
{
	const struct uart_ra6b1_config *cfg = dev->config;
	struct uart_ra6b1_data *data = dev->data;

	data->irq_event = cfg->regs->UART_IIR_FCR_REG & 0x0F;
	data->irq_event_from_update = true;
}

static void uart_ra6b1_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_ra6b1_data *data = dev->data;

	data->user_cb = cb;
	data->user_cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_ra6b1_err_check(const struct device *dev)
{
	int errors = UART_ERR_NOERROR;
	const struct uart_ra6b1_config *cfg = dev->config;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	struct uart_ra6b1_data *data = dev->data;

	if (data->irq_event != UART_W_B_INT_RECEIVE_LINE_STAT) {
		return 0;
	}
#endif

	const uint32_t status = cfg->regs->UART_LSR_REG;

	if ((status & UART_UART_LSR_REG_UART_OE_Msk) != 0) {
		errors |= UART_ERROR_OVERRUN;
	}
	if ((status & UART_UART_LSR_REG_UART_PE_Msk) != 0) {
		errors |= UART_ERROR_PARITY;
	}
	if ((status & UART_UART_LSR_REG_UART_FE_Msk) != 0) {
		errors |= UART_ERROR_FRAMING;
	}
	if ((status & UART_UART_LSR_REG_UART_BI_Msk) != 0) {
		errors |= UART_BREAK;
	}

	return errors;
}

static DEVICE_API(uart, uart_ra6b1_driver_api) = {
	.poll_in = uart_ra6b1_poll_in,
	.poll_out = uart_ra6b1_poll_out,
	.err_check = uart_ra6b1_err_check,
#if defined(CONFIG_UART_USE_RUNTIME_CONFIGURE)
	.configure = uart_ra6b1_configure,
	.config_get = uart_ra6b1_config_get,
#endif
#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	.fifo_fill = uart_ra6b1_fifo_fill,
	.fifo_read = uart_ra6b1_fifo_read,
	.irq_tx_enable = uart_ra6b1_irq_tx_enable,
	.irq_tx_disable = uart_ra6b1_irq_tx_disable,
	.irq_tx_ready = uart_ra6b1_irq_tx_ready,
	.irq_rx_enable = uart_ra6b1_irq_rx_enable,
	.irq_rx_disable = uart_ra6b1_irq_rx_disable,
	.irq_tx_complete = uart_ra6b1_irq_tx_complete,
	.irq_rx_ready = uart_ra6b1_irq_rx_ready,
	.irq_err_enable = uart_ra6b1_irq_err_enable,
	.irq_err_disable = uart_ra6b1_irq_err_disable,
	.irq_is_pending = uart_ra6b1_irq_is_pending,
	.irq_update = uart_ra6b1_irq_update,
	.irq_callback_set = uart_ra6b1_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_ra6b1_apply_runtime_config(const struct device *dev)
{
	const struct uart_ra6b1_config *config = dev->config;
	struct uart_ra6b1_data *data = dev->data;
	int ret;

	bsp_pd_use(BSP_PD_COM);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply default pins settings");
		return ret;
	}

	ret = fsp_uart->open(&data->uart, &data->fsp_config);
	if ((fsp_err_t)ret != FSP_SUCCESS) {
		LOG_ERR("Failed to open UART instance with status %d", ret);
		return -EIO;
	}

#if !defined(CONFIG_UART_INTERRUPT_DRIVEN)
	/*
	 * Renesas FSP provides only interrupt driven drivers.
	 * Calling fsp_uart open will enable UART interrupts.
	 * If CONFIG_UART_INTERRUPT_DRIVEN is disabled we
	 * have to disable UART interrupts.
	 */
	irq_disable(data->fsp_config_extend.gen_irq);
#endif

	return 0;
}

static int uart_ra6b1_init(const struct device *dev)
{
	struct uart_ra6b1_data *data = dev->data;
	int ret;

	/* Setup fsp uart setting */
	ret = uart_ra6b1_apply_config(&data->uart_config, &data->fsp_config,
					&data->fsp_config_extend, &data->fsp_baud_setting);
	if (ret != 0) {
		return ret;
	}

	data->fsp_config_extend.p_baud_setting = &data->fsp_baud_setting;
	data->fsp_config.p_extend = &data->fsp_config_extend;

	return uart_ra6b1_apply_runtime_config(dev);
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)

static void uart_ra6b1_isr(const struct device *dev)
{
	struct uart_ra6b1_data *data = dev->data;

	R_BSP_IrqStatusClear(R_FSP_CurrentIrqGet());
	if (data->user_cb != NULL) {
		do {
			data->user_cb(dev, data->user_cb_data);
		} while (data->irq_event != UART_W_B_INT_NO_INT_PEND);
	}
}
#endif /* defined(CONFIG_UART_INTERRUPT_DRIVEN) */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#define UART_RA6B1_IRQ_CONFIG_INIT(index)							   \
	do {											   \
		BUILD_ASSERT(DT_IRQ_BY_NAME(DT_DRV_INST(index), gen, irq) < CONFIG_NUM_IRQS,	   \
			     "Please configure a valid ICU event in the interrupts property");	   \
		uint32_t channel = DT_INST_PROP(index, channel);				   \
		volatile uint32_t *ielsr_reg = ICU_IELSRn_REG(					   \
						DT_IRQ_BY_NAME(DT_DRV_INST(index), gen, irq));	   \
												   \
		*ielsr_reg = ICU_EVENT_UARTWB1_IRQ + (channel * 4);				   \
		IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(index), gen, irq),			   \
				DT_IRQ_BY_NAME(DT_DRV_INST(index), gen, priority),		   \
				uart_ra6b1_isr, DEVICE_DT_INST_GET(index), 0);			   \
		irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(index), gen, irq));			   \
	} while (0)
#else
#define UART_RA6B1_IRQ_CONFIG_INIT(index)
#endif

#define UART_RA6B1_INIT(index)									   \
	PINCTRL_DT_INST_DEFINE(index);								   \
												   \
	static const struct uart_ra6b1_config uart_ra6b1_config_##index = {			   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),					   \
		.regs = (UART_Type *)DT_INST_REG_ADDR(index),					   \
	};											   \
												   \
	static struct uart_ra6b1_data uart_ra6b1_data_##index = {				   \
		.uart_config =									   \
			{									   \
				.baudrate = DT_INST_PROP(index, current_speed),			   \
				.parity = DT_INST_ENUM_IDX(index, parity),			   \
				.stop_bits = DT_INST_ENUM_IDX(index, stop_bits),		   \
				.data_bits = DT_INST_ENUM_IDX(index, data_bits),		   \
				.flow_ctrl = COND_CODE_1(DT_NODE_HAS_PROP(index, hw_flow_control), \
							 (UART_CFG_FLOW_CTRL_RTS_CTS),		   \
							 (UART_CFG_FLOW_CTRL_NONE)),		   \
			},									   \
		.fsp_config =									   \
			{									   \
				.channel = DT_INST_PROP(index, channel),			   \
				.tei_irq = DT_IRQ_BY_NAME(DT_DRV_INST(index), tei, irq),	   \
				.tei_ipl = DT_IRQ_BY_NAME(DT_DRV_INST(index), tei, priority),	   \
			},									   \
		.fsp_config_extend =								   \
			{									   \
				.flow_control = COND_CODE_1(					   \
					DT_NODE_HAS_PROP(index, hw_flow_control),		   \
					(UART_CFG_FLOW_CTRL_RTS_CTS),				   \
					(UART_CFG_FLOW_CTRL_NONE)),				   \
				.gen_irq = DT_IRQ_BY_NAME(DT_DRV_INST(index), gen, irq),	   \
				.gen_ipl = DT_IRQ_BY_NAME(DT_DRV_INST(index), gen, priority),	   \
				.rx_fifo_trigger = (UART_W_B_RX_FIFO_2LESS_THAN_FULL_TRIGGER),	   \
			},									   \
		.fsp_baud_setting = {},								   \
		.dev = DEVICE_DT_GET(DT_DRV_INST(index))};					   \
												   \
	static int uart_ra6b1_init_##index(const struct device *dev)				   \
	{											   \
		UART_RA6B1_IRQ_CONFIG_INIT(index);						   \
		uart_ra6b1_data_##index.fsp_config.tei_ipl =					   \
			NVIC_GetPriority(DT_INST_IRQ_BY_NAME(index, tei, irq));			   \
		uart_ra6b1_data_##index.fsp_config_extend.gen_ipl =				   \
			NVIC_GetPriority(DT_INST_IRQ_BY_NAME(index, gen, irq));			   \
		int err = uart_ra6b1_init(dev);							   \
		if (err != 0) {									   \
			return err;								   \
		}										   \
												   \
		return 0;									   \
	}											   \
												   \
	DEVICE_DT_INST_DEFINE(index, uart_ra6b1_init_##index,					   \
				NULL,								   \
				&uart_ra6b1_data_##index,					   \
				&uart_ra6b1_config_##index, PRE_KERNEL_1,			   \
				CONFIG_SERIAL_INIT_PRIORITY, &uart_ra6b1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RA6B1_INIT)
