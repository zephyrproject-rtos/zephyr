/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_uart

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>
#include <soc.h>

#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma.h>
#endif

/* Driverlib includes */
#include <ti/driverlib/dl_uart_main.h>

struct uart_mspm0_async_tx_data {
	const uint8_t *buf;
	size_t buf_len;
	struct k_work_delayable timeout_work;
};

struct uart_mspm0_async_rx_data {
	uint8_t *buf;
	size_t buf_len;
	size_t offset;
	uint8_t *next_buf;
	size_t next_buf_len;
	int32_t timeout;
	struct k_work_delayable timeout_work;
};

struct uart_mspm0_config {
	UART_Regs *regs;
	uint32_t clock_frequency;
	uint32_t current_speed;
	const struct mspm0_clockSys *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN ) || defined (CONFIG_UART_ASYNC_API)
	uart_irq_config_func_t irq_config_func;
#endif
#ifdef CONFIG_UART_ASYNC_API
	const struct device *dma_dev;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_channel;
	uint8_t rx_trigger;
	uint8_t tx_trigger;
#endif
};

struct uart_mspm0_data {
	/* UART clock structure */
	DL_UART_Main_ClockConfig UART_ClockConfig;
	/* UART config structure */
	DL_UART_Main_Config UART_Config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uint32_t interruptState;          /* Masked Interrupt Status when called by irq_update */
	uart_irq_callback_user_data_t cb; /* Callback function pointer */
	void *cb_data;                    /* Callback function arg */
#endif                                    /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	const struct device *dma_dev;
	const struct device *uart_dev;
	struct uart_mspm0_async_tx_data async_tx;
	struct uart_mspm0_async_rx_data async_rx;
	uart_callback_t async_cb;
	void *async_user_data;
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define MSPM0_INTERRUPT_CALLBACK_FN(index) .cb = NULL,
#else
#define MSPM0_INTERRUPT_CALLBACK_FN(index)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_mspm0_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_mspm0_config *config = dev->config;

	return (DL_UART_Main_receiveDataCheck(config->regs, c)) ? 0 : -1;
}

static void uart_mspm0_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_transmitDataBlocking(config->regs, c);
}

#define UART_MSPM0_TX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_TX | DL_UART_MAIN_INTERRUPT_EOT_DONE)
#define UART_MSPM0_RX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_RX)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_mspm0_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_mspm0_config *config = dev->config;

	return (int)DL_UART_Main_fillTXFIFO(config->regs, (uint8_t *)tx_data, size);
}

static int uart_mspm0_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_mspm0_config *config = dev->config;

	return (int)DL_UART_Main_drainRXFIFO(config->regs, rx_data, size);
}

static void uart_mspm0_irq_rx_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_enableInterrupt(config->regs, UART_MSPM0_RX_INTERRUPTS);
}

static void uart_mspm0_irq_rx_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_RX_INTERRUPTS);
}

static int uart_mspm0_irq_rx_ready(const struct device *dev)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	return ((dev_data->interruptState & DL_UART_MAIN_INTERRUPT_RX) == DL_UART_MAIN_INTERRUPT_RX)
		       ? 1
		       : 0;
}

static void uart_mspm0_irq_tx_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_enableInterrupt(config->regs, UART_MSPM0_TX_INTERRUPTS);
}

static void uart_mspm0_irq_tx_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_TX_INTERRUPTS);
}

static int uart_mspm0_irq_tx_ready(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *const dev_data = dev->data;

	return (((dev_data->interruptState & DL_UART_MAIN_INTERRUPT_TX) ==
		 DL_UART_MAIN_INTERRUPT_TX) ||
		DL_UART_Main_isTXFIFOEmpty(config->regs))
		       ? 1
		       : 0;
}

static int uart_mspm0_irq_tx_complete(const struct device *dev)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	return ((dev_data->interruptState & DL_UART_MAIN_INTERRUPT_EOT_DONE) ==
		DL_UART_MAIN_INTERRUPT_EOT_DONE)
		       ? 1
		       : 0;
}

static int uart_mspm0_irq_is_pending(const struct device *dev)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	return ((dev_data->interruptState != 0) ? 1 : 0);
}

