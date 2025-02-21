/*
 * Copyright (c) 2019 Mohamed ElShahawi (extremegtx@hotmail.com)
 * Copyright (c) 2023-2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_uart

/* Include esp-idf headers first to avoid redefining BIT() macro */
/* TODO: include w/o prefix */
#ifdef CONFIG_SOC_SERIES_ESP32
#include <esp32/rom/ets_sys.h>
#include <esp32/rom/gpio.h>
#include <soc/dport_reg.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S2)
#include <esp32s2/rom/ets_sys.h>
#include <esp32s2/rom/gpio.h>
#include <soc/dport_reg.h>
#elif defined(CONFIG_SOC_SERIES_ESP32S3)
#include <esp32s3/rom/ets_sys.h>
#include <esp32s3/rom/gpio.h>
#include <zephyr/dt-bindings/clock/esp32s3_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C2)
#include <esp32c2/rom/ets_sys.h>
#include <esp32c2/rom/gpio.h>
#include <zephyr/dt-bindings/clock/esp32c2_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C3)
#include <esp32c3/rom/ets_sys.h>
#include <esp32c3/rom/gpio.h>
#include <zephyr/dt-bindings/clock/esp32c3_clock.h>
#elif defined(CONFIG_SOC_SERIES_ESP32C6)
#include <esp32c6/rom/ets_sys.h>
#include <esp32c6/rom/gpio.h>
#include <zephyr/dt-bindings/clock/esp32c6_clock.h>
#endif
#ifdef CONFIG_UART_ASYNC_API
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#include <hal/uhci_ll.h>
#endif
#include <soc/uart_struct.h>
#include <hal/uart_ll.h>
#include <hal/uart_hal.h>
#include <hal/uart_types.h>
#include <esp_clk_tree.h>
#include <zephyr/drivers/pinctrl.h>

#include <soc/uart_reg.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <zephyr/drivers/clock_control.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <esp_attr.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_esp32, CONFIG_UART_LOG_LEVEL);

struct uart_esp32_config {
	const struct device *clock_dev;
	const struct pinctrl_dev_config *pcfg;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
	int irq_priority;
	int irq_flags;
	bool tx_invert;
	bool rx_invert;
#if CONFIG_UART_ASYNC_API
	const struct device *dma_dev;
	uint8_t tx_dma_channel;
	uint8_t rx_dma_channel;
#endif
};

#if CONFIG_UART_ASYNC_API
struct uart_esp32_async_data {
	struct k_work_delayable tx_timeout_work;
	const uint8_t *tx_buf;
	size_t tx_len;
	struct k_work_delayable rx_timeout_work;
	uint8_t *rx_buf;
	uint8_t *rx_next_buf;
	size_t rx_len;
	size_t rx_next_len;
	size_t rx_timeout;
	volatile size_t rx_counter;
	size_t rx_offset;
	uart_callback_t cb;
	void *user_data;
};
#endif

/* driver data */
struct uart_esp32_data {
	struct uart_config uart_config;
	uart_hal_context_t hal;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
#if CONFIG_UART_ASYNC_API
	struct uart_esp32_async_data async;
	uhci_dev_t *uhci_dev;
	const struct device *uart_dev;
#endif
};

#define UART_FIFO_LIMIT	    (UART_LL_FIFO_DEF_LEN)
#define UART_TX_FIFO_THRESH (CONFIG_UART_ESP32_TX_FIFO_THRESH)
#define UART_RX_FIFO_THRESH (CONFIG_UART_ESP32_RX_FIFO_THRESH)

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
static void uart_esp32_isr(void *arg);
#endif

static int uart_esp32_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_esp32_data *data = dev->data;
	int inout_rd_len = 1;

	if (uart_hal_get_rxfifo_len(&data->hal) == 0) {
		return -1;
	}

	uart_hal_read_rxfifo(&data->hal, p_char, &inout_rd_len);

	return 0;
}

