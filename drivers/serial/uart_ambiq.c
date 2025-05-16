/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_uart

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>

/* ambiq-sdk includes */
#include <soc.h>

LOG_MODULE_REGISTER(uart_ambiq, CONFIG_UART_LOG_LEVEL);

#define UART_AMBIQ_RSR_ERROR_MASK                                                             \
	(UART0_RSR_FESTAT_Msk | UART0_RSR_PESTAT_Msk | UART0_RSR_BESTAT_Msk | UART0_RSR_OESTAT_Msk)

struct uart_ambiq_config {
	uint32_t base;
	int size;
	int inst_idx;
	uint32_t clk_src;
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

/* Device data structure */
struct uart_ambiq_data {
	am_hal_uart_config_t hal_cfg;
	struct uart_config uart_cfg;
	void *uart_handler;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	volatile bool sw_call_txdrdy;
	uart_irq_callback_user_data_t irq_cb;
	struct k_spinlock irq_cb_lock;
	void *irq_cb_data;
#endif
};

static int uart_ambiq_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_ambiq_config *config = dev->config;
	struct uart_ambiq_data *data = dev->data;

	data->hal_cfg.eTXFifoLevel = AM_HAL_UART_FIFO_LEVEL_16;
	data->hal_cfg.eRXFifoLevel = AM_HAL_UART_FIFO_LEVEL_16;
	data->hal_cfg.ui32BaudRate = cfg->baudrate;

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		data->hal_cfg.eDataBits = AM_HAL_UART_DATA_BITS_5;
		break;
	case UART_CFG_DATA_BITS_6:
		data->hal_cfg.eDataBits = AM_HAL_UART_DATA_BITS_6;
		break;
	case UART_CFG_DATA_BITS_7:
		data->hal_cfg.eDataBits = AM_HAL_UART_DATA_BITS_7;
		break;
	case UART_CFG_DATA_BITS_8:
		data->hal_cfg.eDataBits = AM_HAL_UART_DATA_BITS_8;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		data->hal_cfg.eStopBits = AM_HAL_UART_ONE_STOP_BIT;
		break;
	case UART_CFG_STOP_BITS_2:
		data->hal_cfg.eStopBits = AM_HAL_UART_TWO_STOP_BITS;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		data->hal_cfg.eFlowControl = AM_HAL_UART_FLOW_CTRL_NONE;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		data->hal_cfg.eFlowControl = AM_HAL_UART_FLOW_CTRL_RTS_CTS;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		data->hal_cfg.eParity = AM_HAL_UART_PARITY_NONE;
		break;
	case UART_CFG_PARITY_EVEN:
		data->hal_cfg.eParity = AM_HAL_UART_PARITY_EVEN;
		break;
	case UART_CFG_PARITY_ODD:
		data->hal_cfg.eParity = AM_HAL_UART_PARITY_ODD;
		break;
	default:
		return -ENOTSUP;
	}

	switch (config->clk_src) {
	case 0:
		data->hal_cfg.eClockSrc = AM_HAL_UART_CLOCK_SRC_HFRC;
		break;
	case 1:
		data->hal_cfg.eClockSrc = AM_HAL_UART_CLOCK_SRC_SYSPLL;
		break;
	default:
		return -EINVAL;
	}

	if (am_hal_uart_configure(data->uart_handler, &data->hal_cfg) != AM_HAL_STATUS_SUCCESS) {
		return -EINVAL;
	}

	data->uart_cfg = *cfg;

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_ambiq_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_ambiq_data *data = dev->data;

	*cfg = data->uart_cfg;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static bool uart_ambiq_is_readable(const struct device *dev)
{
	const struct uart_ambiq_config *config = dev->config;
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;

	if (!(UARTn(config->inst_idx)->CR & UART0_CR_UARTEN_Msk) ||
	    !(UARTn(config->inst_idx)->CR & UART0_CR_RXE_Msk)) {
		return false;
	}
	am_hal_uart_flags_get(data->uart_handler, &flag);

	return (flag & UART0_FR_RXFE_Msk) == 0U;
}

