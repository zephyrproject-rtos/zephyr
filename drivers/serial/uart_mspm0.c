/*
 * Copyright (c) 2025-2026 Texas Instruments
 * Copyright (c) 2025 Linumiz
 * Copyright (c) 2025 Bang & Olufsen A/S, Denmark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_uart

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>

/* Driverlib includes */
#include <ti/driverlib/dl_uart_main.h>

#define UART_MSPM0_RX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_RX | UART_MSPM0_ERROR_INTERRUPTS)

#define UART_MSPM0_TX_INTERRUPTS (DL_UART_MAIN_INTERRUPT_TX | DL_UART_MAIN_INTERRUPT_EOT_DONE)

#define UART_MSPM0_ERROR_INTERRUPTS                                                                \
	(DL_UART_MAIN_INTERRUPT_BREAK_ERROR | DL_UART_MAIN_INTERRUPT_FRAMING_ERROR |               \
	 DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR | DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR)

struct uart_mspm0_config {
	UART_Regs *regs;
	const struct mspm0_sys_clock *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_mspm0_data {
	/* UART clock structure */
	DL_UART_Main_ClockConfig uart_clockconfig;
	/* Baud Rate */
	uint32_t current_speed;
	/* UART config structure */
	DL_UART_Main_Config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Callback function pointer */
	uart_irq_callback_user_data_t cb;
	/* Callback function arg */
	void *cb_data;
	/* Pending interrupt backup */
	DL_UART_IIDX pending_interrupt;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	/* Async callback function pointer */
	uart_callback_t async_cb;
	/* Async callback user data */
	void *async_user_data;

	/* RX data */
	uint8_t *rx_buffer;
	uint8_t *rx_secondary_buffer;
	size_t rx_buffer_length;
	size_t rx_secondary_buffer_length;
	volatile size_t rx_counter;
	volatile size_t rx_offset;
	int32_t rx_timeout;
	struct k_timer rx_timeout_timer;
	bool rx_enabled;

	/* TX data */
	const uint8_t *tx_buf;
	volatile size_t tx_len;
	volatile size_t tx_counter;
	bool tx_aborted;
	int32_t tx_timeout;
	struct k_timer tx_timeout_timer;

	/* Device pointer for timer callbacks */
	const struct device *dev;
#endif /* CONFIG_UART_ASYNC_API */
};

static int uart_mspm0_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

#ifdef CONFIG_UART_ASYNC_API
	/* If async RX is enabled, polling is not available */
	if (data->rx_enabled) {
		return -EBUSY;
	}
#endif

	if (DL_UART_Main_receiveDataCheck(config->regs, c) == false) {
		return -1;
	}

	return 0;
}

static void uart_mspm0_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

#ifdef CONFIG_UART_ASYNC_API
	/* If async TX is ongoing, wait for it to complete */
	while (data->tx_buf) {
		/* Just busy wait */
	}
#endif

	DL_UART_Main_transmitDataBlocking(config->regs, c);
}