static void uart_esp32_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_esp32_data *data = dev->data;
	uint32_t written;

	/* Wait for space in FIFO */
	while (uart_hal_get_txfifo_len(&data->hal) == 0) {
		; /* Wait */
	}

	/* Send a character */
	uart_hal_write_txfifo(&data->hal, &c, 1, &written);
}

static int uart_esp32_err_check(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;
	uint32_t mask = uart_hal_get_intsts_mask(&data->hal);
	uint32_t err = mask & (UART_INTR_PARITY_ERR | UART_INTR_FRAM_ERR);

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static uint32_t uart_esp32_get_standard_baud(uint32_t calc_baud)
{
	const uint32_t standard_bauds[] = {9600,  14400,  19200,  38400,  57600,
					   74880, 115200, 230400, 460800, 921600};
	int num_bauds = ARRAY_SIZE(standard_bauds);
	uint32_t baud = calc_baud;

	/* Find the standard baudrate within 0.1% range. If no close
	 * value is found, input is returned.
	 */
	for (int i = 0; i < num_bauds; i++) {
		float range = (float)abs(calc_baud - standard_bauds[i]) / standard_bauds[i];

		if (range < 0.001f) {
			baud = standard_bauds[i];
			break;
		}
	}

	return baud;
}

static int uart_esp32_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_esp32_data *data = dev->data;
	uart_parity_t parity;
	uart_stop_bits_t stop_bit;
	uart_word_length_t data_bit;
	uart_hw_flowcontrol_t hw_flow;
	uart_sclk_t src_clk;
	uint32_t sclk_freq;
	uint32_t calc_baud;

	uart_hal_get_sclk(&data->hal, &src_clk);
	esp_clk_tree_src_get_freq_hz((soc_module_clk_t)src_clk,
		ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &sclk_freq);

	uart_hal_get_baudrate(&data->hal, &calc_baud, sclk_freq);
	cfg->baudrate = uart_esp32_get_standard_baud(calc_baud);

	uart_hal_get_parity(&data->hal, &parity);
	switch (parity) {
	case UART_PARITY_DISABLE:
		cfg->parity = UART_CFG_PARITY_NONE;
		break;
	case UART_PARITY_EVEN:
		cfg->parity = UART_CFG_PARITY_EVEN;
		break;
	case UART_PARITY_ODD:
		cfg->parity = UART_CFG_PARITY_ODD;
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_get_stop_bits(&data->hal, &stop_bit);
	switch (stop_bit) {
	case UART_STOP_BITS_1:
		cfg->stop_bits = UART_CFG_STOP_BITS_1;
		break;
	case UART_STOP_BITS_1_5:
		cfg->stop_bits = UART_CFG_STOP_BITS_1_5;
		break;
	case UART_STOP_BITS_2:
		cfg->stop_bits = UART_CFG_STOP_BITS_2;
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_get_data_bit_num(&data->hal, &data_bit);
	switch (data_bit) {
	case UART_DATA_5_BITS:
		cfg->data_bits = UART_CFG_DATA_BITS_5;
		break;
	case UART_DATA_6_BITS:
		cfg->data_bits = UART_CFG_DATA_BITS_6;
		break;
	case UART_DATA_7_BITS:
		cfg->data_bits = UART_CFG_DATA_BITS_7;
		break;
	case UART_DATA_8_BITS:
		cfg->data_bits = UART_CFG_DATA_BITS_8;
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_get_hw_flow_ctrl(&data->hal, &hw_flow);
	switch (hw_flow) {
	case UART_HW_FLOWCTRL_DISABLE:
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
		break;
	case UART_HW_FLOWCTRL_CTS_RTS:
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
		break;
	default:
		return -ENOTSUP;
	}

	if (uart_hal_is_mode_rs485_half_duplex(&data->hal)) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RS485;
	}

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_esp32_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_esp32_config *config = dev->config;
	struct uart_esp32_data *data = dev->data;
	uart_sclk_t src_clk;
	uint32_t sclk_freq;

	int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		return ret;
	}

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	clock_control_on(config->clock_dev, config->clock_subsys);

	uart_hal_set_sclk(&data->hal, UART_SCLK_DEFAULT);
	uart_hal_set_rxfifo_full_thr(&data->hal, UART_RX_FIFO_THRESH);
	uart_hal_set_txfifo_empty_thr(&data->hal, UART_TX_FIFO_THRESH);
	uart_hal_rxfifo_rst(&data->hal);
	uart_hal_txfifo_rst(&data->hal);

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		uart_hal_set_parity(&data->hal, UART_PARITY_DISABLE);
		break;
	case UART_CFG_PARITY_EVEN:
		uart_hal_set_parity(&data->hal, UART_PARITY_EVEN);
		break;
	case UART_CFG_PARITY_ODD:
		uart_hal_set_parity(&data->hal, UART_PARITY_ODD);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		uart_hal_set_stop_bits(&data->hal, UART_STOP_BITS_1);
		break;
	case UART_CFG_STOP_BITS_1_5:
		uart_hal_set_stop_bits(&data->hal, UART_STOP_BITS_1_5);
		break;
	case UART_CFG_STOP_BITS_2:
		uart_hal_set_stop_bits(&data->hal, UART_STOP_BITS_2);
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		uart_hal_set_data_bit_num(&data->hal, UART_DATA_5_BITS);
		break;
	case UART_CFG_DATA_BITS_6:
		uart_hal_set_data_bit_num(&data->hal, UART_DATA_6_BITS);
		break;
	case UART_CFG_DATA_BITS_7:
		uart_hal_set_data_bit_num(&data->hal, UART_DATA_7_BITS);
		break;
	case UART_CFG_DATA_BITS_8:
		uart_hal_set_data_bit_num(&data->hal, UART_DATA_8_BITS);
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_set_mode(&data->hal, UART_MODE_UART);

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		uart_hal_set_hw_flow_ctrl(&data->hal, UART_HW_FLOWCTRL_DISABLE, 0);
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		uart_hal_set_hw_flow_ctrl(&data->hal, UART_HW_FLOWCTRL_CTS_RTS, 10);
		break;
	case UART_CFG_FLOW_CTRL_RS485:
		uart_hal_set_mode(&data->hal, UART_MODE_RS485_HALF_DUPLEX);
		break;
	default:
		return -ENOTSUP;
	}

	uart_hal_get_sclk(&data->hal, &src_clk);
	esp_clk_tree_src_get_freq_hz((soc_module_clk_t)src_clk,
		ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &sclk_freq);
	uart_hal_set_baudrate(&data->hal, cfg->baudrate, sclk_freq);

	uart_hal_set_rx_timeout(&data->hal, 0x16);

	if (config->tx_invert) {
		uart_hal_inverse_signal(&data->hal, UART_SIGNAL_TXD_INV);
	}

	if (config->rx_invert) {
		uart_hal_inverse_signal(&data->hal, UART_SIGNAL_RXD_INV);
	}
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_esp32_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_esp32_data *data = dev->data;
	uint32_t written = 0;

	if (len < 0) {
		return 0;
	}

	uart_hal_write_txfifo(&data->hal, tx_data, len, &written);
	return written;
}

