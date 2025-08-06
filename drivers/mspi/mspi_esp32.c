/*
 * Copyright (c) 2025 Pavel Maloletkov.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_mspi

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <soc.h>

#include <hal/spi_types.h>
#include <hal/spi_ll.h>
#include <soc/spi_struct.h>
#include <soc/spi_reg.h>
#include <soc/spi_periph.h>
#include <esp_clk_tree.h>
#include <esp_memory_utils.h>

#ifdef SOC_GDMA_SUPPORTED
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_esp32.h>
#endif

#include "mspi_esp32.h"

LOG_MODULE_REGISTER(mspi_esp32, CONFIG_MSPI_LOG_LEVEL);

static int mspi_mode_to_esp32(enum mspi_cpp_mode mode)
{
	switch (mode) {
	case MSPI_CPP_MODE_0:
		return 0; /* CPOL=0, CPHA=0 */
	case MSPI_CPP_MODE_1:
		return 1; /* CPOL=0, CPHA=1 */
	case MSPI_CPP_MODE_2:
		return 2; /* CPOL=1, CPHA=0 */
	case MSPI_CPP_MODE_3:
		return 3; /* CPOL=1, CPHA=1 */
	default:
		LOG_ERR("Unsupported mode %d", mode);
		return -ENOTSUP;
	}
}

static int update_spi_line_mode(spi_line_mode_t *line_mode, enum mspi_io_mode io_mode)
{
	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
		line_mode->cmd_lines = 1;
		line_mode->addr_lines = 1;
		line_mode->data_lines = 1;
		break;
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_1_2:
		line_mode->cmd_lines = 1;
		line_mode->addr_lines = 1;
		line_mode->data_lines = 2;
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
		line_mode->cmd_lines = 1;
		line_mode->addr_lines = 2;
		line_mode->data_lines = 2;
		break;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
		line_mode->cmd_lines = 1;
		line_mode->addr_lines = 1;
		line_mode->data_lines = 4;
		break;
	case MSPI_IO_MODE_QUAD_1_4_4:
		line_mode->cmd_lines = 1;
		line_mode->addr_lines = 4;
		line_mode->data_lines = 4;
		break;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_1_8:
		line_mode->cmd_lines = 1;
		line_mode->addr_lines = 1;
		line_mode->data_lines = 8;
		break;
	case MSPI_IO_MODE_OCTAL_1_8_8:
		line_mode->cmd_lines = 1;
		line_mode->addr_lines = 8;
		line_mode->data_lines = 8;
		break;
	default:
		LOG_ERR("Unsupported IO mode %d", io_mode);
		return -ENOTSUP;
	}

	return 0;
}

static int cs_gpio_set(const struct mspi_esp32_data *data, const struct mspi_esp32_config *config,
		       bool active)
{
	if (!config->mspi_config.ce_group) {
		return 0;
	}

	return gpio_pin_set_dt(config->mspi_config.ce_group + data->mspi_dev_config.ce_num,
			       active ? 1 : 0);
}

static esp_err_t update_timing_config(const struct mspi_esp32_config *config,
				      struct mspi_esp32_data *data,
				      spi_hal_timing_conf_t *timing_conf, uint32_t clock_frequency)
{
	if (clock_frequency > config->mspi_config.max_freq) {
		LOG_ERR("Clock frequency %d exceeds maximum %d", clock_frequency,
			config->mspi_config.max_freq);
		return -EINVAL;
	}

	data->clock_frequency = clock_frequency;

	spi_hal_timing_param_t timing_param = {
		.clk_src_hz = data->clock_source_hz,
		.half_duplex = data->hal_dev_config.half_duplex,
		.no_compensate = data->hal_dev_config.no_compensate,
		.expected_freq = clock_frequency,
		.duty_cycle = config->duty_cycle,
		.input_delay_ns = config->input_delay_ns,
		.use_gpio = !config->use_iomux,
	};

	int actual_freq = 0;
	int ret = spi_hal_cal_clock_conf(&timing_param, &actual_freq, timing_conf);

	return ret;
}

