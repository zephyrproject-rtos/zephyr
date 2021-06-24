/*
 * Copyright (c) 2020 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_spi

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <hal/spi_hal.h>
#include <esp_attr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(esp32_spi, CONFIG_SPI_LOG_LEVEL);

#include <soc.h>
#include <drivers/spi.h>
#include <drivers/interrupt_controller/intc_esp32.h>
#include <drivers/gpio/gpio_esp32.h>
#include <drivers/clock_control.h>
#include "spi_context.h"
#include "spi_esp32_spim.h"

/* pins, signals and interrupts shall be placed into dts */
#define MISO_IDX_2 HSPIQ_IN_IDX
#define MISO_IDX_3 VSPIQ_IN_IDX
#define MOSI_IDX_2 HSPID_OUT_IDX
#define MOSI_IDX_3 VSPID_OUT_IDX
#define SCLK_IDX_2 HSPICLK_OUT_IDX
#define SCLK_IDX_3 VSPICLK_OUT_IDX
#define CSEL_IDX_2 HSPICS0_OUT_IDX
#define CSEL_IDX_3 VSPICS0_OUT_IDX

#define INST_2_ESPRESSIF_ESP32_SPI_IRQ_0 13
#define INST_3_ESPRESSIF_ESP32_SPI_IRQ_0 17

static bool spi_esp32_transfer_ongoing(struct spi_esp32_data *data)
{
	return spi_context_tx_on(&data->ctx) || spi_context_rx_on(&data->ctx);
}

static inline void spi_esp32_complete(struct spi_esp32_data *data,
				      spi_dev_t *spi, int status)
{
#ifdef CONFIG_SPI_ESP32_INTERRUPT
	spi_ll_disable_int(spi);
	spi_ll_clear_int_stat(spi);
#endif

	spi_context_cs_control(&data->ctx, false);

#ifdef CONFIG_SPI_ESP32_INTERRUPT
	spi_context_complete(&data->ctx, status);
#endif

}

static int IRAM_ATTR spi_esp32_transfer(const struct device *dev)
{
	struct spi_esp32_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	spi_hal_context_t *hal = &data->hal;
	spi_hal_dev_config_t *hal_dev = &data->dev_config;
	spi_hal_trans_config_t *hal_trans = &data->trans_config;
	size_t chunk_len = spi_context_max_continuous_chunk(&data->ctx);

	/* clean up and prepare SPI hal */
	memset((uint32_t *) hal->hw->data_buf, 0, sizeof(hal->hw->data_buf));
	hal_trans->send_buffer = (uint8_t *) ctx->tx_buf;
	hal_trans->rcv_buffer = ctx->rx_buf;
	hal_trans->tx_bitlen = chunk_len << 3;
	hal_trans->rx_bitlen = chunk_len << 3;

	/* configure SPI */
	spi_hal_setup_trans(hal, hal_dev, hal_trans);
	spi_hal_prepare_data(hal, hal_dev, hal_trans);

	/* send data */
	spi_hal_user_start(hal);
	spi_context_update_tx(&data->ctx, data->dfs, chunk_len);

	while (!spi_hal_usr_is_done(hal)) {
		/* nop */
	}

	/* read data */
	spi_hal_fetch_result(hal);
	spi_context_update_rx(&data->ctx, data->dfs, chunk_len);

	return 0;
}

#ifdef CONFIG_SPI_ESP32_INTERRUPT
static void IRAM_ATTR spi_esp32_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_esp32_data *data = dev->data;

	do {
		spi_esp32_transfer(dev);
	} while (spi_esp32_transfer_ongoing(data));

	spi_esp32_complete(data, cfg->spi, 0);
}
#endif