static int uart_esp32_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	struct uart_esp32_data *data = dev->data;
	const int num_rx = uart_hal_get_rxfifo_len(&data->hal);
	int read = MIN(len, num_rx);

	if (!read) {
		return 0;
	}

	uart_hal_read_rxfifo(&data->hal, rx_data, &read);
	return read;
}

static void uart_esp32_irq_tx_enable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
}

static void uart_esp32_irq_tx_disable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_disable_intr_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
}

static int uart_esp32_irq_tx_ready(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	return (uart_hal_get_txfifo_len(&data->hal) > 0 &&
		uart_hal_get_intr_ena_status(&data->hal) & UART_INTR_TXFIFO_EMPTY);
}

static void uart_esp32_irq_rx_disable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_disable_intr_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_disable_intr_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
}

static int uart_esp32_irq_tx_complete(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	return uart_hal_is_tx_idle(&data->hal);
}

static int uart_esp32_irq_rx_ready(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	return (uart_hal_get_rxfifo_len(&data->hal) > 0);
}

static void uart_esp32_irq_err_enable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	/* enable framing, parity */
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_FRAM_ERR);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_PARITY_ERR);
}

static void uart_esp32_irq_err_disable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_disable_intr_mask(&data->hal, UART_INTR_FRAM_ERR);
	uart_hal_disable_intr_mask(&data->hal, UART_INTR_PARITY_ERR);
}