static int uart_mspm0_install_configuration(const struct device *dev)
{
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	uint32_t clock_rate;
	int ret;

	/* Set UART configs */
	DL_UART_Main_setClockConfig(config->regs, &data->uart_clockconfig);
	DL_UART_Main_init(config->regs, &data->uart_config);

	/*
	 * Configure baud rate by setting oversampling and baud rate divisor
	 * from the selected current-speed
	 */
	ret = clock_control_get_rate(clk_dev, (struct mspm0_sys_clock *)config->clock_subsys,
				     &clock_rate);
	if (ret < 0) {
		return ret;
	}

	DL_UART_Main_configBaudRate(config->regs, clock_rate, data->current_speed);

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static const uint32_t uart_parity_to_mspm0[5] = {
	DL_UART_MAIN_PARITY_NONE,      DL_UART_MAIN_PARITY_ODD,        DL_UART_MAIN_PARITY_EVEN,
	DL_UART_MAIN_PARITY_STICK_ONE, DL_UART_MAIN_PARITY_STICK_ZERO,
};

static const uint32_t uart_stop_bits_to_mspm0[4] = {
	UINT32_MAX,
	DL_UART_MAIN_STOP_BITS_ONE,
	UINT32_MAX,
	DL_UART_MAIN_STOP_BITS_TWO,
};

static const uint32_t uart_data_bits_to_mspm0[4] = {
	DL_UART_MAIN_WORD_LENGTH_5_BITS,
	DL_UART_MAIN_WORD_LENGTH_6_BITS,
	DL_UART_MAIN_WORD_LENGTH_7_BITS,
	DL_UART_MAIN_WORD_LENGTH_8_BITS,
};

static const uint32_t uart_flow_control_to_mspm0[2] = {
	DL_UART_MAIN_FLOW_CONTROL_NONE,
	DL_UART_MAIN_FLOW_CONTROL_RTS_CTS,
};

static int uart_mspm0_translate_in(const uint32_t value_array[], int value_array_length,
				   uint8_t uart_cfg_value, uint32_t *mspm0_cfg_value)
{
	if (uart_cfg_value >= value_array_length) {
		return -EINVAL;
	}

	if (value_array[uart_cfg_value] == UINT32_MAX) {
		return -ENOSYS;
	}

	*mspm0_cfg_value = value_array[uart_cfg_value];

	return 0;
}

static int uart_mspm0_translate_out(const uint32_t value_array[], int value_array_length,
				    uint32_t mspm0_cfg_value, uint8_t *uart_cfg_value)
{
	int idx;

	for (idx = 0; idx < value_array_length; idx++) {
		if (value_array[idx] == mspm0_cfg_value) {
			break;
		}
	}

	if (idx == value_array_length || value_array[idx] == UINT32_MAX) {
		return -EINVAL;
	}

	*uart_cfg_value = (uint8_t)idx;

	return 0;
}

static int uart_mspm0_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	uint32_t value;
	int ret;

	DL_UART_Main_disable(config->regs);

	data->current_speed = cfg->baudrate;

	ret = uart_mspm0_translate_in(uart_parity_to_mspm0, ARRAY_SIZE(uart_parity_to_mspm0),
				      cfg->parity, &value);
	if (ret != 0) {
		return ret;
	}

	data->uart_config.parity = value;

	ret = uart_mspm0_translate_in(uart_stop_bits_to_mspm0, ARRAY_SIZE(uart_stop_bits_to_mspm0),
				      cfg->stop_bits, &value);
	if (ret != 0) {
		return ret;
	}

	data->uart_config.stopBits = value;

	ret = uart_mspm0_translate_in(uart_data_bits_to_mspm0, ARRAY_SIZE(uart_data_bits_to_mspm0),
				      cfg->data_bits, &value);
	if (ret != 0) {
		return ret;
	}

	data->uart_config.wordLength = value;

	ret = uart_mspm0_translate_in(uart_flow_control_to_mspm0,
				      ARRAY_SIZE(uart_flow_control_to_mspm0), cfg->flow_ctrl,
				      &value);
	if (ret != 0) {
		return ret;
	}

	data->uart_config.flowControl = value;

	ret = uart_mspm0_install_configuration(dev);
	if (ret != 0) {
		return ret;
	}

	DL_UART_Main_enable(config->regs);

	return 0;
}