static int uart_mspm0_irq_update(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *const dev_data = dev->data;

	dev_data->interruptState = DL_UART_Main_getEnabledInterruptStatus(
		config->regs, UART_MSPM0_RX_INTERRUPTS | UART_MSPM0_TX_INTERRUPTS);

	/*
	 * Clear interrupts explicitly after storing all in the update. Interrupts
	 * can be re-set by the MIS during the ISR should they be available.
	 */

	DL_UART_Main_clearInterruptStatus(config->regs, dev_data->interruptState);

	return 1;
}

static void uart_mspm0_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	/* Set callback function and data */
	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static inline void uart_mspm0_async_timer_start(struct k_work_delayable *work, size_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && timeout != 0) {
		k_work_reschedule(work, K_USEC(timeout));
	}
}

static void uart_mspm0_async_rx_callback(const struct device *dev, void *user_data,
					 uint32_t channel, int status)
{
	int ret;
	struct uart_event evt = {0};
	const struct device *uart_dev = user_data;
	const struct uart_mspm0_config *cfg = uart_dev->config;
	struct uart_mspm0_data *data = uart_dev->data;
	struct dma_status stat = {0};
	unsigned int key = irq_lock();

	if (channel != cfg->rx_dma_channel) {
		irq_unlock(key);
		return;
	}

	k_work_cancel_delayable(&data->async_rx.timeout_work);

	if (data->async_rx.next_buf) {
		ret = dma_reload(cfg->dma_dev, cfg->rx_dma_channel, 0, (uint32_t)data->async_rx.next_buf,
				 data->async_rx.next_buf_len);
		if (ret == 0) {
			dma_start(cfg->dma_dev, cfg->rx_dma_channel);
		} else {
			dma_stop(cfg->dma_dev, cfg->rx_dma_channel);
			evt.type = UART_RX_STOPPED;
			evt.data.rx_stop.reason = ret;
			data->async_cb(uart_dev, &evt, data->async_user_data);
		}
	}

	evt.type = UART_RX_RDY;
	evt.data.rx.buf = data->async_rx.buf;
	evt.data.rx.offset = data->async_rx.offset;
	evt.data.rx.len = data->async_rx.buf_len;

	if (data->async_cb) {
		data->async_cb(uart_dev, &evt, data->async_user_data);

		data->async_rx.offset = 0;

		evt.type = UART_RX_BUF_RELEASED;
		data->async_cb(uart_dev, &evt, data->async_user_data);
		data->async_rx.buf = data->async_rx.next_buf;
		data->async_rx.buf_len = data->async_rx.next_buf_len;
		data->async_rx.next_buf = NULL;
		data->async_rx.next_buf_len = 0;
		evt.type = UART_RX_BUF_REQUEST;
		data->async_cb(uart_dev, &evt, data->async_user_data);
	}

	uart_mspm0_async_timer_start(&data->async_rx.timeout_work,
				     data->async_rx.timeout);

	irq_unlock(key);
}

static void uart_mspm0_async_tx_callback(const struct device *dev, void *user_data,
					 uint32_t channel, int status)
{
	struct uart_event evt = {0};
	const struct device *uart_dev = user_data;
	const struct uart_mspm0_config *cfg = uart_dev->config;
	struct uart_mspm0_data *data = uart_dev->data;
	unsigned int key = irq_lock();

	if (channel != cfg->tx_dma_channel) {
		irq_unlock(key);
		return;
	}

	k_work_cancel_delayable(&data->async_tx.timeout_work);
	DL_UART_disableDMATransmitEvent(cfg->regs);

	evt.type = UART_TX_DONE;
	evt.data.tx.buf = data->async_tx.buf;
	evt.data.tx.len = data->async_tx.buf_len;
	if (data->async_cb) {
		data->async_cb(uart_dev, &evt, data->async_user_data);
	}

	data->async_tx.buf = NULL;
	data->async_tx.buf_len = 0;
	irq_unlock(key);
}