static int uart_ambiq_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;

	if (!uart_ambiq_is_readable(dev)) {
		return -1;
	}

	/* got a character */
	am_hal_uart_fifo_read(data->uart_handler, c, 1, NULL);
	am_hal_uart_flags_get(data->uart_handler, &flag);
	return flag & UART_AMBIQ_RSR_ERROR_MASK;
}

static void uart_ambiq_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;

	/* Wait for space in FIFO */
	do {
		am_hal_uart_flags_get(data->uart_handler, &flag);
	} while (flag & UART0_FR_TXFF_Msk);

	/* Send a character */
	am_hal_uart_fifo_write(data->uart_handler, &c, 1, NULL);
}

static int uart_ambiq_err_check(const struct device *dev)
{
	const struct uart_ambiq_config *cfg = dev->config;
	int errors = 0;

	if (UARTn(cfg->inst_idx)->RSR & AM_HAL_UART_RSR_OESTAT) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (UARTn(cfg->inst_idx)->RSR & AM_HAL_UART_RSR_BESTAT) {
		errors |= UART_BREAK;
	}

	if (UARTn(cfg->inst_idx)->RSR & AM_HAL_UART_RSR_PESTAT) {
		errors |= UART_ERROR_PARITY;
	}

	if (UARTn(cfg->inst_idx)->RSR & AM_HAL_UART_RSR_FESTAT) {
		errors |= UART_ERROR_FRAMING;
	}

	return errors;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_ambiq_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_ambiq_data *data = dev->data;
	int num_tx = 0U;

	am_hal_uart_fifo_write(data->uart_handler, (uint8_t *)tx_data, len, &num_tx);

	return num_tx;
}

static int uart_ambiq_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	struct uart_ambiq_data *data = dev->data;
	int num_rx = 0U;

	am_hal_uart_fifo_read(data->uart_handler, rx_data, len, &num_rx);

	return num_rx;
}

static void uart_ambiq_irq_tx_enable(const struct device *dev)
{
	const struct uart_ambiq_config *cfg = dev->config;
	struct uart_ambiq_data *data = dev->data;

	am_hal_uart_interrupt_enable(data->uart_handler, AM_HAL_UART_INT_TX);

	if (!data->sw_call_txdrdy) {
		return;
	}
	data->sw_call_txdrdy = false;

	/*
	 * Verify if the callback has been registered. Due to HW limitation, the
	 * first TX interrupt should be triggered by the software.
	 *
	 * PL011 TX interrupt is based on a transition through a level, rather
	 * than on the level itself[1]. So that, enable TX interrupt can not
	 * trigger TX interrupt if no data was filled to TX FIFO at the
	 * beginning.
	 *
	 * [1]: PrimeCell UART (PL011) Technical Reference Manual
	 *      functional-overview/interrupts
	 */
	if (!data->irq_cb) {
		return;
	}

	/*
	 * Execute callback while TX interrupt remains enabled. If
	 * uart_fifo_fill() is called with small amounts of data, the 1/8 TX
	 * FIFO threshold may never be reached, and the hardware TX interrupt
	 * will never trigger.
	 */
	while (UARTn(cfg->inst_idx)->IER & AM_HAL_UART_INT_TX) {
		K_SPINLOCK(&data->irq_cb_lock) {
			data->irq_cb(dev, data->irq_cb_data);
		}
	}
}

static void uart_ambiq_irq_tx_disable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;

	data->sw_call_txdrdy = true;
	am_hal_uart_interrupt_disable(data->uart_handler, AM_HAL_UART_INT_TX);
}

static int uart_ambiq_irq_tx_complete(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;
	/* Check for UART is busy transmitting data. */
	am_hal_uart_flags_get(data->uart_handler, &flag);
	return ((flag & AM_HAL_UART_FR_BUSY) == 0);
}

static int uart_ambiq_irq_tx_ready(const struct device *dev)
{
	const struct uart_ambiq_config *cfg = dev->config;
	struct uart_ambiq_data *data = dev->data;
	uint32_t status, flag = 0;

	if (!(UARTn(cfg->inst_idx)->CR & UART0_CR_TXE_Msk)) {
		return false;
	}

	/* Check for TX interrupt status is set or TX FIFO is empty. */
	am_hal_uart_interrupt_status_get(data->uart_handler, &status, true);
	am_hal_uart_flags_get(data->uart_handler, &flag);
	return ((status & UART0_IES_TXRIS_Msk) || (flag & AM_HAL_UART_FR_TX_EMPTY));
}