static int uart_mspm0_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_mspm0_data *data = dev->data;
	int ret;

	cfg->baudrate = data->current_speed;

	ret = uart_mspm0_translate_out(uart_parity_to_mspm0, ARRAY_SIZE(uart_parity_to_mspm0),
				       data->uart_config.parity, &cfg->parity);
	if (ret != 0) {
		return ret;
	}

	ret = uart_mspm0_translate_out(uart_stop_bits_to_mspm0, ARRAY_SIZE(uart_stop_bits_to_mspm0),
				       data->uart_config.stopBits, &cfg->stop_bits);
	if (ret != 0) {
		return ret;
	}

	ret = uart_mspm0_translate_out(uart_data_bits_to_mspm0, ARRAY_SIZE(uart_data_bits_to_mspm0),
				       data->uart_config.wordLength, &cfg->data_bits);
	if (ret != 0) {
		return ret;
	}

	ret = uart_mspm0_translate_out(uart_flow_control_to_mspm0,
				       ARRAY_SIZE(uart_flow_control_to_mspm0),
				       data->uart_config.flowControl, &cfg->flow_ctrl);
	if (ret != 0) {
		return ret;
	}

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_mspm0_err_check(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;

	switch (data->pending_interrupt) {
	case DL_UART_MAIN_IIDX_BREAK_ERROR:
		return UART_BREAK;
	case DL_UART_MAIN_IIDX_FRAMING_ERROR:
		return UART_ERROR_FRAMING;
	default:
		return 0;
	}
}

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
	struct uart_mspm0_data *data = dev->data;

	return ((data->pending_interrupt == DL_UART_MAIN_IIDX_TX) ||
		(data->pending_interrupt == DL_UART_MAIN_IIDX_EOT_DONE)) &&
		!DL_UART_Main_isTXFIFOFull(config->regs) ? 1 : 0;
}

static void uart_mspm0_irq_rx_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_enableInterrupt(config->regs, DL_UART_MAIN_INTERRUPT_RX);
}

static void uart_mspm0_irq_rx_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_disableInterrupt(config->regs, DL_UART_MAIN_INTERRUPT_RX);
}

static int uart_mspm0_irq_tx_complete(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	return (DL_UART_Main_isTXFIFOEmpty(config->regs)) ? 1 : 0;
}

static int uart_mspm0_irq_rx_ready(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

	return ((data->pending_interrupt == DL_UART_MAIN_IIDX_RX) ||
		(data->pending_interrupt == DL_UART_MAIN_IIDX_RX_TIMEOUT_ERROR)) &&
		!DL_UART_Main_isRXFIFOEmpty(config->regs) ? 1 : 0;
}

static int uart_mspm0_irq_is_pending(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;

	return data->pending_interrupt != DL_UART_MAIN_IIDX_NO_INTERRUPT;
}

static int uart_mspm0_irq_update(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;
	const struct uart_mspm0_config *config = dev->config;

	data->pending_interrupt = DL_UART_Main_getPendingInterrupt(config->regs);

	return 1;
}

static void uart_mspm0_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	/* Set callback function and data */
	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

static void uart_mspm0_irq_error_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_enableInterrupt(config->regs, UART_MSPM0_ERROR_INTERRUPTS);
}

static void uart_mspm0_irq_error_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_ERROR_INTERRUPTS);
}

static void uart_mspm0_isr(const struct device *dev)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	/* Perform callback if defined */
	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API
static void uart_mspm0_async_evt_handler(const struct device *dev, enum uart_event_type event_type,
					 uint8_t *rx_buf_released, enum uart_rx_stop_reason reason)
{
	struct uart_mspm0_data *data = dev->data;
	struct uart_event evt;
	memset(&evt, 0, sizeof(evt));
	evt.type = event_type;

	switch (event_type) {
	case UART_RX_RDY:
		evt.data.rx.buf = data->rx_buffer;
		evt.data.rx.offset = data->rx_offset;
		evt.data.rx.len = data->rx_counter - data->rx_offset;
		data->rx_offset = data->rx_counter;
		break;

	case UART_RX_BUF_RELEASED:
		evt.data.rx_buf.buf = rx_buf_released;
		break;

	case UART_RX_DISABLED:
		/* No additional data needed */
		break;

	case UART_RX_BUF_REQUEST:
		/* No additional data needed */
		break;

	case UART_TX_DONE:
		evt.data.tx.buf = data->tx_buf;
		evt.data.tx.len = data->tx_counter;
		break;

	case UART_TX_ABORTED:
		evt.data.tx.buf = data->tx_buf;
		evt.data.tx.len = data->tx_counter;
		break;

	case UART_RX_STOPPED:
		evt.data.rx_stop.reason = reason;
		evt.data.rx_stop.data.buf = data->rx_buffer;
		evt.data.rx_stop.data.offset = data->rx_offset;
		evt.data.rx_stop.data.len = data->rx_counter - data->rx_offset;
		break;
	};

	if (data->async_cb) {
		data->async_cb(dev, &evt, data->async_user_data);
	}
}

