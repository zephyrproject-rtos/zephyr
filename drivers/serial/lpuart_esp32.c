/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lpuart

#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/clock/esp32c6_clock.h>
#include <esp_rom_sys.h>

#include <hal/uart_hal.h>
#if defined(CONFIG_SOC_ESP32C6_HPCORE) || defined(CONFIG_SOC_ESP32C5_HPCORE) ||                    \
	defined(CONFIG_SOC_ESP32P4_HPCORE)
#include <hal/uart_ll.h>
#include <hal/clk_tree_ll.h>
#include <hal/clk_tree_hal.h>
#include <hal/rtc_io_hal.h>
#include <hal/rtc_io_ll.h>
#include <hal/rtc_io_periph.h>
#include <soc/uart_pins.h>
#include <esp_private/esp_clk_tree_common.h>
#include <esp_clk_tree.h>
#include <ulp_lp_core_uart.h>
#if SOC_LP_GPIO_MATRIX_SUPPORTED
#include <soc/lp_gpio_sig_map.h>
#endif
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif

#define ESP_LP_UART_TX_IDLE_NUM_DEFAULT (0U)

/*
 * Mask of all error-class LP UART interrupts. The LP UART peripheral has no
 * RS485 mode, so only parity, framing, overrun and break apply.
 */
#define LP_UART_ESP32_ERR_INTR_MASK                                                                \
	(UART_INTR_PARITY_ERR | UART_INTR_FRAM_ERR | UART_INTR_RXFIFO_OVF | UART_INTR_BRK_DET)

struct lp_uart_esp32_data {
	uart_hal_context_t hal;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
	/* Error flags saved by the ISR before it clears the hardware. */
	uint32_t isr_error_flags;
#endif
};

struct lp_uart_esp32_config {
	uint8_t tx_io_num;
	uint8_t rx_io_num;
	uint8_t rts_io_num;
	uint8_t cts_io_num;
	int baud_rate;
	uint8_t data_bits;
	uint8_t parity;
	uint8_t stop_bits;
	uint8_t flow_ctrl;
	uint8_t rx_flow_ctrl_thresh;
	uint8_t lp_uart_source_clk;
	bool loopback;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	int irq_source;
	int irq_priority;
	int irq_flags;
#endif
};

static int lp_uart_esp32_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct lp_uart_esp32_data *data = dev->data;
	int inout_rd_len = 1;

	if (uart_hal_get_rxfifo_len(&data->hal) == 0) {
		return -1;
	}

	uart_hal_read_rxfifo(&data->hal, p_char, &inout_rd_len);

	return 0;
}

static void lp_uart_esp32_poll_out(const struct device *dev, unsigned char c)
{
	struct lp_uart_esp32_data *data = dev->data;
	int tx_len = 0;

	/* Wait for space in FIFO */
	while (uart_hal_get_txfifo_len(&data->hal) == 0) {
		; /* Wait */
	}

	uart_hal_write_txfifo(&data->hal, (const void *)&c, 1, &tx_len);
}

static int lp_uart_esp32_err_check(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;
	uint32_t mask;
	int errors = 0;

	/*
	 * In interrupt-driven mode the ISR saves error flags before clearing
	 * hardware, so consume the saved copy and also poll for any errors
	 * that arrived since the last ISR invocation. In pure polling mode
	 * read the hardware directly.
	 */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	unsigned int key = irq_lock();

	mask = data->isr_error_flags;
	data->isr_error_flags = 0;
	mask |= uart_hal_get_intsts_mask(&data->hal) & LP_UART_ESP32_ERR_INTR_MASK;
	uart_hal_clr_intsts_mask(&data->hal, mask & LP_UART_ESP32_ERR_INTR_MASK);

	irq_unlock(key);
#else
	mask = uart_hal_get_intsts_mask(&data->hal) & LP_UART_ESP32_ERR_INTR_MASK;
	uart_hal_clr_intsts_mask(&data->hal, mask);
#endif

	if (mask & UART_INTR_PARITY_ERR) {
		errors |= UART_ERROR_PARITY;
	}
	if (mask & UART_INTR_FRAM_ERR) {
		errors |= UART_ERROR_FRAMING;
	}
	if (mask & UART_INTR_RXFIFO_OVF) {
		errors |= UART_ERROR_OVERRUN;
	}
	if (mask & UART_INTR_BRK_DET) {
		errors |= UART_BREAK;
	}

	return errors;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int lp_uart_esp32_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct lp_uart_esp32_data *data = dev->data;
	uint32_t written = 0;

	if (len < 0) {
		return 0;
	}

	uart_hal_write_txfifo(&data->hal, tx_data, len, &written);
	return written;
}