static int uart_mspm0_async_tx_abort(const struct device *dev)
{
	int ret;
	const struct uart_mspm0_config *cfg = dev->config;
	struct uart_mspm0_data *data = dev->data;
	struct uart_event evt = {0};
	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async_tx.timeout_work);
	if (cfg->tx_dma_channel == 0xFF) {
		ret = -EINVAL;
		goto out;
	}

	DL_UART_disableDMATransmitEvent(cfg->regs);
	ret = dma_stop(cfg->dma_dev, cfg->tx_dma_channel);
	if (ret < 0) {
		goto out;
	}

	evt.type = UART_TX_ABORTED;
	evt.data.tx.buf = data->async_tx.buf;
	evt.data.tx.len = data->async_tx.buf_len;
	if (data->async_cb) {
		data->async_cb(data->uart_dev, &evt, data->async_user_data);
	}

out:
	irq_unlock(key);

	return ret;
}

static int uart_mspm0_async_callback_set(const struct device *dev,
					uart_callback_t callback, void *userdata)
{
	struct uart_mspm0_data *data = dev->data;

	if (!callback) {
		return -EINVAL;
	}

	data->async_cb = callback;
	data->async_user_data = userdata;

	return 0;
}

static int uart_mspm0_async_tx(const struct device *dev, const uint8_t *buf,
			       size_t len, int32_t timeout)
{
	int ret;
	struct uart_mspm0_data *data = dev->data;
	const struct uart_mspm0_config *cfg = dev->config;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	unsigned int key = irq_lock();

	if (cfg->tx_dma_channel == 0xFF) {
		ret = -EINVAL;
		goto unlock;
	}

	data->async_tx.buf		= buf;
	data->async_tx.buf_len		= len;
	dma_cfg.source_data_size	= 1;
	dma_cfg.dest_data_size		= 1;
	dma_cfg.channel_direction	= MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_callback		= uart_mspm0_async_tx_callback;
	dma_cfg.user_data		= (void *)dev;
	dma_cfg.dma_slot		= cfg->tx_trigger;
	dma_cfg.head_block		= &dma_blk;
	dma_blk.block_size		= len;
	dma_blk.source_address		= (uint32_t)buf;
	dma_blk.dest_address		= (uint32_t)&cfg->regs->TXDATA;
	dma_blk.dest_addr_adj		= DMA_ADDR_ADJ_NO_CHANGE;
	dma_blk.source_addr_adj		= DMA_ADDR_ADJ_INCREMENT;

	DL_UART_Main_enableDMATransmitEvent(cfg->regs);
	ret = dma_config(cfg->dma_dev, cfg->tx_dma_channel, &dma_cfg);
	if (ret < 0) {
		goto unlock;
	}

	uart_mspm0_async_timer_start(&data->async_tx.timeout_work, timeout);
	ret = dma_start(cfg->dma_dev, cfg->tx_dma_channel);
	if (ret < 0) {
		goto unlock;
	}

unlock:
	irq_unlock(key);

	return ret;
}

static int uart_mspm0_async_rx_enable(const struct device *dev, uint8_t *buf,
				      size_t len, int32_t timeout)
{
	int ret;
	const struct uart_mspm0_config *cfg = dev->config;
	struct uart_mspm0_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	struct uart_event evt = {0};
	unsigned int key = irq_lock();

	if (cfg->rx_dma_channel == 0xFF) {
		ret = -EINVAL;
		goto unlock;
	}

	data->async_rx.buf	  = buf;
	data->async_rx.buf_len	  = len;
	data->async_rx.timeout	  = timeout;
	dma_cfg.source_data_size  = 1;
	dma_cfg.dest_data_size	  = 1;
	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.dma_callback	  = uart_mspm0_async_rx_callback;
	dma_cfg.user_data	  = (void *)dev;
	dma_cfg.dma_slot	  = cfg->rx_trigger;
	dma_cfg.head_block	  = &dma_blk;
	dma_blk.block_size	  = len;
	dma_blk.dest_address	  = (uint32_t)data->async_rx.buf;
	dma_blk.source_address	  = (uint32_t)&cfg->regs->RXDATA;
	dma_blk.source_addr_adj	  = DMA_ADDR_ADJ_NO_CHANGE;
	dma_blk.dest_addr_adj	  = DMA_ADDR_ADJ_INCREMENT;

//	DL_UART_Main_enableInterrupt(cfg->regs, DL_UART_IIDX_RX_TIMEOUT_ERROR);
//	DL_UART_setRXInterruptTimeout(cfg->regs, 15);
	DL_UART_Main_enableDMAReceiveEvent(cfg->regs, DL_UART_DMA_INTERRUPT_RX);
	ret = dma_config(cfg->dma_dev, cfg->rx_dma_channel, &dma_cfg);
	if (ret < 0) {
		goto unlock;
	}

	ret = dma_start(cfg->dma_dev, cfg->rx_dma_channel);
	if (ret < 0) {
		goto unlock;
	}

	uart_mspm0_async_timer_start(&data->async_rx.timeout_work,
				     data->async_rx.timeout);

	evt.type = UART_RX_BUF_REQUEST;
	if (data->async_cb) {
		data->async_cb(dev, &evt, data->async_user_data);
	}

unlock:
	irq_unlock(key);

	return ret;
}