static void uart_mspm0_async_rx_reset(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_RX_INTERRUPTS);

	while (!DL_UART_Main_isRXFIFOEmpty(config->regs)) {
		(void)DL_UART_Main_receiveData(config->regs);
	}

	data->rx_buffer = NULL;
	data->rx_buffer_length = 0;
	data->rx_counter = 0;
	data->rx_offset = 0;

	data->rx_secondary_buffer = NULL;
	data->rx_secondary_buffer_length = 0;

	data->rx_enabled = false;
}

static void uart_mspm0_async_rx_timeout(struct k_timer *timer)
{
	struct uart_mspm0_data *data =
		CONTAINER_OF(timer, struct uart_mspm0_data, rx_timeout_timer);

	const struct device *dev = data->dev;

	if (data->rx_counter > data->rx_offset) {
		uart_mspm0_async_evt_handler(dev, UART_RX_RDY, NULL, 0);
	}
}

static void uart_mspm0_async_tx_timeout(struct k_timer *timer)
{
	struct uart_mspm0_data *data =
		CONTAINER_OF(timer, struct uart_mspm0_data, tx_timeout_timer);

	const struct device *dev = data->dev;
	const struct uart_mspm0_config *config = dev->config;

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_TX_INTERRUPTS);

	uart_mspm0_async_evt_handler(dev, UART_TX_ABORTED, NULL, 0);

	data->tx_buf = NULL;
	data->tx_len = 0;
	data->tx_counter = 0;
}
#endif /* CONFIG_UART_ASYNC_API */

#ifdef CONFIG_UART_ASYNC_API
static int uart_mspm0_callback_set(const struct device *dev, uart_callback_t callback,
				   void *user_data)
{
	struct uart_mspm0_data *data = dev->data;

	data->async_cb = callback;
	data->async_user_data = user_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS) && defined(CONFIG_UART_INTERRUPT_DRIVEN)
	data->cb = NULL;
	data->cb_data = NULL;
#endif

	return 0;
}

static int uart_mspm0_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

	if (data->tx_buf) {
		return -EBUSY;
	}

	data->tx_buf = buf;
	data->tx_len = len;
	data->tx_counter = 0;
	data->tx_aborted = false;
	data->tx_timeout = timeout;

	DL_UART_Main_clearInterruptStatus(config->regs, UART_MSPM0_TX_INTERRUPTS);

	DL_UART_Main_enableInterrupt(config->regs, UART_MSPM0_TX_INTERRUPTS);

	data->tx_counter = DL_UART_Main_fillTXFIFO(config->regs, data->tx_buf, data->tx_len);

	return 0;
}

static int uart_mspm0_tx_abort(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

	if (!data->tx_buf) {
		return -EINVAL;
	}

	data->tx_aborted = true;

	if (data->tx_timeout != SYS_FOREVER_US) {
		k_timer_stop(&data->tx_timeout_timer);
	}

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_TX_INTERRUPTS);

	uart_mspm0_async_evt_handler(dev, UART_TX_ABORTED, NULL, 0);

	data->tx_buf = NULL;
	data->tx_len = 0;
	data->tx_counter = 0;

	return 0;
}

static int uart_mspm0_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

	if (data->rx_enabled) {
		return -EBUSY;
	}

	data->rx_buffer = buf;
	data->rx_buffer_length = len;
	data->rx_counter = 0;
	data->rx_offset = 0;

	data->rx_secondary_buffer = NULL;
	data->rx_secondary_buffer_length = 0;

	data->rx_enabled = true;
	data->rx_timeout = timeout;

	if (data->rx_secondary_buffer_length == 0) {
		uart_mspm0_async_evt_handler(dev, UART_RX_BUF_REQUEST, NULL, 0);
	}

	DL_UART_Main_clearInterruptStatus(config->regs, UART_MSPM0_RX_INTERRUPTS);

	while (!DL_UART_Main_isRXFIFOEmpty(config->regs)) {
		(void)DL_UART_Main_receiveData(config->regs);
	}

	DL_UART_Main_enableInterrupt(config->regs, UART_MSPM0_RX_INTERRUPTS);

	return 0;
}