#ifdef SOC_GDMA_SUPPORTED
static int spi_esp32_gdma_start(const struct device *dev, uint8_t dir, const uint8_t *buf,
				size_t len)
{
	const struct mspi_esp32_config *config = dev->config;

	struct dma_config dma_cfg = {};
	struct dma_status dma_status = {};
	struct dma_block_config dma_blk = {};
	int err = 0;

	uint8_t dma_channel = (dir == MSPI_RX) ? config->dma_rx_ch : config->dma_tx_ch;

	if (dma_channel == 0xFF) {
		LOG_ERR("DMA channel is not configured in device tree");
		return -EINVAL;
	}

	err = dma_get_status(config->dma_dev, dma_channel, &dma_status);
	if (err) {
		return -EINVAL;
	}

	if (dma_status.busy) {
		LOG_ERR("DMA channel %d is busy", dma_channel);
		return -EBUSY;
	}

	unsigned int key = irq_lock();

	if (dir == MSPI_RX) {
		dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
		dma_blk.dest_address = (uint32_t)buf;
	} else {
		dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
		dma_blk.source_address = (uint32_t)buf;
	}
	dma_cfg.dma_slot = config->dma_host;
	dma_cfg.block_count = 1;
	dma_cfg.head_block = &dma_blk;
	dma_blk.block_size = len;

	err = dma_config(config->dma_dev, dma_channel, &dma_cfg);
	if (err) {
		LOG_ERR("Error configuring DMA (%d)", err);
		goto unlock;
	}

	err = dma_start(config->dma_dev, dma_channel);
	if (err) {
		LOG_ERR("Error starting DMA (%d)", err);
		goto unlock;
	}

unlock:
	irq_unlock(key);

	return err;
}
#endif

static int IRAM_ATTR transfer(const struct device *dev, const struct mspi_xfer *xfer,
			      size_t packet_index)
{
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;
	spi_hal_context_t *hal = &data->hal_ctx;
	spi_hal_dev_config_t *hal_dev = &data->hal_dev_config;
	spi_hal_trans_config_t *trans_config = &data->trans_config;

	uint8_t *data_buffer = NULL;
	size_t dma_len = 0;
	int res = 0;

	if (xfer->num_packet == 0 || !xfer->packets || xfer->num_packet <= packet_index) {
		LOG_ERR("Invalid transfer parameters");
		return -EINVAL;
	}

	/* clean up and prepare SPI hal */
	for (size_t i = 0; i < ARRAY_SIZE(hal->hw->data_buf); ++i) {
#ifdef CONFIG_SOC_SERIES_ESP32C6
		hal->hw->data_buf[i].val = 0;
#else
		hal->hw->data_buf[i] = 0;
#endif
	}

	const struct mspi_xfer_packet *packet = &xfer->packets[packet_index];

	/* Handle DMA buffer allocation if DMA is enabled and we have data to transfer */
	if (config->dma_enabled && packet->num_bytes > 0) {
		dma_len = MIN(packet->num_bytes, CONFIG_MSPI_DMA_MAX_BUFFER_SIZE);

		if (packet->dir == MSPI_TX && packet->data_buf) {
			if (!esp_ptr_dma_capable((uint32_t *)packet->data_buf)) {
				LOG_DBG("Tx buffer not DMA capable");

				data_buffer = k_aligned_alloc(4, ROUND_UP(dma_len, 4));
				if (!data_buffer) {
					LOG_ERR("Error allocating temp buffer Tx");
					return -ENOMEM;
				}

				memcpy(data_buffer, packet->data_buf, dma_len);
			}
		} else if (packet->dir == MSPI_RX && packet->data_buf) {
			if (!esp_ptr_dma_capable((uint32_t *)packet->data_buf) ||
			    ((int)packet->data_buf % 4 != 0) || (dma_len % 4 != 0)) {
				LOG_DBG("Rx buffer not DMA capable");

				/* The rx buffer need to be length of
				 * multiples of 32 bits to avoid heap
				 * corruption.
				 */
				data_buffer = k_calloc(((dma_len << 3) + 31) / 8, sizeof(uint8_t));
				if (!data_buffer) {
					res = -ENOMEM;
					goto cleanup;
				}
			}
		}
	}

	/* keep cs line active until last transmission */
	trans_config->cs_keep_active = xfer->hold_ce;

	/* Handle command phase if present */
	if (xfer->cmd_length > 0) {
		trans_config->cmd = packet->cmd;
		trans_config->cmd_bits = xfer->cmd_length * 8;
	}

	/* Handle address phase if present */
	if (xfer->addr_length > 0) {
		trans_config->addr = packet->address;
		trans_config->addr_bits = xfer->addr_length * 8;
	}

	/* Handle data phase if present */
	if (packet->num_bytes > 0) {
		int bit_len = packet->num_bytes * 8;

		if (config->dma_enabled) {
			/* bit_len needs to be at least one byte long when using DMA */
			bit_len = !bit_len ? 8 : bit_len;
		}

		if (packet->dir == MSPI_TX) {
			trans_config->send_buffer =
				data_buffer ? data_buffer : (uint8_t *)packet->data_buf;
			trans_config->tx_bitlen = bit_len;
		} else {
			trans_config->rcv_buffer = data_buffer ? data_buffer : packet->data_buf;
			trans_config->rx_bitlen = bit_len;
		}
	} else if (config->dma_enabled) {
		trans_config->tx_bitlen = 8;
		trans_config->rx_bitlen = 8;
	}

	/* Configure SPI */
	spi_hal_setup_trans(hal, hal_dev, trans_config);

#if defined(SOC_GDMA_SUPPORTED)
	if (config->dma_enabled) {
		if (trans_config->send_buffer) {
			/* setup DMA channels via DMA driver */
			spi_ll_dma_tx_fifo_reset(hal->hw);
			spi_ll_outfifo_empty_clr(hal->hw);
			spi_ll_dma_tx_enable(hal->hw, 1);

			res = spi_esp32_gdma_start(dev, MSPI_TX, trans_config->send_buffer,
						   dma_len);
			if (res) {
				goto cleanup;
			}
		} else if (trans_config->rcv_buffer) {
			/* setup DMA channels via DMA driver */
			spi_ll_dma_rx_fifo_reset(hal->hw);
			spi_ll_infifo_full_clr(hal->hw);
			spi_ll_dma_rx_enable(hal->hw, 1);

			res = spi_esp32_gdma_start(dev, MSPI_RX, trans_config->rcv_buffer, dma_len);
			if (res) {
				goto cleanup;
			}
		}
	}
#endif

	spi_hal_prepare_data(hal, hal_dev, trans_config);

	/* Send data */
	spi_hal_user_start(hal);

	k_timeout_t timeout = K_MSEC(xfer->timeout ? xfer->timeout : config->transfer_timeout);
	int64_t start_time = k_uptime_get();

	while (!spi_hal_usr_is_done(hal)) {
		if (k_uptime_get() - start_time > timeout.ticks) {
			LOG_ERR("Transfer timeout");
			res = -ETIMEDOUT;
			goto cleanup;
		}
	}

	if (!config->dma_enabled) {
		/* Read data */
		spi_hal_fetch_result(hal);
	}

	if (data_buffer && packet->dir == MSPI_RX && packet->data_buf) {
		memcpy(packet->data_buf, data_buffer, MIN(packet->num_bytes, dma_len));
	}

cleanup:

	k_free(data_buffer);

	return res;
}