static int uart_esp32_irq_is_pending(const struct device *dev)
{
	return uart_esp32_irq_rx_ready(dev) || uart_esp32_irq_tx_ready(dev);
}

static int uart_esp32_irq_update(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);

	return 1;
}

static void uart_esp32_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_esp32_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->async.cb = NULL;
	data->async.user_data = NULL;
#endif
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_ASYNC_API

static inline void uart_esp32_async_timer_start(struct k_work_delayable *work, size_t timeout)
{
	if ((timeout != SYS_FOREVER_US) && (timeout != 0)) {
		LOG_DBG("Async timer started for %d us", timeout);
		k_work_reschedule(work, K_USEC(timeout));
	}
}

#endif
#if CONFIG_UART_ASYNC_API || CONFIG_UART_INTERRUPT_DRIVEN

static void uart_esp32_irq_rx_enable(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;

	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
}

static void uart_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct uart_esp32_data *data = dev->data;
	uint32_t uart_intr_status = uart_hal_get_intsts_mask(&data->hal);

	if (uart_intr_status == 0) {
		return;
	}
	uart_hal_clr_intsts_mask(&data->hal, uart_intr_status);

#if CONFIG_UART_INTERRUPT_DRIVEN
	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
#endif

#if CONFIG_UART_ASYNC_API
	if (uart_intr_status & UART_INTR_RXFIFO_FULL) {
		data->async.rx_counter++;
		uart_esp32_async_timer_start(&data->async.rx_timeout_work, data->async.rx_timeout);
	}
#endif
}

#endif

#if CONFIG_UART_ASYNC_API
static void IRAM_ATTR uart_esp32_dma_rx_done(const struct device *dma_dev, void *user_data,
					     uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	const struct uart_esp32_config *config = uart_dev->config;
	struct uart_esp32_data *data = uart_dev->data;
	struct uart_event evt = {0};
	unsigned int key = irq_lock();

	/* If the receive buffer is not complete we reload the DMA at current buffer position and
	 * let the timeout callback handle the notifications
	 */
	if (data->async.rx_counter != data->async.rx_len) {
		dma_reload(config->dma_dev, config->rx_dma_channel, 0,
			   (uint32_t)data->async.rx_buf + data->async.rx_counter,
			   data->async.rx_len - data->async.rx_counter);
		dma_start(config->dma_dev, config->rx_dma_channel);
		data->uhci_dev->pkt_thres.thrs = data->async.rx_len - data->async.rx_counter;
		irq_unlock(key);
		return;
	}

	/*Notify RX_RDY*/
	evt.type = UART_RX_RDY;
	evt.data.rx.buf = data->async.rx_buf;
	evt.data.rx.len = data->async.rx_counter - data->async.rx_offset;
	evt.data.rx.offset = data->async.rx_offset;

	if (data->async.cb && evt.data.rx.len) {
		data->async.cb(data->uart_dev, &evt, data->async.user_data);
	}

	data->async.rx_offset = 0;
	data->async.rx_counter = 0;

	/*Release current buffer*/
	evt.type = UART_RX_BUF_RELEASED;
	evt.data.rx_buf.buf = data->async.rx_buf;
	if (data->async.cb) {
		data->async.cb(uart_dev, &evt, data->async.user_data);
	}

	/*Load next buffer and request another*/
	data->async.rx_buf = data->async.rx_next_buf;
	data->async.rx_len = data->async.rx_next_len;
	data->async.rx_next_buf = NULL;
	data->async.rx_next_len = 0U;
	evt.type = UART_RX_BUF_REQUEST;
	if (data->async.cb) {
		data->async.cb(uart_dev, &evt, data->async.user_data);
	}

	/*Notify RX_DISABLED when there is no buffer*/
	if (!data->async.rx_buf) {
		evt.type = UART_RX_DISABLED;
		if (data->async.cb) {
			data->async.cb(uart_dev, &evt, data->async.user_data);
		}
	} else {
		/*Reload DMA with new buffer*/
		dma_reload(config->dma_dev, config->rx_dma_channel, 0, (uint32_t)data->async.rx_buf,
			   data->async.rx_len);
		dma_start(config->dma_dev, config->rx_dma_channel);
		data->uhci_dev->pkt_thres.thrs = data->async.rx_len;
	}

	irq_unlock(key);
}