static int uart_mspm0_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_mspm0_data *data = dev->data;
	int ret;
	unsigned int key;

	key = irq_lock();

	if (!data->rx_enabled) {
		ret = -EACCES;
	} else if (data->rx_secondary_buffer) {
		ret = -EBUSY;
	} else {
		data->rx_secondary_buffer = buf;
		data->rx_secondary_buffer_length = len;
		ret = 0;
	}

	irq_unlock(key);

	return ret;
}

static int uart_mspm0_rx_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

	if (!data->rx_enabled) {
		return -EFAULT;
	}

	if (data->rx_timeout != SYS_FOREVER_US) {
		k_timer_stop(&data->rx_timeout_timer);
	}

	DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_RX_INTERRUPTS);

	data->rx_enabled = false;

	if (!DL_UART_Main_isRXFIFOEmpty(config->regs)) {
		uart_mspm0_async_evt_handler(dev, UART_RX_RDY, NULL, 0);
	}

	uart_mspm0_async_evt_handler(dev, UART_RX_BUF_RELEASED, data->rx_buffer, 0);

	uart_mspm0_async_evt_handler(dev, UART_RX_DISABLED, NULL, 0);

	return 0;
}

static void uart_mspm0_async_tx_isr(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

	if (data->tx_buf == NULL) {
		DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_TX_INTERRUPTS);
		return;
	}

	size_t c = DL_UART_Main_fillTXFIFO(config->regs, &data->tx_buf[data->tx_counter], data->tx_len - data->tx_counter);
	data->tx_counter += c;

	if (data->tx_timeout != SYS_FOREVER_US) {
		k_timer_start(&data->tx_timeout_timer, K_USEC(data->tx_timeout), K_NO_WAIT);
	}
	if (data->tx_counter >= data->tx_len) {
		if (data->tx_timeout != SYS_FOREVER_US) {
			k_timer_stop(&data->tx_timeout_timer);
		}

		DL_UART_Main_disableInterrupt(config->regs, UART_MSPM0_TX_INTERRUPTS);

		data->tx_buf = NULL;
		data->tx_len = 0;
		data->tx_counter = 0;

		uart_mspm0_async_evt_handler(dev, UART_TX_DONE, NULL, 0);
	}
}

static void uart_mspm0_async_rx_isr(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	bool buffer_full = false;
	size_t rx_count = 0;

	if (!data->rx_enabled || !data->rx_buffer) {
		while (!DL_UART_Main_isRXFIFOEmpty(config->regs)) {
			(void)DL_UART_Main_receiveData(config->regs);
		}
		return;
	}

	while (!DL_UART_Main_isRXFIFOEmpty(config->regs)) {

		uint8_t rx_data = DL_UART_Main_receiveData(config->regs);
		rx_count++;

		if (data->rx_counter >= data->rx_buffer_length - 1) {
			buffer_full = true;
			data->rx_buffer[data->rx_counter++] = rx_data;
			break;
		}

		data->rx_buffer[data->rx_counter++] = rx_data;
	}

	if (rx_count > 0) {
		if (data->rx_timeout == 0) {
			uart_mspm0_async_evt_handler(dev, UART_RX_RDY, NULL, 0);
		} else if (data->rx_timeout != SYS_FOREVER_US) {
			k_timer_start(&data->rx_timeout_timer, K_USEC(data->rx_timeout), K_NO_WAIT);
		}
	}

	if (buffer_full) {

		if (data->rx_timeout != SYS_FOREVER_US) {
			k_timer_stop(&data->rx_timeout_timer);
		}

		uart_mspm0_async_evt_handler(dev, UART_RX_RDY, NULL, 0);

		if (data->rx_secondary_buffer != NULL) {

			uart_mspm0_async_evt_handler(dev, UART_RX_BUF_RELEASED, data->rx_buffer, 0);

			data->rx_buffer = data->rx_secondary_buffer;
			data->rx_buffer_length = data->rx_secondary_buffer_length;
			data->rx_counter = 0;
			data->rx_offset = 0;

			data->rx_secondary_buffer = NULL;
			data->rx_secondary_buffer_length = 0;

			uart_mspm0_async_evt_handler(dev, UART_RX_BUF_REQUEST, NULL, 0);

		} else {

			uart_mspm0_rx_disable(dev);
		}
	}
}