static int uart_mspm0_async_rx_buf_rsp(const struct device *dev, uint8_t *buf,
				       size_t len)
{
	struct uart_mspm0_data *data = dev->data;

	data->async_rx.next_buf = buf;
	data->async_rx.next_buf_len = len;

	return 0;
}

static int uart_mspm0_async_rx_disable(const struct device *dev)
{
	int ret;
	const struct uart_mspm0_config *cfg = dev->config;
	struct uart_mspm0_data *data = dev->data;
	struct dma_status status = {0};
	unsigned int key = irq_lock();
	struct uart_event evt = {0};

	k_work_cancel_delayable(&data->async_rx.timeout_work);
	if (cfg->rx_dma_channel == 0xFF) {
		ret = -EINVAL;
		goto unlock;
	}

	if (!data->async_rx.buf_len) {
		ret = -EINVAL;
		goto unlock;
	}

	ret = dma_get_status(cfg->dma_dev, cfg->rx_dma_channel, &status);
	if (ret < 0) {
		goto unlock;
	}

	ret = dma_stop(cfg->dma_dev, cfg->rx_dma_channel);
	if (ret < 0) {
		goto unlock;
	}

	evt.type = UART_RX_RDY;
	evt.data.rx.buf = data->async_rx.buf;
	evt.data.rx.len = data->async_rx.buf_len - status.pending_length;
	evt.data.rx.offset = data->async_rx.offset;

	if (data->async_cb && evt.data.rx.len) {
		data->async_cb(data->uart_dev, &evt, data->async_user_data);

		data->async_rx.offset = 0;
		evt.type = UART_RX_BUF_RELEASED;
		evt.data.rx_buf.buf = data->async_rx.buf;
		data->async_cb(dev, &evt, data->async_user_data);

		data->async_rx.buf_len = 0;
		data->async_rx.buf = NULL;

		if (data->async_rx.next_buf) {
			evt.type = UART_RX_BUF_RELEASED;
			evt.data.rx_buf.buf = data->async_rx.next_buf;
			data->async_cb(dev, &evt, data->async_user_data);
		}

		data->async_rx.next_buf_len = 0;
		data->async_rx.next_buf = NULL;

		evt.type = UART_RX_DISABLED;
			data->async_cb(dev, &evt, data->async_user_data);
	}

unlock:
	irq_unlock(key);

	return ret;
}

static void uart_mspm0_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *timeout_work = k_work_delayable_from_work(work);
	struct uart_mspm0_async_tx_data *async_tx =
		CONTAINER_OF(timeout_work, struct uart_mspm0_async_tx_data, timeout_work);
	struct uart_mspm0_data *data = CONTAINER_OF(async_tx, struct uart_mspm0_data, async_tx);

	uart_mspm0_async_tx_abort(data->uart_dev);
}

