/* Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc84x_uart

#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <fsl_usart.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_lpc84x, CONFIG_UART_LOG_LEVEL);

struct lpc84x_uart_config {
	DEVICE_MMIO_ROM;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	uint32_t baudrate;
	void (*irq_config_func)(const struct device *dev);
};

struct lpc84x_uart_data {
	DEVICE_MMIO_RAM;
	usart_config_t usart_cfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t callback;
	void *callback_data;
	uint32_t irq_status;
#endif
};

/* Defined macro to get the Register Base Address */
#define DEV_BASE(dev) ((USART_Type *)DEVICE_MMIO_GET(dev))

static int lpc84x_uart_poll_in(const struct device *dev, unsigned char *c)
{
	/* Return -1 if no data is available */
	if ((USART_GetStatusFlags(DEV_BASE(dev)) & kUSART_RxReady) == 0) {
		return -1;
	}

	*c = USART_ReadByte(DEV_BASE(dev));
	return 0;
}

static void lpc84x_uart_poll_out(const struct device *dev, unsigned char c)
{
	while (!(USART_GetStatusFlags(DEV_BASE(dev)) & kUSART_TxReady)) {
	}

	USART_WriteByte(DEV_BASE(dev), c);
}

static int lpc84x_uart_err_check(const struct device *dev)
{
	uint32_t flags = USART_GetStatusFlags(DEV_BASE(dev));
	int error_status = 0;

	if (flags & kUSART_HardwareOverrunFlag) {
		error_status |= UART_ERROR_OVERRUN;
	}

	if (flags & kUSART_ParityErrorFlag) {
		error_status |= UART_ERROR_PARITY;
	}

	if (flags & kUSART_FramErrorFlag) {
		error_status |= UART_ERROR_FRAMING;
	}

	if (flags & kUSART_RxNoiseFlag) {
		error_status |= UART_ERROR_NOISE;
	}

	/* Clear error flags by writing to STAT register */
	USART_ClearStatusFlags(DEV_BASE(dev), kUSART_HardwareOverrunFlag | kUSART_ParityErrorFlag |
						      kUSART_FramErrorFlag | kUSART_RxNoiseFlag);

	return error_status;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void lpc84x_uart_irq_tx_enable(const struct device *dev)
{
	USART_EnableInterrupts(DEV_BASE(dev), kUSART_TxReadyInterruptEnable);
}

static void lpc84x_uart_irq_tx_disable(const struct device *dev)
{
	USART_DisableInterrupts(DEV_BASE(dev), kUSART_TxReadyInterruptEnable);
}

static void lpc84x_uart_irq_rx_enable(const struct device *dev)
{
	USART_EnableInterrupts(DEV_BASE(dev), kUSART_RxReadyInterruptEnable);
}

static void lpc84x_uart_irq_rx_disable(const struct device *dev)
{
	USART_DisableInterrupts(DEV_BASE(dev), kUSART_RxReadyInterruptEnable);
}

static int lpc84x_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	int tx_size = 0;

	while (tx_size < size && (USART_GetStatusFlags(DEV_BASE(dev)) & kUSART_TxReady)) {
		USART_WriteByte(DEV_BASE(dev), tx_data[tx_size++]);
	}

	return tx_size;
}

static int lpc84x_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	int rx_size = 0;

	while (rx_size < size && (USART_GetStatusFlags(DEV_BASE(dev)) & kUSART_RxReady)) {
		rx_data[rx_size++] = USART_ReadByte(DEV_BASE(dev));
	}

	return rx_size;
}

static int lpc84x_uart_irq_tx_ready(const struct device *dev)
{
	struct lpc84x_uart_data *data = dev->data;

	return (data->irq_status & kUSART_TxReady) &&
	       (USART_GetEnabledInterrupts(DEV_BASE(dev)) & kUSART_TxReadyInterruptEnable);
}