static void uart_mspm0_async_err_isr(const struct device *dev, enum uart_rx_stop_reason reason)
{
	struct uart_mspm0_data *data = dev->data;

	if (data->rx_timeout != SYS_FOREVER_US) {
		k_timer_stop(&data->rx_timeout_timer);
	}

	uart_mspm0_rx_disable(dev);

	uart_mspm0_async_evt_handler(dev, UART_RX_STOPPED, NULL, reason);
}

static void uart_mspm0_async_isr(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	uint32_t pending_int = DL_UART_Main_getPendingInterrupt(config->regs);
	switch (pending_int) {
	case DL_UART_MAIN_IIDX_RX:
		uart_mspm0_async_rx_isr(dev);
		break;
	case DL_UART_MAIN_IIDX_BREAK_ERROR:
		uart_mspm0_async_err_isr(dev, UART_BREAK);
		break;
	case DL_UART_MAIN_IIDX_FRAMING_ERROR:
		uart_mspm0_async_err_isr(dev, UART_ERROR_FRAMING);
		break;
	case DL_UART_MAIN_IIDX_OVERRUN_ERROR:
		uart_mspm0_async_err_isr(dev, UART_ERROR_OVERRUN);
		break;
	case DL_UART_MAIN_IIDX_EOT_DONE:
	case DL_UART_MAIN_IIDX_TX:
		uart_mspm0_async_tx_isr(dev);
		break;
	default:
		break;
	}

	/* on read, hightest priority (current interrupt) is automatically cleared by the hardware */
	/* and the corresponding interrupt flag in RIS and MIS are cleared also */
}
#endif /* CONFIG_UART_ASYNC_API */

static int uart_mspm0_init(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	int ret;

	DL_UART_Main_reset(config->regs);
	DL_UART_Main_enablePower(config->regs);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = uart_mspm0_install_configuration(dev);
	if (ret != 0) {
		return ret;
	}


#ifdef CONFIG_UART_ASYNC_API
	data->async_cb = NULL;
	data->async_user_data = NULL;

	data->rx_buffer = NULL;
	data->rx_buffer_length = 0;
	data->rx_counter = 0;
	data->rx_offset = 0;

	data->rx_secondary_buffer = NULL;
	data->rx_secondary_buffer_length = 0;

	data->rx_enabled = false;
	data->tx_buf = NULL;
	data->tx_len = 0;
	data->tx_counter = 0;
	data->tx_aborted = false;
	data->dev = dev;

	k_timer_init(&data->rx_timeout_timer, uart_mspm0_async_rx_timeout, NULL);
	k_timer_init(&data->tx_timeout_timer, uart_mspm0_async_tx_timeout, NULL);

	DL_UART_Main_clearInterruptStatus(config->regs, 0xFFFFFFFF);
#endif /* CONFIG_UART_ASYNC_API */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
	if (config->irq_config_func) {
		config->irq_config_func(dev);
	}
	DL_UART_Main_enableFIFOs(config->regs);
	DL_UART_Main_setRXFIFOThreshold(config->regs, DL_UART_RX_FIFO_LEVEL_1_2_FULL);
	DL_UART_Main_setTXFIFOThreshold(config->regs, DL_UART_TX_FIFO_LEVEL_EMPTY);
	DL_UART_Main_setRXInterruptTimeout(config->regs, 15U);

#endif /* CONFIG_UART_INTERRUPT_DRIVEN OR CONFIG_UART_ASYNC_API */

	DL_UART_Main_enable(config->regs);

	return 0;
}