static void IRAM_ATTR uart_esp32_dma_tx_done(const struct device *dma_dev, void *user_data,
					     uint32_t channel, int status)
{
	const struct device *uart_dev = user_data;
	struct uart_esp32_data *data = uart_dev->data;
	struct uart_event evt = {0};
	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async.tx_timeout_work);

	evt.type = UART_TX_DONE;
	evt.data.tx.buf = data->async.tx_buf;
	evt.data.tx.len = data->async.tx_len;
	if (data->async.cb) {
		data->async.cb(uart_dev, &evt, data->async.user_data);
	}

	/* Reset TX Buffer */
	data->async.tx_buf = NULL;
	data->async.tx_len = 0U;
	irq_unlock(key);
}

static int uart_esp32_async_tx_abort(const struct device *dev)
{
	const struct uart_esp32_config *config = dev->config;
	struct uart_esp32_data *data = dev->data;
	struct uart_event evt = {0};
	int err = 0;
	unsigned int key = irq_lock();

	k_work_cancel_delayable(&data->async.tx_timeout_work);

	err = dma_stop(config->dma_dev, config->tx_dma_channel);
	if (err) {
		LOG_ERR("Error stopping Tx DMA (%d)", err);
		goto unlock;
	}

	evt.type = UART_TX_ABORTED;
	evt.data.tx.buf = data->async.tx_buf;
	evt.data.tx.len = data->async.tx_len;

	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}

unlock:
	irq_unlock(key);
	return err;
}

static void uart_esp32_async_tx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_esp32_async_data *async =
		CONTAINER_OF(dwork, struct uart_esp32_async_data, tx_timeout_work);
	struct uart_esp32_data *data = CONTAINER_OF(async, struct uart_esp32_data, async);

	uart_esp32_async_tx_abort(data->uart_dev);
}

static void uart_esp32_async_rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct uart_esp32_async_data *async =
		CONTAINER_OF(dwork, struct uart_esp32_async_data, rx_timeout_work);
	struct uart_esp32_data *data = CONTAINER_OF(async, struct uart_esp32_data, async);
	struct uart_event evt = {0};
	unsigned int key = irq_lock();

	evt.type = UART_RX_RDY;
	evt.data.rx.buf = data->async.rx_buf;
	evt.data.rx.len = data->async.rx_counter - data->async.rx_offset;
	evt.data.rx.offset = data->async.rx_offset;

	if (data->async.cb && evt.data.rx.len) {
		data->async.cb(data->uart_dev, &evt, data->async.user_data);
	}

	data->async.rx_offset = data->async.rx_counter;
	k_work_cancel_delayable(&data->async.rx_timeout_work);
	irq_unlock(key);
}

static int uart_esp32_async_callback_set(const struct device *dev, uart_callback_t callback,
					 void *user_data)
{
	struct uart_esp32_data *data = dev->data;

	if (!callback) {
		return -EINVAL;
	}

	data->async.cb = callback;
	data->async.user_data = user_data;

#if defined(CONFIG_UART_EXCLUSIVE_API_CALLBACKS)
	data->irq_cb = NULL;
	data->irq_cb_data = NULL;
#endif

	return 0;
}