static int IRAM_ATTR update_transfer_config(const struct device *dev)
{
	const struct mspi_esp32_config *config = dev->config;
	struct mspi_esp32_data *data = dev->data;
	spi_hal_context_t *hal = &data->hal_ctx;
	spi_hal_dev_config_t *hal_dev = &data->hal_dev_config;

	memset(&data->trans_config, 0, sizeof(data->trans_config));
	data->trans_config.dummy_bits = hal_dev->timing_conf.timing_dummy;

	hal_dev->tx_lsbfirst = false;
	hal_dev->rx_lsbfirst = false;

	if (update_spi_line_mode(&data->trans_config.line_mode, data->mspi_dev_config.io_mode)) {
		return -ENOTSUP;
	}

	spi_hal_setup_device(hal, hal_dev);

	/* Workaround to handle default state of MISO and MOSI lines */
#ifndef CONFIG_SOC_SERIES_ESP32
	spi_dev_t *hw = hal->hw;

	if (config->line_idle_low) {
		hw->ctrl.d_pol = 0;
		hw->ctrl.q_pol = 0;
		hw->ctrl.hold_pol = 0;
		hw->ctrl.wp_pol = 0;
	} else {
		hw->ctrl.d_pol = 1;
		hw->ctrl.q_pol = 1;
		hw->ctrl.hold_pol = 1;
		hw->ctrl.wp_pol = 1;
	}
#endif

	/*
	 * Workaround for ESP32S3 and ESP32Cx SoC's. This dummy transaction is needed
	 * to sync CLK and software controlled CS when SPI is in mode 3
	 */
#if defined(CONFIG_SOC_SERIES_ESP32S3) || defined(CONFIG_SOC_SERIES_ESP32C2) ||                    \
	defined(CONFIG_SOC_SERIES_ESP32C3) || defined(CONFIG_SOC_SERIES_ESP32C6)
	if (config->mspi_config.num_ce_gpios && (hal_dev->mode & MSPI_CPP_MODE_3)) {
		uint8_t src[] = {0x00};

		struct mspi_xfer_packet data_packet = {
			.dir = MSPI_TX,
			.data_buf = (void *)src,
			.num_bytes = sizeof(src),
		};

		struct mspi_xfer xfer = {
			.packets = &data_packet,
			.num_packet = 1,
			.async = false,
			.timeout = 100,
			.priority = 0,
			.hold_ce = 0,
			.cmd_length = 0,
			.addr_length = 0,
		};

		transfer(dev, &xfer, 0);
	}
#endif

	return 0;
}