static void uart_ambiq_irq_rx_enable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;

	am_hal_uart_interrupt_enable(data->uart_handler,
				     (AM_HAL_UART_INT_RX | AM_HAL_UART_INT_RX_TMOUT));
}

static void uart_ambiq_irq_rx_disable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;

	am_hal_uart_interrupt_disable(data->uart_handler,
				      (AM_HAL_UART_INT_RX | AM_HAL_UART_INT_RX_TMOUT));
}

static int uart_ambiq_irq_rx_ready(const struct device *dev)
{
	const struct uart_ambiq_config *cfg = dev->config;
	struct uart_ambiq_data *data = dev->data;
	uint32_t flag = 0;
	uint32_t ier = 0;

	if (!(UARTn(cfg->inst_idx)->CR & UART0_CR_RXE_Msk)) {
		return false;
	}

	am_hal_uart_flags_get(data->uart_handler, &flag);
	am_hal_uart_interrupt_enable_get(data->uart_handler, &ier);
	return ((ier & AM_HAL_UART_INT_RX) && (!(flag & AM_HAL_UART_FR_RX_EMPTY)));
}

static void uart_ambiq_irq_err_enable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;
	/* enable framing, parity, break, and overrun */
	am_hal_uart_interrupt_enable(data->uart_handler,
				     (AM_HAL_UART_INT_FRAME_ERR | AM_HAL_UART_INT_PARITY_ERR |
				      AM_HAL_UART_INT_BREAK_ERR | AM_HAL_UART_INT_OVER_RUN));
}

static void uart_ambiq_irq_err_disable(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;

	am_hal_uart_interrupt_disable(data->uart_handler,
				      (AM_HAL_UART_INT_FRAME_ERR | AM_HAL_UART_INT_PARITY_ERR |
				       AM_HAL_UART_INT_BREAK_ERR | AM_HAL_UART_INT_OVER_RUN));
}

static int uart_ambiq_irq_is_pending(const struct device *dev)
{
	return uart_ambiq_irq_rx_ready(dev) || uart_ambiq_irq_tx_ready(dev);
}