static int uart_esp32_async_tx(const struct device *dev, const uint8_t *buf, size_t len,
			       int32_t timeout)
{
	const struct uart_esp32_config *config = dev->config;
	struct uart_esp32_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	struct dma_status dma_status = {0};
	int err = 0;
	unsigned int key = irq_lock();

	if (config->tx_dma_channel == 0xFF) {
		LOG_ERR("Tx DMA channel is not configured");
		err = -ENOTSUP;
		goto unlock;
	}

	err = dma_get_status(config->dma_dev, config->tx_dma_channel, &dma_status);
	if (err) {
		LOG_ERR("Unable to get Tx status (%d)", err);
		goto unlock;
	}

	if (dma_status.busy) {
		LOG_ERR("Tx DMA Channel is busy");
		err = -EBUSY;
		goto unlock;
	}

	data->async.tx_buf = buf;
	data->async.tx_len = len;

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.dma_callback = uart_esp32_dma_tx_done;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_slot = ESP_GDMA_TRIG_PERIPH_UHCI0;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = len;
	dma_blk.source_address = (uint32_t)buf;

	err = dma_config(config->dma_dev, config->tx_dma_channel, &dma_cfg);
	if (err) {
		LOG_ERR("Error configuring Tx DMA (%d)", err);
		goto unlock;
	}

	uart_esp32_async_timer_start(&data->async.tx_timeout_work, timeout);

	err = dma_start(config->dma_dev, config->tx_dma_channel);
	if (err) {
		LOG_ERR("Error starting Tx DMA (%d)", err);
		goto unlock;
	}

unlock:
	irq_unlock(key);
	return err;
}

static int uart_esp32_async_rx_enable(const struct device *dev, uint8_t *buf, size_t len,
				      int32_t timeout)
{
	const struct uart_esp32_config *config = dev->config;
	struct uart_esp32_data *data = dev->data;
	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_blk = {0};
	struct dma_status dma_status = {0};
	int err = 0;
	struct uart_event evt = {0};

	if (config->rx_dma_channel == 0xFF) {
		LOG_ERR("Rx DMA channel is not configured");
		return -ENOTSUP;
	}

	err = dma_get_status(config->dma_dev, config->rx_dma_channel, &dma_status);
	if (err) {
		LOG_ERR("Unable to get Rx status (%d)", err);
		return err;
	}

	if (dma_status.busy) {
		LOG_ERR("Rx DMA Channel is busy");
		return -EBUSY;
	}

	unsigned int key = irq_lock();

	data->async.rx_buf = buf;
	data->async.rx_len = len;
	data->async.rx_timeout = timeout;

	dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma_cfg.dma_callback = uart_esp32_dma_rx_done;
	dma_cfg.user_data = (void *)dev;
	dma_cfg.dma_slot = ESP_GDMA_TRIG_PERIPH_UHCI0;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = len;
	dma_blk.dest_address = (uint32_t)data->async.rx_buf;

	err = dma_config(config->dma_dev, config->rx_dma_channel, &dma_cfg);
	if (err) {
		LOG_ERR("Error configuring Rx DMA (%d)", err);
		goto unlock;
	}

	/*
	 * Enable interrupt on first receive byte so we can start async timer
	 */
	uart_hal_set_rxfifo_full_thr(&data->hal, 1);
	uart_esp32_irq_rx_enable(dev);

	err = dma_start(config->dma_dev, config->rx_dma_channel);
	if (err) {
		LOG_ERR("Error starting Rx DMA (%d)", err);
		goto unlock;
	}

	data->uhci_dev->pkt_thres.thrs = len;

	/**
	 * Request next buffer
	 */
	evt.type = UART_RX_BUF_REQUEST;
	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}

unlock:
	irq_unlock(key);
	return err;
}

static int uart_esp32_async_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct uart_esp32_data *data = dev->data;

	data->async.rx_next_buf = buf;
	data->async.rx_next_len = len;

	return 0;
}

