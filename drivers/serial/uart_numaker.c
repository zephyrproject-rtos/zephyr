/*
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Copyright (c) 2023 Nuvoton Technology Corporation.
 */

#define DT_DRV_COMPAT nuvoton_numaker_uart

#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include <zephyr/drivers/pinctrl.h>
#include <NuMicro.h>

LOG_MODULE_REGISTER(numaker_uart, LOG_LEVEL_ERR);

struct uart_numaker_config {
	UART_T *uart;
	const struct reset_dt_spec reset;
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
	const struct device *clk_dev;
	uint32_t irq_n;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
	const struct pinctrl_dev_config *pincfg;
};

struct uart_numaker_data {
	const struct device *clock;
	struct uart_config ucfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif
};

static int uart_numaker_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_numaker_config *config = dev->config;
	uint32_t count;

	count = UART_Read(config->uart, c, 1);
	if (!count) {
		return -1;
	}

	return 0;
}

static void uart_numaker_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_numaker_config *config = dev->config;

	UART_Write(config->uart, &c, 1);
}

static int uart_numaker_err_check(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;
	uint32_t flags = uart->FIFOSTS;
	int err = 0;

	if (flags & UART_FIFOSTS_RXOVIF_Msk) {
		err |= UART_ERROR_OVERRUN;
	}

	if (flags & UART_FIFOSTS_PEF_Msk) {
		err |= UART_ERROR_PARITY;
	}

	if (flags & UART_FIFOSTS_FEF_Msk) {
		err |= UART_ERROR_FRAMING;
	}

	if (flags & UART_FIFOSTS_BIF_Msk) {
		err |= UART_BREAK;
	}

	if (flags & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk |
		     UART_FIFOSTS_RXOVIF_Msk)) {
		uart->FIFOSTS = (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk |
				 UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk);
	}
	return err;
}

static inline int32_t uart_numaker_convert_stopbit(enum uart_config_stop_bits sb)
{
	switch (sb) {
	case UART_CFG_STOP_BITS_1:
		return UART_STOP_BIT_1;
	case UART_CFG_STOP_BITS_1_5:
		return UART_STOP_BIT_1_5;
	case UART_CFG_STOP_BITS_2:
		return UART_STOP_BIT_2;
	default:
		return -ENOTSUP;
	}
};

static inline int32_t uart_numaker_convert_datalen(enum uart_config_data_bits db)
{
	switch (db) {
	case UART_CFG_DATA_BITS_5:
		return UART_WORD_LEN_5;
	case UART_CFG_DATA_BITS_6:
		return UART_WORD_LEN_6;
	case UART_CFG_DATA_BITS_7:
		return UART_WORD_LEN_7;
	case UART_CFG_DATA_BITS_8:
		return UART_WORD_LEN_8;
	default:
		return -ENOTSUP;
	}
}

static inline uint32_t uart_numaker_convert_parity(enum uart_config_parity parity)
{
	switch (parity) {
	case UART_CFG_PARITY_ODD:
		return UART_PARITY_ODD;
	case UART_CFG_PARITY_EVEN:
		return UART_PARITY_EVEN;
	case UART_CFG_PARITY_MARK:
		return UART_PARITY_MARK;
	case UART_CFG_PARITY_SPACE:
		return UART_PARITY_SPACE;
	case UART_CFG_PARITY_NONE:
	default:
		return UART_PARITY_NONE;
	}
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_numaker_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_numaker_config *config = dev->config;
	struct uart_numaker_data *pData = dev->data;
	int32_t databits, stopbits;
	uint32_t parity;

	databits = uart_numaker_convert_datalen(cfg->data_bits);
	if (databits < 0) {
		return databits;
	}

	stopbits = uart_numaker_convert_stopbit(cfg->stop_bits);
	if (stopbits < 0) {
		return stopbits;
	}

	if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_NONE) {
		UART_DisableFlowCtrl(config->uart);
	} else if (cfg->flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS) {
		UART_EnableFlowCtrl(config->uart);
	} else {
		return -ENOTSUP;
	}

	parity = uart_numaker_convert_parity(cfg->parity);

	UART_SetLineConfig(config->uart, cfg->baudrate, databits, parity, stopbits);

	memcpy(&pData->ucfg, cfg, sizeof(*cfg));

	return 0;
}