static int lp_uart_esp32_fifo_read(const struct device *dev, uint8_t *rx_data, const int len)
{
	struct lp_uart_esp32_data *data = dev->data;
	const int num_rx = uart_hal_get_rxfifo_len(&data->hal);
	int read = MIN(len, num_rx);

	if (!read) {
		return 0;
	}

	uart_hal_read_rxfifo(&data->hal, rx_data, &read);
	return read;
}

static void lp_uart_esp32_irq_tx_enable(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
}

static void lp_uart_esp32_irq_tx_disable(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	uart_hal_disable_intr_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
}

static int lp_uart_esp32_irq_tx_ready(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	return (uart_hal_get_txfifo_len(&data->hal) > 0 &&
		uart_hal_get_intr_ena_status(&data->hal) & UART_INTR_TXFIFO_EMPTY);
}

static void lp_uart_esp32_irq_rx_enable(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_ena_intr_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
}

static void lp_uart_esp32_irq_rx_disable(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	uart_hal_disable_intr_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_disable_intr_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
}

static int lp_uart_esp32_irq_tx_complete(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	return uart_hal_is_tx_idle(&data->hal);
}

static int lp_uart_esp32_irq_rx_ready(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	return (uart_hal_get_rxfifo_len(&data->hal) > 0);
}

static void lp_uart_esp32_irq_err_enable(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	uart_hal_ena_intr_mask(&data->hal, LP_UART_ESP32_ERR_INTR_MASK);
}

static void lp_uart_esp32_irq_err_disable(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	uart_hal_disable_intr_mask(&data->hal, LP_UART_ESP32_ERR_INTR_MASK);
}

static int lp_uart_esp32_irq_is_pending(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	/* Data ready or error-only interrupt (e.g. framing error with no data ready). */
	return lp_uart_esp32_irq_rx_ready(dev) || lp_uart_esp32_irq_tx_ready(dev) ||
	       data->isr_error_flags;
}

static void lp_uart_esp32_irq_update(const struct device *dev)
{
	struct lp_uart_esp32_data *data = dev->data;

	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_FULL);
	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_RXFIFO_TOUT);
	uart_hal_clr_intsts_mask(&data->hal, UART_INTR_TXFIFO_EMPTY);
}