static void uart_mspm0_async_rx_timeout(struct k_work *work)
{
	size_t recv_size;
	struct k_work_delayable *timeout_work = k_work_delayable_from_work(work);
	struct uart_mspm0_async_rx_data *async_rx =
		CONTAINER_OF(timeout_work, struct uart_mspm0_async_rx_data, timeout_work);
	struct uart_mspm0_data *data = CONTAINER_OF(async_rx, struct uart_mspm0_data, async_rx);
	const struct device *uart_dev = data->uart_dev;
	const struct uart_mspm0_config *cfg = uart_dev->config;
	struct uart_event evt = {0};
	struct dma_status stat = {0};
	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async_rx.timeout_work);

	dma_get_status(cfg->dma_dev, cfg->rx_dma_channel, &stat);
	recv_size = data->async_rx.buf_len - stat.pending_length;
	if ((recv_size) && (data->async_cb)) {
		evt.type = UART_RX_RDY;
		evt.data.rx.buf = data->async_rx.buf;
		evt.data.rx.len = recv_size;
		evt.data.rx.offset = data->async_rx.offset;
		data->async_cb(data->uart_dev, &evt, data->async_user_data);
		data->async_rx.buf_len -= recv_size;
		data->async_rx.offset += recv_size;
	}

	uart_mspm0_async_timer_start(&data->async_rx.timeout_work,
				     data->async_rx.timeout);

	irq_unlock(key);
}
#endif /* CONFIG_UART_ASYNC_API */

static int uart_mspm0_init(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(clkmux));
	uint32_t clock_rate;
	int ret;

	/* Reset power */
	DL_UART_Main_reset(config->regs);
	DL_UART_Main_enablePower(config->regs);
	delay_cycles(POWER_STARTUP_DELAY);

	/* Init UART pins */
	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Set UART configs */
	DL_UART_Main_setClockConfig(config->regs,
				    (DL_UART_Main_ClockConfig *)&data->UART_ClockConfig);
	DL_UART_Main_init(config->regs, (DL_UART_Main_Config *)&data->UART_Config);

	/*
	 * Configure baud rate by setting oversampling and baud rate divisor
	 * from the device tree data current-speed
	 */
	ret = clock_control_get_rate(clk_dev, (clock_control_subsys_t)config->clock_subsys,
				     &clock_rate);

	if (ret < 0) {
		return ret;
	}

	DL_UART_Main_configBaudRate(config->regs, clock_rate, config->current_speed);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined (CONFIG_UART_ASYNC_API)
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
	data->dma_dev = config->dma_dev;
	data->uart_dev = dev;

	k_work_init_delayable(&data->async_tx.timeout_work, uart_mspm0_async_tx_timeout);
	k_work_init_delayable(&data->async_rx.timeout_work, uart_mspm0_async_rx_timeout);
#endif

	/* Enable UART */
	DL_UART_Main_enable(config->regs);

	return 0;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 */
static void uart_mspm0_isr(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *const dev_data = dev->data;
	uint32_t int_status;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Perform callback if defined */
	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}
#endif
	int_status = DL_UART_Main_getEnabledInterruptStatus(
		config->regs, UART_MSPM0_TX_INTERRUPTS | UART_MSPM0_RX_INTERRUPTS);
	DL_UART_Main_clearInterruptStatus(config->regs, int_status);
}

#define MSP_UART_IRQ_REGISTER(index)                                                               \
	static void uart_mspm0_##index##_irq_register(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), uart_mspm0_isr,     \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}
#else

#define MSP_UART_IRQ_REGISTER(index)

#endif