static int mspi_esp32_dev_config(const struct device *dev, const struct mspi_dev_id *dev_id,
				 const enum mspi_dev_cfg_mask cfg_mask,
				 const struct mspi_dev_cfg *mspi_dev_config)
{
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;
	int ret = 0;

	if ((cfg_mask &
	     (~(MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_CPP | MSPI_DEVICE_CONFIG_CE_NUM |
		MSPI_DEVICE_CONFIG_CE_POL | MSPI_DEVICE_CONFIG_FREQUENCY)))) {
		LOG_ERR("Configuration type not supported.");
		return -ENOTSUP;
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		data->mspi_dev_config.io_mode = mspi_dev_config->io_mode;
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_CPP) {
		data->mspi_dev_config.cpp = mspi_dev_config->cpp;
		data->hal_dev_config.mode = mspi_mode_to_esp32(mspi_dev_config->cpp);
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_CE_NUM) {
		data->mspi_dev_config.ce_num = mspi_dev_config->ce_num;
		data->hal_dev_config.cs_pin_id = mspi_dev_config->ce_num;
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		data->mspi_dev_config.ce_polarity = mspi_dev_config->ce_polarity;
		data->hal_dev_config.positive_cs =
			mspi_dev_config->ce_polarity == MSPI_CE_ACTIVE_HIGH;
	}

	if (cfg_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		ret = update_timing_config(config, data, &data->hal_dev_config.timing_conf,
					   mspi_dev_config->freq);
		if (ret != ESP_OK) {
			LOG_ERR("Failed to calculate timing config: %d", ret);
			return -EINVAL;
		}
	}

	return ret;
}