static DEVICE_API(uart, uart_mspm0_driver_api) = {
	.poll_in = uart_mspm0_poll_in,
	.poll_out = uart_mspm0_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_mspm0_configure,
	.config_get = uart_mspm0_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.err_check = uart_mspm0_err_check,
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
	.irq_err_enable = uart_mspm0_irq_error_enable,
	.irq_err_disable = uart_mspm0_irq_error_disable,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_ASYNC_API
	.callback_set = uart_mspm0_callback_set,
	.tx = uart_mspm0_tx,
	.tx_abort = uart_mspm0_tx_abort,
	.rx_enable = uart_mspm0_rx_enable,
	.rx_buf_rsp = uart_mspm0_rx_buf_rsp,
	.rx_disable = uart_mspm0_rx_disable,
#endif /* CONFIG_UART_ASYNC_API */
};

static void uart_mspm0_combined_isr(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;

#ifdef CONFIG_UART_ASYNC_API
	/* If async callback is set, use async handler */
	if (data->async_cb) {
		uart_mspm0_async_isr(dev);
		return;
	}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Otherwise use interrupt-driven handler */
	if (data->cb) {
		uart_mspm0_isr(dev);
	}
#endif
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN) || defined(CONFIG_UART_ASYNC_API)
#define MSP_UART_IRQ_DEFINE(inst)                                                                  \
	static void uart_mspm0_##inst##_irq_register(const struct device *dev)                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    uart_mspm0_combined_isr, DEVICE_DT_INST_GET(inst), 0);                 \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}
#else
#define MSP_UART_IRQ_DEFINE(inst)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API */

#define MSPM0_MAIN_CLK_DIV(n) CONCAT(DL_UART_MAIN_CLOCK_DIVIDE_RATIO_, DT_INST_PROP(n, clk_div))

#define MSPM0_UART_INIT_FN(index)                                                                  \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static const struct mspm0_sys_clock mspm0_uart_sys_clock##index =                          \
		MSPM0_CLOCK_SUBSYS_FN(index);                                                      \
                                                                                                   \
	MSP_UART_IRQ_DEFINE(index);                                                                \
                                                                                                   \
	static const struct uart_mspm0_config uart_mspm0_cfg_##index = {                           \
		.regs = (UART_Regs *)DT_INST_REG_ADDR(index),                                      \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                  \
		.clock_subsys = &mspm0_uart_sys_clock##index,                                      \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN,                                           \
			   (.irq_config_func = uart_mspm0_##index##_irq_register,))                \
				   IF_ENABLED(CONFIG_UART_ASYNC_API,                               \
			   (.irq_config_func = uart_mspm0_##index##_irq_register,)) };             \
                                                                                                   \
	static struct uart_mspm0_data uart_mspm0_data_##index = {                                  \
		.uart_clockconfig =                                                                \
			{                                                                          \
				.clockSel = MSPM0_CLOCK_PERIPH_REG_MASK(                           \
					DT_INST_CLOCKS_CELL(index, clk)),                          \
				.divideRatio = MSPM0_MAIN_CLK_DIV(index),                          \
			},                                                                         \
		.current_speed = DT_INST_PROP(index, current_speed),                               \
		.uart_config =                                                                     \
			{                                                                          \
				.mode = DL_UART_MAIN_MODE_NORMAL,                                  \
				.direction = DL_UART_MAIN_DIRECTION_TX_RX,                         \
				.flowControl = (DT_INST_PROP(index, hw_flow_control)               \
							? DL_UART_MAIN_FLOW_CONTROL_RTS_CTS        \
							: DL_UART_MAIN_FLOW_CONTROL_NONE),         \
				.parity = DL_UART_MAIN_PARITY_NONE,                                \
				.wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,                     \
				.stopBits = DL_UART_MAIN_STOP_BITS_ONE,                            \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &uart_mspm0_init, NULL, &uart_mspm0_data_##index,             \
			      &uart_mspm0_cfg_##index, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY,  \
			      &uart_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_UART_INIT_FN)