static int uart_numaker_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_numaker_data *pData = dev->data;

	memcpy(cfg, &pData->ucfg, sizeof(*cfg));

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_numaker_init(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	struct uart_numaker_data *pData = dev->data;
	int err = 0;

	SYS_UnlockReg();

	struct numaker_scc_subsys scc_subsys;

	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = config->clk_modidx;
	scc_subsys.pcc.clk_src = config->clk_src;
	scc_subsys.pcc.clk_div = config->clk_div;

	/* Equivalent to CLK_EnableModuleClock(clk_modidx) */
	err = clock_control_on(config->clk_dev, (clock_control_subsys_t)&scc_subsys);
	if (err != 0) {
		goto move_exit;
	}
	/* Equivalent to CLK_SetModuleClock(clk_modidx, clk_src, clk_div) */
	err = clock_control_configure(config->clk_dev, (clock_control_subsys_t)&scc_subsys, NULL);
	if (err != 0) {
		goto move_exit;
	}

	/*
	 * Set pinctrl for UART0 RXD and TXD
	 * Set multi-function pins for UART0 RXD and TXD
	 */
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		goto move_exit;
	}

	/* Same as BSP's SYS_ResetModule(id_rst) */
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	/* Reset UART to default state */
	reset_line_toggle_dt(&config->reset);

	UART_Open(config->uart, pData->ucfg.baudrate);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

move_exit:
	SYS_LockReg();
	return err;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_numaker_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;
	int tx_bytes = 0;

	/* Check TX FIFO not full, then fill */
	while (((size - tx_bytes) > 0) && (!(uart->FIFOSTS & UART_FIFOSTS_TXFULL_Msk))) {
		/* Fill one byte into TX FIFO */
		uart->DAT = tx_data[tx_bytes++];
	}

	return tx_bytes;
}

static int uart_numaker_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;
	int rx_bytes = 0;

	/* Check RX FIFO not empty, then read */
	while (((size - rx_bytes) > 0) && (!(uart->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk))) {
		/* Read one byte from UART RX FIFO */
		rx_data[rx_bytes++] = (uint8_t)uart->DAT;
	}

	return rx_bytes;
}

static void uart_numaker_irq_tx_enable(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;

	UART_EnableInt(uart, UART_INTEN_THREIEN_Msk);
}

static void uart_numaker_irq_tx_disable(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;

	UART_DisableInt(uart, UART_INTEN_THREIEN_Msk);
}

static int uart_numaker_irq_tx_ready(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;

	return ((!UART_IS_TX_FULL(uart)) && (uart->INTEN & UART_INTEN_THREIEN_Msk));
}

static int uart_numaker_irq_tx_complete(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;

	return (uart->INTSTS & UART_INTSTS_THREINT_Msk);
}

static void uart_numaker_irq_rx_enable(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;

	UART_EnableInt(uart, UART_INTEN_RDAIEN_Msk);
}

static void uart_numaker_irq_rx_disable(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;

	UART_DisableInt(uart, UART_INTEN_RDAIEN_Msk);
}

static int uart_numaker_irq_rx_ready(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;

	return ((!UART_GET_RX_EMPTY(uart)) && (uart->INTEN & UART_INTEN_RDAIEN_Msk));
}

static void uart_numaker_irq_err_enable(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;

	UART_EnableInt(uart, UART_INTEN_BUFERRIEN_Msk | UART_INTEN_SWBEIEN_Msk);
}