static int IRAM_ATTR mspi_esp32_transceive(const struct device *dev,
					   const struct mspi_dev_id *dev_id,
					   const struct mspi_xfer *xfer)
{
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;
	int ret = 0;

	if (xfer->num_packet == 0 || !xfer->packets) {
		LOG_ERR("Invalid transfer parameters");
		return -EINVAL;
	}

	if (xfer->async) {
		LOG_ERR("Async mode not supported");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = update_transfer_config(dev);
	if (ret != 0) {
		LOG_ERR("Failed to configure SPI: %d", ret);
		goto unlock;
	}

	/* Assert CS at the beginning of transaction */
	ret = cs_gpio_set(data, config, true);
	if (ret != 0) {
		LOG_ERR("Failed to assert CS: %d", ret);
		goto unlock;
	}

	for (int i = 0; i < xfer->num_packet; i++) {
		ret = transfer(dev, xfer, i);
		if (ret != 0) {
			LOG_ERR("Failed to transfer packet %d: %d", i, ret);
			goto unlock;
		}
	}

	/* Deassert CS at the end of transaction (unless hold_ce is set) */
	if (xfer->hold_ce == false) {
		int cs_ret = cs_gpio_set(data, config, false);

		if (cs_ret != 0) {
			LOG_ERR("Failed to deassert CS: %d", cs_ret);
			if (ret == 0) {
				ret = cs_ret;
			}
		}
	}

unlock:
	if (ret != 0) {
		cs_gpio_set(data, config, false);
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int init_dma(const struct device *dev)
{
	const struct mspi_esp32_config *config = dev->config;
	struct mspi_esp32_data *data = dev->data;
	uint8_t channel_offset = 0;

#ifdef SOC_GDMA_SUPPORTED
	if (config->dma_dev) {
		if (!device_is_ready(config->dma_dev)) {
			LOG_ERR("DMA device is not ready");
			return -ENODEV;
		}
	}
#else
	channel_offset = 1;

	if (clock_control_on(config->clock_dev, (clock_control_subsys_t)config->dma_clk_src)) {
		LOG_ERR("Could not enable DMA clock");
		return -EIO;
	}
#endif

#ifdef CONFIG_SOC_SERIES_ESP32
	/*Connect SPI and DMA*/
	DPORT_SET_PERI_REG_BITS(DPORT_SPI_DMA_CHAN_SEL_REG, 3, config->dma_host + 1,
				((config->dma_host + 1) * 2));
#endif

	data->hal_config.dma_in = (spi_dma_dev_t *)config->spi;
	data->hal_config.dma_out = (spi_dma_dev_t *)config->spi;
	data->hal_config.dma_enabled = true;
	data->hal_config.tx_dma_chan = config->dma_host + channel_offset;
	data->hal_config.rx_dma_chan = config->dma_host + channel_offset;
	data->hal_config.dmadesc_n = 1;
	data->hal_config.dmadesc_rx = &data->dma_desc_rx;
	data->hal_config.dmadesc_tx = &data->dma_desc_tx;

	if (data->hal_config.dmadesc_tx == NULL || data->hal_config.dmadesc_rx == NULL) {
		k_free(data->hal_config.dmadesc_tx);
		k_free(data->hal_config.dmadesc_rx);
		return -ENOMEM;
	}

	spi_hal_init(&data->hal_ctx, config->dma_host + 1, &data->hal_config);

	return 0;
}

static inline int cs_configure(const struct mspi_cfg *mspi_config)
{
	int ret = 0;

	for (int i = 0; i < mspi_config->num_ce_gpios; ++i) {
		const struct gpio_dt_spec *cs_gpio = mspi_config->ce_group + i;

		if (!device_is_ready(cs_gpio->port)) {
			LOG_ERR("CS GPIO port %s pin %d is not ready", cs_gpio->port->name,
				cs_gpio->pin);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(cs_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

static int mspi_esp32_config(const struct mspi_dt_spec *spec)
{
	const struct device *dev = spec->bus;
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;
	int ret = 0;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to configure SPI pins");
		return ret;
	}

	if (!config->clock_dev) {
		LOG_ERR("Clock device not specified");
		return -EINVAL;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	/* Enable SPI peripheral */
	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Error enabling SPI clock: %d", ret);
		return ret;
	}

	/* Get clock source frequency */
	ret = esp_clk_tree_src_get_freq_hz(config->clock_source,
					   ESP_CLK_TREE_SRC_FREQ_PRECISION_APPROX,
					   &data->clock_source_hz);
	if (ret != ESP_OK) {
		LOG_ERR("Could not get clock source frequency (%d)", ret);
		return -EINVAL;
	}

	data->mspi_dev_config.freq = config->mspi_config.max_freq;
	ret = update_timing_config(config, data, &data->hal_dev_config.timing_conf,
				   data->mspi_dev_config.freq);
	if (ret != ESP_OK) {
		LOG_ERR("Failed to calculate timing config: %d", ret);
		return -EINVAL;
	}

	spi_ll_master_init(config->spi);

	if (config->dma_enabled) {
		ret = init_dma(dev);
		if (ret != 0) {
			LOG_ERR("Failed to initialize SPI DMA: %d", ret);
			return ret;
		}
	}

	ret = cs_configure(&config->mspi_config);
	if (ret < 0) {
		LOG_ERR("Failed to configure CS GPIOs: %d", ret);
		return ret;
	}

	LOG_INF("Configured successfully");

	return ret;
}

static int mspi_esp32_init(const struct device *dev)
{
	struct mspi_esp32_data *data = dev->data;
	const struct mspi_esp32_config *config = dev->config;
	int ret = 0;

	k_mutex_init(&data->lock);

	memset(&data->hal_dev_config, 0, sizeof(data->hal_dev_config));

	data->hal_dev_config.cs_setup = 0;
	data->hal_dev_config.cs_hold = 0;
	data->hal_dev_config.half_duplex = (config->mspi_config.duplex == MSPI_HALF_DUPLEX);
	data->hal_dev_config.tx_lsbfirst = false;
	data->hal_dev_config.rx_lsbfirst = false;
	data->hal_dev_config.no_compensate = false;

	data->clock_frequency = config->mspi_config.max_freq;

	const struct mspi_dt_spec spec = {
		.bus = dev,
		.config = config->mspi_config,
	};

	ret = mspi_esp32_config(&spec);
	if (ret != 0) {
		LOG_ERR("Failed to configure MSPI: %d", ret);
		return ret;
	}

	LOG_INF("ESP32 MSPI driver initialized (peripheral %d)", config->peripheral_id);

	return ret;
}

/* Device driver API */
static DEVICE_API(mspi, mspi_esp32_api) = {
	.config = mspi_esp32_config,
	.dev_config = mspi_esp32_dev_config,
	.transceive = mspi_esp32_transceive,
};

#if defined(SOC_GDMA_SUPPORTED)
#define SPI_DMA_CFG(inst)                                                                          \
	.dma_dev = ESP32_DT_INST_DMA_CTLR(inst, tx),                                               \
	.dma_tx_ch = ESP32_DT_INST_DMA_CELL(inst, tx, channel),                                    \
	.dma_rx_ch = ESP32_DT_INST_DMA_CELL(inst, rx, channel)
#else
#define SPI_DMA_CFG(inst) .dma_clk_src = DT_INST_PROP(inst, dma_clk)
#endif /* defined(SOC_GDMA_SUPPORTED) */

#define MSPI_CONFIG(inst)                                                                          \
	{                                                                                          \
		.channel_num = DT_INST_PROP(inst, peripheral_id),                                  \
		.op_mode = MSPI_OP_MODE_CONTROLLER,                                                \
		.duplex = DT_ENUM_IDX_OR(DT_DRV_INST(inst), duplex, MSPI_FULL_DUPLEX),             \
		.max_freq = DT_INST_PROP(inst, clock_frequency),                                   \
		.dqs_support = false,                                                              \
		.num_periph = DT_INST_CHILD_NUM(inst),                                             \
		.sw_multi_periph = DT_INST_PROP(inst, software_multiperipheral),                   \
		.re_init = false,                                                                  \
		.ce_group = (struct gpio_dt_spec *)ce_gpios##inst,                                 \
		.num_ce_gpios = ARRAY_SIZE(ce_gpios##inst),                                        \
	}

/* Device instantiation macro */
#define ESP32_MSPI_INIT(inst)                                                                      \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct mspi_esp32_data mspi_esp32_data_##inst = {                                   \
		.hal_ctx =                                                                         \
			{                                                                          \
				.hw = (spi_dev_t *)DT_INST_REG_ADDR(inst),                         \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct gpio_dt_spec ce_gpios##inst[] = MSPI_CE_GPIOS_DT_SPEC_INST_GET(inst);        \
	static const struct mspi_esp32_config mspi_esp32_cfg_##inst = {                            \
		.spi = (spi_dev_t *)DT_INST_REG_ADDR(inst),                                        \
		.peripheral_id = DT_INST_PROP(inst, peripheral_id),                                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.mspi_config = MSPI_CONFIG(inst),                                                  \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, offset),         \
		.clock_source = DT_ENUM_IDX_OR(DT_DRV_INST(inst), clk_src, SPI_CLK_SRC_DEFAULT),   \
		.dma_enabled = DT_INST_PROP_OR(inst, dma_enabled, false),                          \
		.dma_host = DT_INST_PROP(inst, dma_host),                                          \
		SPI_DMA_CFG(inst),                                                                 \
		.line_idle_low = DT_INST_PROP_OR(inst, line_idle_low, false),                      \
		.use_iomux = DT_INST_PROP_OR(inst, use_iomux, false),                              \
		.duty_cycle = DT_INST_PROP(inst, duty_cycle),                                      \
		.transfer_timeout = DT_INST_PROP(inst, transfer_timeout),                          \
		.input_delay_ns = 0,                                                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mspi_esp32_init, NULL, &mspi_esp32_data_##inst,                \
			      &mspi_esp32_cfg_##inst, POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,      \
			      &mspi_esp32_api);

/* Instantiate all MSPI devices */
DT_INST_FOREACH_STATUS_OKAY(ESP32_MSPI_INIT)