static int uart_esp32_async_rx_disable(const struct device *dev)
{
	const struct uart_esp32_config *config = dev->config;
	struct uart_esp32_data *data = dev->data;
	unsigned int key = irq_lock();
	int err = 0;
	struct uart_event evt = {0};

	k_work_cancel_delayable(&data->async.rx_timeout_work);

	if (!data->async.rx_len) {
		err = -EINVAL;
		goto unlock;
	}

	err = dma_stop(config->dma_dev, config->rx_dma_channel);
	if (err) {
		LOG_ERR("Error stopping Rx DMA (%d)", err);
		goto unlock;
	}

	/*If any bytes have been received notify RX_RDY*/
	evt.type = UART_RX_RDY;
	evt.data.rx.buf = data->async.rx_buf;
	evt.data.rx.len = data->async.rx_counter - data->async.rx_offset;
	evt.data.rx.offset = data->async.rx_offset;

	if (data->async.cb && evt.data.rx.len) {
		data->async.cb(data->uart_dev, &evt, data->async.user_data);
	}

	data->async.rx_offset = 0;
	data->async.rx_counter = 0;

	/* Release current buffer*/
	evt.type = UART_RX_BUF_RELEASED;
	evt.data.rx_buf.buf = data->async.rx_buf;

	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}

	data->async.rx_len = 0;
	data->async.rx_buf = NULL;

	/*Release next buffer*/
	if (data->async.rx_next_len) {
		evt.type = UART_RX_BUF_RELEASED;
		evt.data.rx_buf.buf = data->async.rx_next_buf;
		if (data->async.cb) {
			data->async.cb(dev, &evt, data->async.user_data);
		}

		data->async.rx_next_len = 0;
		data->async.rx_next_buf = NULL;
	}

	/*Notify UART_RX_DISABLED*/
	evt.type = UART_RX_DISABLED;
	if (data->async.cb) {
		data->async.cb(dev, &evt, data->async.user_data);
	}

unlock:
	irq_unlock(key);
	return err;
}

#endif /* CONFIG_UART_ASYNC_API */

static int uart_esp32_init(const struct device *dev)
{
	struct uart_esp32_data *data = dev->data;
	int ret = uart_esp32_configure(dev, &data->uart_config);

	if (ret < 0) {
		LOG_ERR("Error configuring UART (%d)", ret);
		return ret;
	}

#if CONFIG_UART_INTERRUPT_DRIVEN || CONFIG_UART_ASYNC_API
	const struct uart_esp32_config *config = dev->config;

	ret = esp_intr_alloc(config->irq_source,
			ESP_PRIO_TO_FLAGS(config->irq_priority) |
			ESP_INT_FLAGS_CHECK(config->irq_flags),
			(intr_handler_t)uart_esp32_isr,
			(void *)dev,
			NULL);
	if (ret < 0) {
		LOG_ERR("Error allocating UART interrupt (%d)", ret);
		return ret;
	}
#endif
#if CONFIG_UART_ASYNC_API
	if (config->dma_dev) {
		if (!device_is_ready(config->dma_dev)) {
			LOG_ERR("DMA device is not ready");
			return -ENODEV;
		}

		clock_control_on(config->clock_dev, (clock_control_subsys_t)ESP32_UHCI0_MODULE);
		uhci_ll_init(data->uhci_dev);
		uhci_ll_set_eof_mode(data->uhci_dev, UHCI_RX_IDLE_EOF | UHCI_RX_LEN_EOF);
		uhci_ll_attach_uart_port(data->uhci_dev, uart_hal_get_port_num(&data->hal));
		data->uart_dev = dev;

		k_work_init_delayable(&data->async.tx_timeout_work, uart_esp32_async_tx_timeout);
		k_work_init_delayable(&data->async.rx_timeout_work, uart_esp32_async_rx_timeout);
	}
#endif
	return 0;
}