static void lp_uart_esp32_irq_callback_set(const struct device *dev,
					   uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct lp_uart_esp32_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

static void lp_uart_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct lp_uart_esp32_data *data = dev->data;
	uint32_t uart_intr_status = uart_hal_get_intsts_mask(&data->hal);

	if (uart_intr_status == 0) {
		return;
	}

	/* Save error flags before clearing so err_check() can report them. */
	data->isr_error_flags |= uart_intr_status & LP_UART_ESP32_ERR_INTR_MASK;

	uart_hal_clr_intsts_mask(&data->hal, uart_intr_status);

	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if defined(CONFIG_SOC_ESP32C6_HPCORE) || defined(CONFIG_SOC_ESP32C5_HPCORE) ||                    \
	defined(CONFIG_SOC_ESP32P4_HPCORE)

static int lp_uart_esp32_param_config(const struct device *dev)
{
	const struct lp_uart_esp32_config *const cfg = dev->config;
	struct lp_uart_esp32_data *data = dev->data;
	uint32_t sclk_freq = 0;

	if ((cfg->rx_flow_ctrl_thresh > SOC_LP_UART_FIFO_LEN) ||
	    (cfg->flow_ctrl > UART_CFG_FLOW_CTRL_RTS_CTS) ||
	    (cfg->data_bits > UART_CFG_DATA_BITS_8)) {
		return -EINVAL;
	}

#if CONFIG_SOC_SERIES_ESP32P4
	/*
	 * Drive the LP UART from the precise XTAL_D2 clock rather than the
	 * uncalibrated RC_FAST default, so the baud rate is accurate enough for
	 * reliable framing.
	 */
	esp_clk_tree_enable_src((soc_module_clk_t)SOC_MOD_CLK_XTAL_D2, true);
	sclk_freq = clk_hal_xtal_get_freq_mhz() * MHZ(1) >> 1;

	lp_uart_ll_enable_bus_clock(0, true);
	lp_uart_ll_set_source_clk(data->hal.dev, LP_UART_SCLK_XTAL_D2);
	lp_uart_ll_sclk_enable(0);

	/* Initialize the controller to normal UART mode with default parameters */
	uart_hal_init(&data->hal, 0);
#else
	/* Get LP UART source clock frequency */
	switch (clk_ll_rtc_fast_get_src()) {
	case SOC_RTC_FAST_CLK_SRC_XTAL_D2:
		sclk_freq = clk_hal_xtal_get_freq_mhz() * MHZ(1) >> 1;
		break;
	case SOC_RTC_FAST_CLK_SRC_RC_FAST:
		sclk_freq =
			esp_clk_tree_rc_fast_get_freq_hz(ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED) /
			clk_ll_rc_fast_get_divider();
		break;
#if SOC_CLK_LP_FAST_SUPPORT_LP_PLL
	case SOC_RTC_FAST_CLK_SRC_LP_PLL:
		sclk_freq = clk_ll_lp_pll_get_freq_mhz() * MHZ(1);
		break;
#endif
	default:
		return -EINVAL;
	}

	lp_uart_ll_enable_bus_clock(0, true);
	lp_uart_ll_reset_register(0);
	lp_uart_ll_set_source_clk(data->hal.dev, cfg->lp_uart_source_clk);
	lp_uart_ll_sclk_enable(0);
#endif

	/* Set protocol parameters from the configuration */
	lp_uart_ll_set_baudrate(data->hal.dev, cfg->baud_rate, sclk_freq);
	uart_hal_set_parity(&data->hal, cfg->parity);
	uart_hal_set_data_bit_num(&data->hal, cfg->data_bits);
	uart_hal_set_stop_bits(&data->hal, cfg->stop_bits);
	uart_hal_set_tx_idle_num(&data->hal, ESP_LP_UART_TX_IDLE_NUM_DEFAULT);
	uart_hal_set_hw_flow_ctrl(&data->hal, cfg->flow_ctrl, cfg->rx_flow_ctrl_thresh);

	/* Trigger RX interrupts on a single byte and on receiver idle timeout */
	uart_hal_set_rxfifo_full_thr(&data->hal, 1);
	uart_hal_set_rx_timeout(&data->hal, 0x16);

	/* Tie TX to RX inside the peripheral for internal-loopback self-test */
	if (cfg->loopback) {
		uart_hal_set_loop_back(&data->hal, true);
	}

	/* Reset Tx/Rx FIFOs */
	uart_hal_rxfifo_rst(&data->hal);
	uart_hal_txfifo_rst(&data->hal);

	return 0;
}

static void lp_uart_esp32_config_io(int pin, int direction, int func)
{
	int rtc_io_num = rtc_io_num_map[pin];

#if SOC_LP_IO_CLOCK_IS_INDEPENDENT
	rtcio_ll_enable_io_clock(true);
#endif
	rtcio_hal_function_select(rtc_io_num, RTCIO_LL_FUNC_RTC);
	rtcio_hal_set_direction(rtc_io_num, direction);
	rtcio_hal_iomux_func_sel(rtc_io_num, func);
}

static void lp_uart_esp32_set_pin(const struct device *dev)
{
	const struct lp_uart_esp32_config *const cfg = dev->config;

	/* Configure Tx Pin */
	lp_uart_esp32_config_io(cfg->tx_io_num, RTC_GPIO_MODE_OUTPUT_ONLY, LP_U0TXD_MUX_FUNC);

	/* Configure Rx Pin */
	lp_uart_esp32_config_io(cfg->rx_io_num, RTC_GPIO_MODE_INPUT_ONLY, LP_U0RXD_MUX_FUNC);

#if SOC_LP_GPIO_MATRIX_SUPPORTED
	/* Route the LP UART signals through the LP GPIO matrix to the pads */
	rtcio_ll_set_output_signal_matrix_source(rtc_io_num_map[cfg->tx_io_num],
						 LP_UART_TXD_PAD_OUT_IDX, false);
	rtcio_ll_set_input_signal_matrix_source(rtc_io_num_map[cfg->rx_io_num],
						LP_UART_RXD_PAD_IN_IDX, false);
#endif

	/* Configure RTS/CTS pins only when hardware flow control is enabled */
	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		lp_uart_esp32_config_io(cfg->rts_io_num, RTC_GPIO_MODE_OUTPUT_ONLY,
					LP_U0RTS_MUX_FUNC);
		lp_uart_esp32_config_io(cfg->cts_io_num, RTC_GPIO_MODE_INPUT_ONLY,
					LP_U0CTS_MUX_FUNC);
	}
}