static int uart_ambiq_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_ambiq_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_ambiq_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_ambiq_driver_api) = {
	.poll_in = uart_ambiq_poll_in,
	.poll_out = uart_ambiq_poll_out,
	.err_check = uart_ambiq_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_ambiq_configure,
	.config_get = uart_ambiq_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_ambiq_fifo_fill,
	.fifo_read = uart_ambiq_fifo_read,
	.irq_tx_enable = uart_ambiq_irq_tx_enable,
	.irq_tx_disable = uart_ambiq_irq_tx_disable,
	.irq_tx_ready = uart_ambiq_irq_tx_ready,
	.irq_rx_enable = uart_ambiq_irq_rx_enable,
	.irq_rx_disable = uart_ambiq_irq_rx_disable,
	.irq_tx_complete = uart_ambiq_irq_tx_complete,
	.irq_rx_ready = uart_ambiq_irq_rx_ready,
	.irq_err_enable = uart_ambiq_irq_err_enable,
	.irq_err_disable = uart_ambiq_irq_err_disable,
	.irq_is_pending = uart_ambiq_irq_is_pending,
	.irq_update = uart_ambiq_irq_update,
	.irq_callback_set = uart_ambiq_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int uart_ambiq_init(const struct device *dev)
{
	const struct uart_ambiq_config *config = dev->config;
	struct uart_ambiq_data *data = dev->data;
	int ret = 0;

	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_uart_initialize(config->inst_idx, &data->uart_handler)) {
		LOG_ERR("Fail to initialize UART\n");
		return -ENXIO;
	}

	ret = am_hal_uart_power_control(data->uart_handler, AM_HAL_SYSCTRL_WAKE, false);

	ret |= uart_ambiq_configure(dev, &data->uart_cfg);
	if (ret < 0) {
		LOG_ERR("Fail to config UART\n");
		goto end;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Fail to config UART pins\n");
		goto end;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
	data->sw_call_txdrdy = true;
#endif

end:
	if (ret < 0) {
		am_hal_uart_deinitialize(data->uart_handler);
	}
	return ret;
}

#ifdef CONFIG_PM_DEVICE
static int uart_ambiq_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct uart_ambiq_data *data = dev->data;
	uint32_t ret;
	am_hal_sysctrl_power_state_e status;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		status = AM_HAL_SYSCTRL_WAKE;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		status = AM_HAL_SYSCTRL_DEEPSLEEP;
		break;
	default:
		return -ENOTSUP;
	}

	ret = am_hal_uart_power_control(data->uart_handler, status, true);

	if (ret != AM_HAL_STATUS_SUCCESS) {
		return -EPERM;
	} else {
		return 0;
	}
}
#endif /*CONFIG_PM_DEVICE*/

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
void uart_ambiq_isr(const struct device *dev)
{
	struct uart_ambiq_data *data = dev->data;

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		K_SPINLOCK(&data->irq_cb_lock) {
			data->irq_cb(dev, data->irq_cb_data);
		}
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_AMBIQ_DECLARE_CFG(n, IRQ_FUNC_INIT)                                     \
	static const struct uart_ambiq_config uart_ambiq_cfg_##n = {                     \
		.base = DT_INST_REG_ADDR(n),                                                 \
		.size = DT_INST_REG_SIZE(n),                                                 \
		.inst_idx = (DT_INST_REG_ADDR(n) - UART0_BASE) / (UART1_BASE - UART0_BASE),  \
		.clk_src = DT_INST_PROP(n, clk_src),                                         \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                 \
		IRQ_FUNC_INIT}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_AMBIQ_CONFIG_FUNC(n)                                                    \
	static void uart_ambiq_irq_config_func_##n(const struct device *dev)             \
	{                                                                                \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), uart_ambiq_isr,       \
			    DEVICE_DT_INST_GET(n), 0);                                           \
		irq_enable(DT_INST_IRQN(n));                                                 \
	}
#define UART_AMBIQ_IRQ_CFG_FUNC_INIT(n) .irq_config_func = uart_ambiq_irq_config_func_##n
#define UART_AMBIQ_INIT_CFG(n)    UART_AMBIQ_DECLARE_CFG(n, UART_AMBIQ_IRQ_CFG_FUNC_INIT(n))
#else
#define UART_AMBIQ_CONFIG_FUNC(n)
#define UART_AMBIQ_IRQ_CFG_FUNC_INIT
#define UART_AMBIQ_INIT_CFG(n) UART_AMBIQ_DECLARE_CFG(n, UART_AMBIQ_IRQ_CFG_FUNC_INIT)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define UART_AMBIQ_INIT(n)                                         \
	PINCTRL_DT_INST_DEFINE(n);                                     \
	static struct uart_ambiq_data uart_ambiq_data_##n = {          \
		.uart_cfg =                                                \
			{                                                      \
				.baudrate = DT_INST_PROP(n, current_speed),        \
				.parity = UART_CFG_PARITY_NONE,                    \
				.stop_bits = UART_CFG_STOP_BITS_1,                 \
				.data_bits = UART_CFG_DATA_BITS_8,                 \
				.flow_ctrl = DT_INST_PROP(n, hw_flow_control)      \
						     ? UART_CFG_FLOW_CTRL_RTS_CTS          \
						     : UART_CFG_FLOW_CTRL_NONE,            \
			},                                                     \
	};                                                             \
	static const struct uart_ambiq_config uart_ambiq_cfg_##n;      \
	PM_DEVICE_DT_INST_DEFINE(n, uart_ambiq_pm_action);             \
	DEVICE_DT_INST_DEFINE(n, uart_ambiq_init, PM_DEVICE_DT_INST_GET(n), &uart_ambiq_data_##n, \
			      &uart_ambiq_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_ambiq_driver_api);                         \
	UART_AMBIQ_CONFIG_FUNC(n)                                      \
	UART_AMBIQ_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(UART_AMBIQ_INIT)