static void uart_numaker_irq_err_disable(const struct device *dev)
{
	const struct uart_numaker_config *config = dev->config;
	UART_T *uart = config->uart;

	UART_DisableInt(uart, UART_INTEN_BUFERRIEN_Msk | UART_INTEN_SWBEIEN_Msk);
}

static int uart_numaker_irq_is_pending(const struct device *dev)
{

	return (uart_numaker_irq_tx_ready(dev) || (uart_numaker_irq_rx_ready(dev)));
}

static int uart_numaker_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* nothing to be done */
	return 1;
}

static void uart_numaker_irq_callback_set(const struct device *dev,
					  uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_numaker_data *pData = dev->data;

	pData->user_cb = cb;
	pData->user_data = cb_data;
}

static void uart_numaker_isr(const struct device *dev)
{
	struct uart_numaker_data *pData = dev->data;

	if (pData->user_cb) {
		pData->user_cb(dev, pData->user_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_numaker_driver_api) = {
	.poll_in = uart_numaker_poll_in,
	.poll_out = uart_numaker_poll_out,
	.err_check = uart_numaker_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_numaker_configure,
	.config_get = uart_numaker_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_numaker_fifo_fill,
	.fifo_read = uart_numaker_fifo_read,
	.irq_tx_enable = uart_numaker_irq_tx_enable,
	.irq_tx_disable = uart_numaker_irq_tx_disable,
	.irq_tx_ready = uart_numaker_irq_tx_ready,
	.irq_tx_complete = uart_numaker_irq_tx_complete,
	.irq_rx_enable = uart_numaker_irq_rx_enable,
	.irq_rx_disable = uart_numaker_irq_rx_disable,
	.irq_rx_ready = uart_numaker_irq_rx_ready,
	.irq_err_enable = uart_numaker_irq_err_enable,
	.irq_err_disable = uart_numaker_irq_err_disable,
	.irq_is_pending = uart_numaker_irq_is_pending,
	.irq_update = uart_numaker_irq_update,
	.irq_callback_set = uart_numaker_irq_callback_set,
#endif
};

#define CLOCK_CTRL_INIT(n) .clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(n))),

#define PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define PINCTRL_INIT(n)	  .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define NUMAKER_UART_IRQ_CONFIG_FUNC(n)                                                            \
	static void uart_numaker_irq_config_##n(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_numaker_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define IRQ_FUNC_INIT(n) .irq_config_func = uart_numaker_irq_config_##n
#else
#define NUMAKER_UART_IRQ_CONFIG_FUNC(n)
#define IRQ_FUNC_INIT(n)
#endif

#define NUMAKER_UART_INIT(inst)                                                                    \
	PINCTRL_DEFINE(inst)                                                                       \
	NUMAKER_UART_IRQ_CONFIG_FUNC(inst)                                                         \
                                                                                                   \
	static const struct uart_numaker_config uart_numaker_cfg_##inst = {                        \
		.uart = (UART_T *)DT_INST_REG_ADDR(inst),                                          \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.clk_modidx = DT_INST_CLOCKS_CELL(inst, clock_module_index),                       \
		.clk_src = DT_INST_CLOCKS_CELL(inst, clock_source),                                \
		.clk_div = DT_INST_CLOCKS_CELL(inst, clock_divider),                               \
		CLOCK_CTRL_INIT(inst).irq_n = DT_INST_IRQN(inst),                                  \
		PINCTRL_INIT(inst) IRQ_FUNC_INIT(inst)};                                           \
                                                                                                   \
	static struct uart_numaker_data uart_numaker_data_##inst = {                               \
		.ucfg =                                                                            \
			{                                                                          \
				.baudrate = DT_INST_PROP(inst, current_speed),                     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, uart_numaker_init, NULL, &uart_numaker_data_##inst,            \
			      &uart_numaker_cfg_##inst, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_numaker_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NUMAKER_UART_INIT)