static int lpc84x_uart_irq_rx_ready(const struct device *dev)
{
	struct lpc84x_uart_data *data = dev->data;

	return (data->irq_status & kUSART_RxReady) &&
	       (USART_GetEnabledInterrupts(DEV_BASE(dev)) & kUSART_RxReadyInterruptEnable);
}

static int lpc84x_uart_irq_err_ready(const struct device *dev)
{
	struct lpc84x_uart_data *data = dev->data;
	uint32_t mask = kUSART_HardwareOverRunInterruptEnable | kUSART_FramErrorInterruptEnable |
			kUSART_ParityErrorInterruptEnable | kUSART_RxNoiseInterruptEnable;

	return (data->irq_status & (kUSART_HardwareOverrunFlag | kUSART_FramErrorFlag |
				    kUSART_ParityErrorFlag | kUSART_RxNoiseFlag)) &&
	       (USART_GetEnabledInterrupts(DEV_BASE(dev)) & mask);
}

static void lpc84x_uart_irq_err_enable(const struct device *dev)
{
	uint32_t mask = kUSART_HardwareOverRunInterruptEnable | kUSART_FramErrorInterruptEnable |
			kUSART_ParityErrorInterruptEnable | kUSART_RxNoiseInterruptEnable;

	USART_EnableInterrupts(DEV_BASE(dev), mask);
}

static void lpc84x_uart_irq_err_disable(const struct device *dev)
{
	uint32_t mask = kUSART_HardwareOverRunInterruptEnable | kUSART_FramErrorInterruptEnable |
			kUSART_ParityErrorInterruptEnable | kUSART_RxNoiseInterruptEnable;

	USART_DisableInterrupts(DEV_BASE(dev), mask);
}

static int lpc84x_uart_irq_is_pending(const struct device *dev)
{
	return lpc84x_uart_irq_tx_ready(dev) || lpc84x_uart_irq_rx_ready(dev) ||
	       lpc84x_uart_irq_err_ready(dev);
}

static int lpc84x_uart_irq_update(const struct device *dev)
{
	struct lpc84x_uart_data *data = dev->data;

	data->irq_status = USART_GetStatusFlags(DEV_BASE(dev));

	return 1;
}

static void lpc84x_uart_irq_callback_set(const struct device *dev,
					 uart_irq_callback_user_data_t callback,
					 void *callback_data)
{
	struct lpc84x_uart_data *data = dev->data;

	data->callback = callback;
	data->callback_data = callback_data;
}