static DEVICE_API(uart, uart_esp32_api) = {
	.poll_in = uart_esp32_poll_in,
	.poll_out = uart_esp32_poll_out,
	.err_check = uart_esp32_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_esp32_configure,
	.config_get = uart_esp32_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_esp32_fifo_fill,
	.fifo_read = uart_esp32_fifo_read,
	.irq_tx_enable = uart_esp32_irq_tx_enable,
	.irq_tx_disable = uart_esp32_irq_tx_disable,
	.irq_tx_ready = uart_esp32_irq_tx_ready,
	.irq_rx_enable = uart_esp32_irq_rx_enable,
	.irq_rx_disable = uart_esp32_irq_rx_disable,
	.irq_tx_complete = uart_esp32_irq_tx_complete,
	.irq_rx_ready = uart_esp32_irq_rx_ready,
	.irq_err_enable = uart_esp32_irq_err_enable,
	.irq_err_disable = uart_esp32_irq_err_disable,
	.irq_is_pending = uart_esp32_irq_is_pending,
	.irq_update = uart_esp32_irq_update,
	.irq_callback_set = uart_esp32_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#if CONFIG_UART_ASYNC_API
	.callback_set = uart_esp32_async_callback_set,
	.tx = uart_esp32_async_tx,
	.tx_abort = uart_esp32_async_tx_abort,
	.rx_enable = uart_esp32_async_rx_enable,
	.rx_buf_rsp = uart_esp32_async_rx_buf_rsp,
	.rx_disable = uart_esp32_async_rx_disable,
#endif /*CONFIG_UART_ASYNC_API*/
};

#if CONFIG_UART_ASYNC_API
#define ESP_UART_DMA_INIT(n)                                                                       \
	.dma_dev = ESP32_DT_INST_DMA_CTLR(n, tx),                                                  \
	.tx_dma_channel = ESP32_DT_INST_DMA_CELL(n, tx, channel),                                  \
	.rx_dma_channel = ESP32_DT_INST_DMA_CELL(n, rx, channel)

#define ESP_UART_UHCI_INIT(n)                                                                      \
	.uhci_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas), (&UHCI0), (NULL))

#else
#define ESP_UART_DMA_INIT(n)
#define ESP_UART_UHCI_INIT(n)
#endif

#define ESP32_UART_INIT(idx)                                                                       \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
                                                                                                   \
	static const DRAM_ATTR struct uart_esp32_config uart_esp32_cfg_port_##idx = {              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset),          \
		.irq_source = DT_INST_IRQ_BY_IDX(idx, 0, irq),                                     \
		.irq_priority = DT_INST_IRQ_BY_IDX(idx, 0, priority),                              \
		.irq_flags = DT_INST_IRQ_BY_IDX(idx, 0, flags),                                    \
		.tx_invert = DT_INST_PROP_OR(idx, tx_invert, false),                               \
		.rx_invert = DT_INST_PROP_OR(idx, rx_invert, false),                               \
		ESP_UART_DMA_INIT(idx)};                                                           \
                                                                                                   \
	static struct uart_esp32_data uart_esp32_data_##idx = {                                    \
		.uart_config = {.baudrate = DT_INST_PROP(idx, current_speed),                      \
				.parity = DT_INST_ENUM_IDX(idx, parity),                           \
				.stop_bits = DT_INST_ENUM_IDX(idx, stop_bits),                     \
				.data_bits = DT_INST_ENUM_IDX(idx, data_bits),                     \
				.flow_ctrl = MAX(COND_CODE_1(DT_INST_PROP(idx, hw_rs485_hd_mode),  \
							     (UART_CFG_FLOW_CTRL_RS485),           \
							     (UART_CFG_FLOW_CTRL_NONE)),           \
						 COND_CODE_1(DT_INST_PROP(idx, hw_flow_control),   \
							     (UART_CFG_FLOW_CTRL_RTS_CTS),         \
							     (UART_CFG_FLOW_CTRL_NONE)))},         \
		.hal =                                                                             \
			{                                                                          \
				.dev = (uart_dev_t *)DT_INST_REG_ADDR(idx),                        \
			},                                                                         \
		ESP_UART_UHCI_INIT(idx)};                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, uart_esp32_init, NULL, &uart_esp32_data_##idx,                  \
			      &uart_esp32_cfg_port_##idx, PRE_KERNEL_2,                            \
			      CONFIG_SERIAL_INIT_PRIORITY, &uart_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_UART_INIT);