static int lp_uart_esp32_init(const struct device *dev)
{
	int ret = 0;

	ret = lp_uart_esp32_param_config(dev);
	if (ret != 0) {
		return -EINVAL;
	}

	/* Configure LP UART IO pins */
	lp_uart_esp32_set_pin(dev);

	/*
	 * Routing the pads can latch a spurious byte into the RX FIFO; let the
	 * line settle, then discard it so the first received byte is real data.
	 */
	struct lp_uart_esp32_data *data = dev->data;

	esp_rom_delay_us(1000);
	uart_hal_rxfifo_rst(&data->hal);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	const struct lp_uart_esp32_config *const cfg = dev->config;

	ret = esp_intr_alloc(cfg->irq_source,
			     ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
				     ESP_INT_FLAGS_CHECK(cfg->irq_flags),
			     (intr_handler_t)lp_uart_esp32_isr, (void *)dev, NULL);
	if (ret != 0) {
		return ret;
	}
#endif

	return 0;
}

#endif /* CONFIG_SOC_ESP32C6_HPCORE || CONFIG_SOC_ESP32C5_HPCORE || CONFIG_SOC_ESP32P4_HPCORE */

static DEVICE_API(uart, lp_uart_esp32_api) = {
	.poll_in = lp_uart_esp32_poll_in,
	.poll_out = lp_uart_esp32_poll_out,
	.err_check = lp_uart_esp32_err_check,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = lp_uart_esp32_fifo_fill,
	.fifo_read = lp_uart_esp32_fifo_read,
	.irq_tx_enable = lp_uart_esp32_irq_tx_enable,
	.irq_tx_disable = lp_uart_esp32_irq_tx_disable,
	.irq_tx_ready = lp_uart_esp32_irq_tx_ready,
	.irq_rx_enable = lp_uart_esp32_irq_rx_enable,
	.irq_rx_disable = lp_uart_esp32_irq_rx_disable,
	.irq_tx_complete = lp_uart_esp32_irq_tx_complete,
	.irq_rx_ready = lp_uart_esp32_irq_rx_ready,
	.irq_err_enable = lp_uart_esp32_irq_err_enable,
	.irq_err_disable = lp_uart_esp32_irq_err_disable,
	.irq_is_pending = lp_uart_esp32_irq_is_pending,
	.irq_update = lp_uart_esp32_irq_update,
	.irq_callback_set = lp_uart_esp32_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static struct lp_uart_esp32_data lp_uart_esp32_data = {
	.hal = {.dev = (uart_dev_t *)DT_REG_ADDR(DT_NODELABEL(lp_uart))},
};

static const struct lp_uart_esp32_config lp_uart_esp32_cfg = {
	.tx_io_num = DT_PROP(DT_NODELABEL(lp_uart), tx_pin),
	.rx_io_num = DT_PROP(DT_NODELABEL(lp_uart), rx_pin),
	.rts_io_num = DT_PROP(DT_NODELABEL(lp_uart), rts_pin),
	.cts_io_num = DT_PROP(DT_NODELABEL(lp_uart), cts_pin),
	.baud_rate = DT_PROP(DT_NODELABEL(lp_uart), current_speed),
	.data_bits = DT_PROP_OR(DT_NODELABEL(lp_uart), data_bits, UART_CFG_DATA_BITS_8),
	.parity = DT_ENUM_IDX(DT_NODELABEL(lp_uart), parity),
	.stop_bits = DT_PROP_OR(DT_NODELABEL(lp_uart), stop_bits, UART_CFG_STOP_BITS_1),
	.flow_ctrl = DT_PROP(DT_NODELABEL(lp_uart), hw_flow_control) ? UART_CFG_FLOW_CTRL_RTS_CTS
								     : UART_CFG_FLOW_CTRL_NONE,
	.rx_flow_ctrl_thresh = 0,
	.lp_uart_source_clk = LP_UART_SCLK_DEFAULT,
	.loopback = DT_PROP(DT_NODELABEL(lp_uart), loopback),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_source = DT_IRQ_BY_IDX(DT_NODELABEL(lp_uart), 0, irq),
	.irq_priority = DT_IRQ_BY_IDX(DT_NODELABEL(lp_uart), 0, priority),
	.irq_flags = DT_IRQ_BY_IDX(DT_NODELABEL(lp_uart), 0, flags),
#endif
};

#if defined(CONFIG_SOC_ESP32C6_HPCORE) || defined(CONFIG_SOC_ESP32C5_HPCORE) ||                    \
	defined(CONFIG_SOC_ESP32P4_HPCORE)
#define LP_UART_ESP32_INIT_FUNC lp_uart_esp32_init
#else
#define LP_UART_ESP32_INIT_FUNC NULL
#endif

DEVICE_DT_DEFINE(DT_NODELABEL(lp_uart), LP_UART_ESP32_INIT_FUNC, NULL, &lp_uart_esp32_data,
		 &lp_uart_esp32_cfg, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &lp_uart_esp32_api);