static void lpc84x_uart_isr(const struct device *dev)
{
	struct lpc84x_uart_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->callback_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int lpc84x_uart_apply_config(const struct device *dev, const usart_config_t *usart_cfg,
				    uint32_t clock_freq)
{
	USART_Type *base = DEV_BASE(dev);

	/* Disable USART */
	base->CFG &= ~USART_CFG_ENABLE_MASK;

	/* Set configuration */
	base->CFG = (usart_cfg->syncMode != kUSART_SyncModeDisabled ? USART_CFG_SYNCEN_MASK : 0) |
		    USART_CFG_PARITYSEL(usart_cfg->parityMode) |
		    USART_CFG_STOPLEN(usart_cfg->stopBitCount) |
		    USART_CFG_SYNCEN((uint32_t)usart_cfg->syncMode >> 1) |
		    USART_CFG_DATALEN((uint8_t)usart_cfg->bitCountPerChar) |
		    USART_CFG_LOOP(usart_cfg->loopback ? 1U : 0U) |
		    USART_CFG_SYNCMST(usart_cfg->syncMode) |
		    USART_CFG_CLKPOL(usart_cfg->clockPolarity) |
		    USART_CFG_CTSEN(usart_cfg->enableHardwareFlowControl ? 1U : 0U);

	/* Set baudrate before enabling */
	if (USART_SetBaudRate(base, usart_cfg->baudRate_Bps, clock_freq) != kStatus_Success) {
		return -EIO;
	}

	/* Enable USART */
	base->CFG |= USART_CFG_ENABLE_MASK;

	/* Enable TX/RX */
	USART_EnableTx(base, usart_cfg->enableTx);
	USART_EnableRx(base, usart_cfg->enableRx);

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static void lpc84x_uart_wait_for_idle(const struct device *dev)
{
	uint32_t flags;

	while (!(USART_GetStatusFlags(DEV_BASE(dev)) & kUSART_TxIdleFlag)) {
		/* Wait for TX to be fully idle */
	}

	/* Flush RX FIFO (if any data pending) */
	flags = USART_GetStatusFlags(DEV_BASE(dev));
	while (flags & kUSART_RxReady) {
		(void)USART_ReadByte(DEV_BASE(dev));
		flags = USART_GetStatusFlags(DEV_BASE(dev));
	}
}

static int lpc84x_uart_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct lpc84x_uart_config *config = dev->config;
	struct lpc84x_uart_data *data = dev->data;
	uint32_t clock_freq;
	usart_config_t usart_cfg;

	/* Save current config */
	memcpy(&usart_cfg, &data->usart_cfg, sizeof(usart_cfg));

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		usart_cfg.parityMode = kUSART_ParityDisabled;
		break;
	case UART_CFG_PARITY_EVEN:
		usart_cfg.parityMode = kUSART_ParityEven;
		break;
	case UART_CFG_PARITY_ODD:
		usart_cfg.parityMode = kUSART_ParityOdd;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		usart_cfg.stopBitCount = kUSART_OneStopBit;
		break;
	case UART_CFG_STOP_BITS_2:
		usart_cfg.stopBitCount = kUSART_TwoStopBit;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_7:
		usart_cfg.bitCountPerChar = kUSART_7BitsPerChar;
		break;
	case UART_CFG_DATA_BITS_8:
		usart_cfg.bitCountPerChar = kUSART_8BitsPerChar;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		usart_cfg.enableHardwareFlowControl = false;
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		usart_cfg.enableHardwareFlowControl = true;
		break;
	default:
		return -ENOTSUP;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		return -EIO;
	}

	usart_cfg.baudRate_Bps = cfg->baudrate;

	lpc84x_uart_wait_for_idle(dev);

	if (lpc84x_uart_apply_config(dev, &usart_cfg, clock_freq) != 0) {
		/* Restore hardware configuration to previous state */
		lpc84x_uart_apply_config(dev, &data->usart_cfg, clock_freq);
		return -EIO;
	}

	/* Store the new configuration only if hardware update succeeded */
	memcpy(&data->usart_cfg, &usart_cfg, sizeof(usart_cfg));

	return 0;
}

static int lpc84x_uart_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct lpc84x_uart_data *data = dev->data;

	memset(cfg, 0, sizeof(struct uart_config));

	cfg->baudrate = data->usart_cfg.baudRate_Bps;

	switch (data->usart_cfg.parityMode) {
	case kUSART_ParityDisabled:
		cfg->parity = UART_CFG_PARITY_NONE;
		break;
	case kUSART_ParityEven:
		cfg->parity = UART_CFG_PARITY_EVEN;
		break;
	case kUSART_ParityOdd:
		cfg->parity = UART_CFG_PARITY_ODD;
		break;
	default:
		return -EINVAL;
	}

	switch (data->usart_cfg.stopBitCount) {
	case kUSART_OneStopBit:
		cfg->stop_bits = UART_CFG_STOP_BITS_1;
		break;
	case kUSART_TwoStopBit:
		cfg->stop_bits = UART_CFG_STOP_BITS_2;
		break;
	default:
		return -EINVAL;
	}

	switch (data->usart_cfg.bitCountPerChar) {
	case kUSART_7BitsPerChar:
		cfg->data_bits = UART_CFG_DATA_BITS_7;
		break;
	case kUSART_8BitsPerChar:
		cfg->data_bits = UART_CFG_DATA_BITS_8;
		break;
	default:
		return -EINVAL;
	}

	if (data->usart_cfg.enableHardwareFlowControl) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	} else {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
	}

	return 0;
}