static int spi_esp32_init(const struct device *dev)
{
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_esp32_data *data = dev->data;

	if (!cfg->clock_dev) {
		return -EINVAL;
	}

#ifdef CONFIG_SPI_ESP32_INTERRUPT
	data->irq_line = esp_intr_alloc(cfg->irq_source, 0, spi_esp32_isr, (void *)dev, NULL);
#endif

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static int spi_esp32_configure_pin(gpio_pin_t pin, int pin_sig,
				   gpio_flags_t pin_mode)
{
	const char *device_name = gpio_esp32_get_gpio_for_pin(pin);
	const struct device *gpio;
	int ret;

	if (!device_name) {
		LOG_ERR("Could not find GPIO node on devicetree");
		return -EINVAL;
	}

	gpio = device_get_binding(device_name);
	if (!gpio) {
		LOG_ERR("Could not bind to GPIO device");
		return -EIO;
	}

	ret = gpio_pin_configure(gpio, pin, pin_mode);
	if (ret < 0) {
		LOG_ERR("SPI pin configuration failed");
		return ret;
	}

	if (pin_mode == GPIO_INPUT) {
		esp32_rom_gpio_matrix_in(pin, pin_sig, false);
	} else {
		esp32_rom_gpio_matrix_out(pin, pin_sig, false, false);
	}

	return 0;
}

static inline spi_ll_io_mode_t spi_esp32_get_io_mode(uint16_t operation)
{
	switch (operation & SPI_LINES_MASK) {
	case SPI_LINES_SINGLE:
		return SPI_LL_IO_MODE_NORMAL;
	case SPI_LINES_DUAL:
		return SPI_LL_IO_MODE_DUAL;
	case SPI_LINES_OCTAL:
		return SPI_LL_IO_MODE_QIO;
	case SPI_LINES_QUAD:
		return SPI_LL_IO_MODE_QUAD;
	default:
		return SPI_LL_IO_MODE_NORMAL;
	}
}

static int IRAM_ATTR spi_esp32_configure(const struct device *dev,
					 const struct spi_config *spi_cfg)
{
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_esp32_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	spi_hal_context_t *hal = &data->hal;
	spi_hal_dev_config_t *hal_dev = &data->dev_config;
	int freq;

	if (spi_context_configured(ctx, spi_cfg)) {
		return 0;
	}

	/* enables SPI peripheral */
	if (clock_control_on(cfg->clock_dev, cfg->clock_subsys)) {
		LOG_ERR("Could not enable SPI clock");
		return -EIO;
	}

	ctx->config = spi_cfg;

	if (spi_cfg->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("Slave mode not supported");
		return -ENOTSUP;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -ENOTSUP;
	}

	spi_esp32_configure_pin(cfg->pins.miso,
				cfg->signals.miso_s,
				GPIO_INPUT);

	spi_esp32_configure_pin(cfg->pins.mosi,
				cfg->signals.mosi_s,
				GPIO_OUTPUT_LOW);

	spi_esp32_configure_pin(cfg->pins.sclk,
				cfg->signals.sclk_s,
				GPIO_OUTPUT);

	if (ctx->config->cs == NULL) {
		hal_dev->cs_setup = 1;
		hal_dev->cs_hold = 1;
		hal_dev->cs_pin_id = 0;

		spi_esp32_configure_pin(cfg->pins.csel,
					cfg->signals.csel_s,
					GPIO_OUTPUT | GPIO_ACTIVE_LOW);
	}

	spi_context_cs_configure(&data->ctx);

	/* input parameters to calculate timing configuration */
	spi_hal_timing_param_t timing_param = {
		.half_duplex = hal_dev->half_duplex,
		.no_compensate = hal_dev->no_compensate,
		.clock_speed_hz = cfg->frequency,
		.duty_cycle = cfg->duty_cycle == 0 ? 128 : cfg->duty_cycle,
		.input_delay_ns = cfg->input_delay_ns,
		.use_gpio = true
	};

	spi_hal_cal_clock_conf(&timing_param, &freq, &hal_dev->timing_conf);

	data->trans_config.dummy_bits = hal_dev->timing_conf.timing_dummy;

	hal_dev->tx_lsbfirst = spi_cfg->operation & SPI_TRANSFER_LSB ? 1 : 0;
	hal_dev->rx_lsbfirst = spi_cfg->operation & SPI_TRANSFER_LSB ? 1 : 0;

	data->trans_config.io_mode = spi_esp32_get_io_mode(spi_cfg->operation);

	/* SPI mode */
	hal_dev->mode = 0;
	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPOL) {
		hal_dev->mode = BIT(0);
	}

	if (SPI_MODE_GET(spi_cfg->operation) & SPI_MODE_CPHA) {
		hal_dev->mode |= BIT(1);
	}

	spi_hal_setup_device(hal, hal_dev);

	return 0;
}

static inline uint8_t spi_esp32_get_frame_size(const struct spi_config *spi_cfg)
{
	uint8_t dfs = SPI_WORD_SIZE_GET(spi_cfg->operation);

	dfs /= 8;

	if ((dfs == 0) || (dfs > 4)) {
		LOG_WRN("Unsupported dfs, 1-byte size will be used");
		dfs = 1;
	}

	return dfs;
}

static int transceive(const struct device *dev,
		      const struct spi_config *spi_cfg,
		      const struct spi_buf_set *tx_bufs,
		      const struct spi_buf_set *rx_bufs, bool asynchronous,
		      struct k_poll_signal *signal)
{
	const struct spi_esp32_config *cfg = dev->config;
	struct spi_esp32_data *data = dev->data;
	int ret;

