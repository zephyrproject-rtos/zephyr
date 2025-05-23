/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lpuart

#include <zephyr/drivers/uart.h>
#include <zephyr/dt-bindings/clock/esp32c6_clock.h>

#include <hal/uart_hal.h>
#if defined(CONFIG_SOC_ESP32C6_HPCORE)
#include <hal/uart_ll.h>
#include <hal/clk_tree_ll.h>
#include <hal/clk_tree_hal.h>
#include <hal/rtc_io_hal.h>
#include <soc/uart_pins.h>
#include <soc/rtc_io_periph.h>
#include <esp_private/esp_clk_tree_common.h>
#include <ulp_lp_core_uart.h>
#endif

#define ESP_LP_UART_TX_IDLE_NUM_DEFAULT (0U)

struct lp_uart_esp32_data {
	uart_hal_context_t hal;
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

#if defined(CONFIG_SOC_ESP32C6_HPCORE)

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

	/* Get LP UART source clock frequency */
	switch (clk_ll_rtc_fast_get_src()) {
	case SOC_RTC_FAST_CLK_SRC_XTAL_DIV:
#if CONFIG_SOC_SERIES_ESP32 || CONFIG_SOC_SERIES_ESP32S2 /* SOC_RTC_FAST_CLK_SRC_XTAL_D4 */
		sclk_freq = clk_hal_xtal_get_freq_mhz() * MHZ(1) >> 2;
#else /* SOC_RTC_FAST_CLK_SRC_XTAL_D2 */
		sclk_freq = clk_hal_xtal_get_freq_mhz() * MHZ(1) >> 1;
#endif
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
	lp_uart_ll_set_source_clk(data->hal.dev, cfg->lp_uart_source_clk);
	lp_uart_ll_sclk_enable(0);

	/* Initialize LP UART HAL with default parameters */
	uart_hal_init(&data->hal, LP_UART_NUM_0);

	/* Set protocol parameters from the configuration */
	lp_uart_ll_set_baudrate(data->hal.dev, cfg->baud_rate, sclk_freq);
	uart_hal_set_parity(&data->hal, cfg->parity);
	uart_hal_set_data_bit_num(&data->hal, cfg->data_bits);
	uart_hal_set_stop_bits(&data->hal, cfg->stop_bits);
	uart_hal_set_tx_idle_num(&data->hal, ESP_LP_UART_TX_IDLE_NUM_DEFAULT);
	uart_hal_set_hw_flow_ctrl(&data->hal, cfg->flow_ctrl, cfg->rx_flow_ctrl_thresh);

	/* Reset Tx/Rx FIFOs */
	uart_hal_rxfifo_rst(&data->hal);
	uart_hal_txfifo_rst(&data->hal);

	return 0;
}

static void lp_uart_esp32_config_io(int pin, int direction, int func)
{
	int rtc_io_num = rtc_io_num_map[pin];

	rtcio_hal_function_select(rtc_io_num, RTCIO_FUNC_RTC);
	rtcio_hal_set_direction(rtc_io_num, direction);
	rtcio_hal_iomux_func_sel(rtc_io_num, func);
}

static void lp_uart_esp32_set_pin(const struct device *dev)
{
	const struct lp_uart_esp32_config *const cfg = dev->config;

	/* Configure Tx Pin */
	lp_uart_esp32_config_io(cfg->tx_io_num, RTC_GPIO_MODE_OUTPUT_ONLY, LP_U0TXD_MUX_FUNC);

	/* Configure Rx Pin */
	lp_uart_esp32_config_io(cfg->rx_io_num, RTC_GPIO_MODE_INPUT_ONLY, LP_U0RXD_GPIO_NUM);

	/* Configure RTS Pin */
	lp_uart_esp32_config_io(cfg->rts_io_num, RTC_GPIO_MODE_OUTPUT_ONLY, LP_U0RTS_MUX_FUNC);

	/* Configure CTS Pin */
	lp_uart_esp32_config_io(cfg->cts_io_num, RTC_GPIO_MODE_INPUT_ONLY, LP_U0CTS_MUX_FUNC);
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
	return 0;
}

#endif /* CONFIG_SOC_ESP32C6_HPCORE */

static DEVICE_API(uart, lp_uart_esp32_api) = {
	.poll_in = lp_uart_esp32_poll_in,
	.poll_out = lp_uart_esp32_poll_out,
};

static struct lp_uart_esp32_data lp_uart_esp32_data = {
	.hal =	{
			.dev = (uart_dev_t *)DT_REG_ADDR(DT_NODELABEL(lp_uart)),
		},
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
	.flow_ctrl = DT_PROP_OR(DT_NODELABEL(lp_uart), flow_ctrl, UART_CFG_FLOW_CTRL_NONE),
	.rx_flow_ctrl_thresh = 0,
	.lp_uart_source_clk = LP_UART_SCLK_DEFAULT,
};

#if defined(CONFIG_SOC_ESP32C6_HPCORE)
#define LP_UART_ESP32_INIT_FUNC lp_uart_esp32_init
#else
#define LP_UART_ESP32_INIT_FUNC NULL
#endif

DEVICE_DT_DEFINE(DT_NODELABEL(lp_uart), LP_UART_ESP32_INIT_FUNC, NULL, &lp_uart_esp32_data,
		 &lp_uart_esp32_cfg, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &lp_uart_esp32_api);