static const struct uart_driver_api uart_mspm0_driver_api = {
	.poll_in = uart_mspm0_poll_in,
	.poll_out = uart_mspm0_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_mspm0_fifo_fill,
	.fifo_read = uart_mspm0_fifo_read,
	.irq_tx_enable = uart_mspm0_irq_tx_enable,
	.irq_tx_disable = uart_mspm0_irq_tx_disable,
	.irq_tx_ready = uart_mspm0_irq_tx_ready,
	.irq_rx_enable = uart_mspm0_irq_rx_enable,
	.irq_rx_disable = uart_mspm0_irq_rx_disable,
	.irq_tx_complete = uart_mspm0_irq_tx_complete,
	.irq_rx_ready = uart_mspm0_irq_rx_ready,
	.irq_is_pending = uart_mspm0_irq_is_pending,
	.irq_update = uart_mspm0_irq_update,
	.irq_callback_set = uart_mspm0_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#if CONFIG_UART_ASYNC_API
	.callback_set = uart_mspm0_async_callback_set,
	.tx = uart_mspm0_async_tx,
	.tx_abort = uart_mspm0_async_tx_abort,
	.rx_enable = uart_mspm0_async_rx_enable,
	.rx_buf_rsp = uart_mspm0_async_rx_buf_rsp,
	.rx_disable = uart_mspm0_async_rx_disable,
#endif /*CONFIG_UART_ASYNC_API*/
};

#ifdef CONFIG_UART_ASYNC_API
#define MSPM0_ASYNC_DEFINE(index)								 \
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(index, tx),						 \
		    (.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(index, 0))),		 \
		    (.dma_dev = NULL) )
#endif

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define MSPM0_IRQ_HANDLER_FUNC(index) 								\
	.irq_config_func = uart_mspm0_##index##_irq_register,
#else
#define MSPM0_IRQ_HANDLER_FUNC(index)
#endif

#define MSPM0_UART_INIT_FN(index)                                                                \
                                                                                                 \
	PINCTRL_DT_INST_DEFINE(index);                                                           \
                                                                                                 \
	static const struct mspm0_clockSys mspm0_uart_clockSys##index =                          \
		MSPM0_CLOCK_SUBSYS_FN(index);                                                    \
                                                                                                 \
	MSP_UART_IRQ_REGISTER(index)                                                             \
                                                                                                 \
	static const struct uart_mspm0_config uart_mspm0_cfg_##index = {                         \
		.regs = (UART_Regs *)DT_INST_REG_ADDR(index),                                    \
		.current_speed = DT_INST_PROP(index, current_speed),                             \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                \
		.clock_subsys = &mspm0_uart_clockSys##index,                                     \
		MSPM0_IRQ_HANDLER_FUNC(index)							 \
		IF_ENABLED(CONFIG_UART_ASYNC_API,						 \
			   (MSPM0_ASYNC_DEFINE(index),						 \
		.tx_dma_channel = DT_DMAS_CELL_BY_NAME_OR(DT_DRV_INST(index), tx, channel, 0xFF),\
		.rx_dma_channel = DT_DMAS_CELL_BY_NAME_OR(DT_DRV_INST(index), rx, channel, 0xFF),\
		.tx_trigger = DT_DMAS_CELL_BY_NAME_OR(DT_DRV_INST(index), tx, trigger, 0xFF),	 \
		.rx_trigger = DT_DMAS_CELL_BY_NAME_OR(DT_DRV_INST(index), rx, trigger, 0xFF),))	 \
		};										 \
	static struct uart_mspm0_data uart_mspm0_data_##index = {                                \
			     .UART_ClockConfig = {.clockSel = (DT_INST_CLOCKS_CELL(index, bus) & \
						  MSPM0_CLOCK_SEL_MASK),			 \
			     .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1},           	 \
		.UART_Config =                                                                   \
			{                                                                        \
				.mode = DL_UART_MAIN_MODE_NORMAL,                                \
				.direction = DL_UART_MAIN_DIRECTION_TX_RX,                       \
				.flowControl = (DT_INST_PROP(index, hw_flow_control)             \
							? DL_UART_MAIN_FLOW_CONTROL_RTS_CTS      \
							: DL_UART_MAIN_FLOW_CONTROL_NONE),       \
				.parity = DL_UART_MAIN_PARITY_NONE,                              \
				.wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,                   \
				.stopBits = DL_UART_MAIN_STOP_BITS_ONE,                          \
			},                                                                       \
		MSPM0_INTERRUPT_CALLBACK_FN(index)};                                             \
												 \
	DEVICE_DT_INST_DEFINE(index, &uart_mspm0_init, NULL, &uart_mspm0_data_##index,           \
			      &uart_mspm0_cfg_##index, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,\
			      &uart_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_UART_INIT_FN)