#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int lpc84x_uart_init(const struct device *dev)
{
	const struct lpc84x_uart_config *config = dev->config;
	struct lpc84x_uart_data *data = dev->data;
	uint32_t clock_freq = 0;
	uint32_t instance = 0;
	int err;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	/* Apply default pin control state to select RX and TX pins */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_ERR("failed to enable clock (err %d)", err);
		return err;
	}

	instance = USART_GetInstance(DEV_BASE(dev));

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq);

	if (err) {
		LOG_ERR("failed to get clock rate (err %d)", err);
		return err;
	}

	USART_GetDefaultConfig(&data->usart_cfg);
	data->usart_cfg.baudRate_Bps = config->baudrate;
	data->usart_cfg.enableTx = true;
	data->usart_cfg.enableRx = true;

	/* Manual initialization
	 * USART_Init enables continuous SCLK for synchronous mode,
	 * but UART doesn't need synchronization.
	 */
	err = lpc84x_uart_apply_config(dev, &data->usart_cfg, clock_freq);
	if (err) {
		LOG_ERR("failed to set baud rate for USART%d (base %p, rate %u, clock %u)",
			instance, DEV_BASE(dev), config->baudrate, clock_freq);
		return err;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	LOG_DBG("USART%d initialized: %u baud", instance, config->baudrate);

	return 0;
}

static DEVICE_API(uart, lpc84x_uart_driver_api) = {
	.poll_in = lpc84x_uart_poll_in,
	.poll_out = lpc84x_uart_poll_out,
	.err_check = lpc84x_uart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = lpc84x_uart_configure,
	.config_get = lpc84x_uart_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = lpc84x_uart_fifo_fill,
	.fifo_read = lpc84x_uart_fifo_read,
	.irq_tx_enable = lpc84x_uart_irq_tx_enable,
	.irq_tx_disable = lpc84x_uart_irq_tx_disable,
	.irq_tx_ready = lpc84x_uart_irq_tx_ready,
	.irq_rx_enable = lpc84x_uart_irq_rx_enable,
	.irq_rx_disable = lpc84x_uart_irq_rx_disable,
	.irq_rx_ready = lpc84x_uart_irq_rx_ready,
	.irq_err_enable = lpc84x_uart_irq_err_enable,
	.irq_err_disable = lpc84x_uart_irq_err_disable,
	.irq_is_pending = lpc84x_uart_irq_is_pending,
	.irq_update = lpc84x_uart_irq_update,
	.irq_callback_set = lpc84x_uart_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define LPC84X_UART_CONFIG_FUNC(n)                                                                 \
	static void lpc84x_uart_config_func_##n(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), lpc84x_uart_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
                                                                                                   \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define LPC84X_UART_IRQ_CFG_FUNC_SET(n) .irq_config_func = lpc84x_uart_config_func_##n,
#else
#define LPC84X_UART_CONFIG_FUNC(n)
#define LPC84X_UART_IRQ_CFG_FUNC_SET(n)
#endif

#define LPC84X_UART_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	LPC84X_UART_CONFIG_FUNC(n)                                                                 \
                                                                                                   \
	static const struct lpc84x_uart_config lpc84x_uart_##n##_config = {                        \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.baudrate = DT_INST_PROP(n, current_speed),                                        \
		LPC84X_UART_IRQ_CFG_FUNC_SET(n)};                                                  \
                                                                                                   \
	static struct lpc84x_uart_data lpc84x_uart_##n##_data;                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, lpc84x_uart_init, NULL, &lpc84x_uart_##n##_data,                  \
			      &lpc84x_uart_##n##_config, PRE_KERNEL_1,                             \
			      CONFIG_SERIAL_INIT_PRIORITY, &lpc84x_uart_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LPC84X_UART_INIT)