	if (!tx_bufs && !rx_bufs) {
		return 0;
	}

#ifndef CONFIG_SPI_ESP32_INTERRUPT
	if (asynchronous) {
		return -ENOTSUP;
	}
#endif

	spi_context_lock(&data->ctx, asynchronous, signal, spi_cfg);

	ret = spi_esp32_configure(dev, spi_cfg);
	if (ret) {
		goto done;
	}

	data->dfs = spi_esp32_get_frame_size(spi_cfg);

	spi_context_buffers_setup(&data->ctx, tx_bufs, rx_bufs, data->dfs);

	spi_context_cs_control(&data->ctx, true);

#ifdef CONFIG_SPI_ESP32_INTERRUPT
	spi_ll_enable_int(cfg->spi);
	spi_ll_set_int_stat(cfg->spi);
#else

	do {
		spi_esp32_transfer(dev);
	} while (spi_esp32_transfer_ongoing(data));

	spi_esp32_complete(data, cfg->spi, 0);

#endif  /* CONFIG_SPI_ESP32_INTERRUPT */

done:
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int spi_esp32_transceive(const struct device *dev,
				const struct spi_config *spi_cfg,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int spi_esp32_transceive_async(const struct device *dev,
				      const struct spi_config *spi_cfg,
				      const struct spi_buf_set *tx_bufs,
				      const struct spi_buf_set *rx_bufs,
				      struct k_poll_signal *async)
{
	return transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, async);
}
#endif /* CONFIG_SPI_ASYNC */

static int spi_esp32_release(const struct device *dev,
			     const struct spi_config *config)
{
	struct spi_esp32_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct spi_driver_api spi_api = {
	.transceive = spi_esp32_transceive,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = spi_esp32_transceive_async,
#endif
	.release = spi_esp32_release
};

#define ESP32_SPI_INIT(idx)	\
										\
	static struct spi_esp32_data spi_data_##idx = {	\
		SPI_CONTEXT_INIT_LOCK(spi_data_##idx, ctx),	\
		SPI_CONTEXT_INIT_SYNC(spi_data_##idx, ctx),	\
		.hal = {	\
			.hw = (spi_dev_t *)DT_REG_ADDR(DT_NODELABEL(spi##idx)),	\
		},	\
		.dev_config = {	\
			.half_duplex = DT_PROP(DT_NODELABEL(spi##idx), half_duplex),	\
			.as_cs = DT_PROP(DT_NODELABEL(spi##idx), clk_as_cs),	\
			.positive_cs = DT_PROP(DT_NODELABEL(spi##idx), positive_cs),	\
			.no_compensate = DT_PROP(DT_NODELABEL(spi##idx), dummy_comp),	\
			.sio = DT_PROP(DT_NODELABEL(spi##idx), sio)	\
		}	\
	};	\
		\
	static const struct spi_esp32_config spi_config_##idx = {	\
		.spi = (spi_dev_t *)DT_REG_ADDR(DT_NODELABEL(spi##idx)),	\
			\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_NODELABEL(spi##idx))),	\
		.frequency = SPI_MASTER_FREQ_8M,\
		.duty_cycle = 0, \
		.input_delay_ns = 0, \
		.irq_source = DT_IRQN(DT_NODELABEL(spi##idx)), \
		.signals = {	\
			.miso_s = MISO_IDX_##idx,	\
			.mosi_s = MOSI_IDX_##idx,	\
			.sclk_s = SCLK_IDX_##idx,	\
			.csel_s = CSEL_IDX_##idx	\
		},	\
			\
		.pins = {	\
			  .miso = DT_PROP(DT_NODELABEL(spi##idx), miso_pin),	\
			  .mosi = DT_PROP(DT_NODELABEL(spi##idx), mosi_pin),	\
			  .sclk = DT_PROP(DT_NODELABEL(spi##idx), sclk_pin),	\
			  .csel = DT_PROP(DT_NODELABEL(spi##idx), csel_pin)	\
		},	\
			\
		.clock_subsys =	\
			(clock_control_subsys_t)DT_CLOCKS_CELL(	\
				DT_NODELABEL(spi##idx), offset),	\
					\
	};	\
		\
	DEVICE_DT_DEFINE(DT_NODELABEL(spi##idx), &spi_esp32_init,	\
			      device_pm_control_no, &spi_data_##idx,	\
			      &spi_config_##idx, POST_KERNEL,	\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &spi_api);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi2), okay)
ESP32_SPI_INIT(2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi3), okay)
ESP32_SPI_INIT(3);
#endif
