/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * CS47L63 audio codec driver for Zephyr.
 *
 * This driver implements the Zephyr audio codec API for the Cirrus Logic
 * CS47L63 Low-Power Audio DSP, communicating over SPI.  It configures the
 * codec for I2S slave playback through the headphone output (OUT1L) and
 * is tailored for the nRF5340 Audio DK board.
 *
 * Register sequences are derived from the Cirrus Logic mcu-drivers
 * (Apache-2.0) and the LENS-TUGraz grptlk project (Nordic-5-Clause /
 * Apache-2.0).
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/audio/codec.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "cs47l63.h"

LOG_MODULE_REGISTER(cirrus_cs47l63, CONFIG_AUDIO_CODEC_LOG_LEVEL);

#define DT_DRV_COMPAT cirrus_cs47l63

/* ------------------------------------------------------------------ */
/* Driver data / config structures                                     */
/* ------------------------------------------------------------------ */

struct cs47l63_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec irq_gpio;
	struct gpio_dt_spec reset_gpio;
};

struct cs47l63_data {
	bool configured;
};

/* ------------------------------------------------------------------ */
/* Low-level SPI register access                                       */
/* ------------------------------------------------------------------ */

static int cs47l63_write_reg(const struct spi_dt_spec *spi, uint32_t reg, uint32_t val)
{
	/*
	 * CS47L63 SPI write frame:
	 *   [4-byte address, MSB first] [4-byte padding] [4-byte data, MSB first]
	 */
	uint8_t addr_buf[4];
	uint8_t pad_buf[CS47L63_SPI_PAD_LEN] = {0};
	uint8_t data_buf[4];

	/* Address – MSB first, bit 31 clear for write */
	addr_buf[0] = (reg >> 24) & 0x7F;
	addr_buf[1] = (reg >> 16) & 0xFF;
	addr_buf[2] = (reg >> 8) & 0xFF;
	addr_buf[3] = reg & 0xFF;

	/* Data – MSB first */
	data_buf[0] = (val >> 24) & 0xFF;
	data_buf[1] = (val >> 16) & 0xFF;
	data_buf[2] = (val >> 8) & 0xFF;
	data_buf[3] = val & 0xFF;

	struct spi_buf tx_bufs[] = {
		{.buf = addr_buf, .len = sizeof(addr_buf)},
		{.buf = pad_buf, .len = sizeof(pad_buf)},
		{.buf = data_buf, .len = sizeof(data_buf)},
	};
	const struct spi_buf_set tx = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};

	return spi_write_dt(spi, &tx);
}

static int cs47l63_read_reg(const struct spi_dt_spec *spi, uint32_t reg, uint32_t *val)
{
	/*
	 * CS47L63 SPI read frame (full-duplex transceive):
	 *   TX: [4-byte address with bit 31 set] [4-byte pad] [4-byte dummy]
	 *   RX: [4-byte ignore]                  [4-byte pad] [4-byte data]
	 */
	uint8_t addr_buf[4];
	uint8_t pad_buf[CS47L63_SPI_PAD_LEN] = {0};
	uint8_t data_buf[4] = {0};

	/* Address – MSB first, bit 31 SET for read */
	addr_buf[0] = ((reg >> 24) & 0x7F) | 0x80;
	addr_buf[1] = (reg >> 16) & 0xFF;
	addr_buf[2] = (reg >> 8) & 0xFF;
	addr_buf[3] = reg & 0xFF;

	struct spi_buf tx_bufs[] = {
		{.buf = addr_buf, .len = sizeof(addr_buf)},
		{.buf = pad_buf, .len = sizeof(pad_buf)},
		{.buf = data_buf, .len = sizeof(data_buf)},
	};
	const struct spi_buf_set tx = {.buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs)};

	/* Re-use the same buffers for RX (in-place transceive). */
	uint8_t rx_addr[4];
	uint8_t rx_pad[CS47L63_SPI_PAD_LEN];
	uint8_t rx_data[4] = {0};

	struct spi_buf rx_bufs[] = {
		{.buf = rx_addr, .len = sizeof(rx_addr)},
		{.buf = rx_pad, .len = sizeof(rx_pad)},
		{.buf = rx_data, .len = sizeof(rx_data)},
	};
	const struct spi_buf_set rx = {.buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs)};

	int ret = spi_transceive_dt(spi, &tx, &rx);

	if (ret == 0) {
		*val = ((uint32_t)rx_data[0] << 24) |
		       ((uint32_t)rx_data[1] << 16) |
		       ((uint32_t)rx_data[2] << 8) |
		       (uint32_t)rx_data[3];
	}

	return ret;
}

/* ------------------------------------------------------------------ */
/* Register sequence helpers                                           */
/* ------------------------------------------------------------------ */

struct reg_val_pair {
	uint32_t reg;
	uint32_t val;
};

/*
 * Magic address used to encode a busy-wait delay inside a register
 * sequence.  Register 0x0001 is read-only in the CS47L63, so writing
 * to it is harmless if the sentinel is ever missed.
 */
#define REG_DELAY 0x0001U

static int cs47l63_write_sequence(const struct spi_dt_spec *spi,
				  const struct reg_val_pair *seq, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		if (seq[i].reg == REG_DELAY) {
			k_busy_wait(seq[i].val);
			continue;
		}
		int ret = cs47l63_write_reg(spi, seq[i].reg, seq[i].val);

		if (ret != 0) {
			LOG_ERR("Reg write failed: addr=0x%06x val=0x%08x err=%d",
				seq[i].reg, seq[i].val, ret);
			return ret;
		}
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/* Register configuration sequences                                    */
/* ------------------------------------------------------------------ */

static const struct reg_val_pair seq_soft_reset[] = {
	{CS47L63_SFT_RESET, CS47L63_SOFT_RESET_VAL},
	{REG_DELAY, 3000}, /* 3 ms post-reset settling */
};

static const struct reg_val_pair seq_clock_config[] = {
	{CS47L63_SAMPLE_RATE3, 0x0012},
	{CS47L63_SAMPLE_RATE2, 0x0002},
	{CS47L63_SAMPLE_RATE1, 0x0003},
	{CS47L63_SYSTEM_CLOCK1, 0x034C},
	{CS47L63_ASYNC_CLOCK1, 0x034C},
	{CS47L63_FLL1_CONTROL2, 0x88200008},
	{CS47L63_FLL1_CONTROL3, 0x10000},
	{CS47L63_FLL1_GPIO_CLOCK, 0x0005},
	{CS47L63_FLL1_CONTROL1, 0x0001},
};

static const struct reg_val_pair seq_gpio_config[] = {
	{CS47L63_GPIO6_CTRL1, 0x61000001},
	{CS47L63_GPIO7_CTRL1, 0x61000001},
	{CS47L63_GPIO8_CTRL1, 0x61000001},
	/* Enable codec LED on GPIO10 */
	{CS47L63_GPIO10_CTRL1, 0x41008001},
};

static const struct reg_val_pair seq_asp1_enable[] = {
	/* Enable ASP1 GPIOs */
	{CS47L63_GPIO1_CTRL1, 0x61000000},
	{CS47L63_GPIO2_CTRL1, 0xE1000000},
	{CS47L63_GPIO3_CTRL1, 0xE1000000},
	{CS47L63_GPIO4_CTRL1, 0xE1000000},
	{CS47L63_GPIO5_CTRL1, 0x61000001},
	/* Sample rate – default to 16 kHz; reconfigured in configure() */
	{CS47L63_SAMPLE_RATE1, 0x00000012},
	/* Disable unused sample rates */
	{CS47L63_SAMPLE_RATE2, 0},
	{CS47L63_SAMPLE_RATE3, 0},
	{CS47L63_SAMPLE_RATE4, 0},
	/* ASP1 slave mode, 16-bit per channel */
	{CS47L63_ASP1_CONTROL2, 0x10100200},
	{CS47L63_ASP1_CONTROL3, 0x0000},
	{CS47L63_ASP1_DATA_CONTROL1, 0x0020},
	{CS47L63_ASP1_DATA_CONTROL5, 0x0020},
	{CS47L63_ASP1_ENABLES1, 0x30003},
};

static const struct reg_val_pair seq_output_enable[] = {
	{CS47L63_OUTPUT_ENABLE_1, 0x0002},
	/* Route ASP1 RX1 + RX2 → OUT1L */
	{CS47L63_OUT1L_INPUT1, 0x800020},
	{CS47L63_OUT1L_INPUT2, 0x800021},
};

static const struct reg_val_pair seq_fll_toggle[] = {
	{CS47L63_FLL1_CONTROL1, 0x0000},
	{REG_DELAY, 1000},
	{CS47L63_FLL1_CONTROL1, 0x0001},
};

/* ------------------------------------------------------------------ */
/* Zephyr audio codec API implementation                               */
/* ------------------------------------------------------------------ */

static int cs47l63_codec_configure(const struct device *dev,
				   struct audio_codec_cfg *cfg)
{
	const struct cs47l63_config *drv_cfg = dev->config;
	struct cs47l63_data *data = dev->data;
	int ret;

	if (data->configured) {
		LOG_WRN("Codec already configured");
		return -EALREADY;
	}

	/* Soft-reset */
	ret = cs47l63_write_sequence(&drv_cfg->spi, seq_soft_reset,
				     ARRAY_SIZE(seq_soft_reset));
	if (ret) {
		return ret;
	}

	/* Wait for boot - give the codec additional time after reset */
	k_msleep(5);

	/* Clock & FLL setup */
	ret = cs47l63_write_sequence(&drv_cfg->spi, seq_clock_config,
				     ARRAY_SIZE(seq_clock_config));
	if (ret) {
		return ret;
	}

	/* GPIO setup */
	ret = cs47l63_write_sequence(&drv_cfg->spi, seq_gpio_config,
				     ARRAY_SIZE(seq_gpio_config));
	if (ret) {
		return ret;
	}

	/* ASP1 (I2S) setup */
	ret = cs47l63_write_sequence(&drv_cfg->spi, seq_asp1_enable,
				     ARRAY_SIZE(seq_asp1_enable));
	if (ret) {
		return ret;
	}

	/* Output path */
	ret = cs47l63_write_sequence(&drv_cfg->spi, seq_output_enable,
				     ARRAY_SIZE(seq_output_enable));
	if (ret) {
		return ret;
	}

	/* Set default output volume */
	ret = cs47l63_write_reg(&drv_cfg->spi, CS47L63_OUT1L_VOLUME_1,
				CS47L63_OUT_VOLUME_DEFAULT | CS47L63_VOLUME_UPDATE_BIT);
	if (ret) {
		return ret;
	}

	data->configured = true;
	LOG_INF("CS47L63 configured");
	return 0;
}

static int cs47l63_codec_start(const struct device *dev, audio_dai_dir_t dir)
{
	const struct cs47l63_config *drv_cfg = dev->config;

	if (dir != AUDIO_DAI_DIR_TX) {
		LOG_WRN("Only TX direction supported");
		return -ENOTSUP;
	}

	/* Toggle the FLL to start audio clocking */
	return cs47l63_write_sequence(&drv_cfg->spi, seq_fll_toggle,
				      ARRAY_SIZE(seq_fll_toggle));
}

static int cs47l63_codec_stop(const struct device *dev, audio_dai_dir_t dir)
{
	const struct cs47l63_config *drv_cfg = dev->config;

	if (dir != AUDIO_DAI_DIR_TX) {
		return -ENOTSUP;
	}

	/* Disable FLL */
	return cs47l63_write_reg(&drv_cfg->spi, CS47L63_FLL1_CONTROL1, 0x0000);
}

static void cs47l63_start_output(const struct device *dev)
{
	const struct cs47l63_config *drv_cfg = dev->config;
	int ret;

	ret = cs47l63_write_reg(&drv_cfg->spi, CS47L63_OUTPUT_ENABLE_1, 0x0002);
	if (ret) {
		LOG_ERR("Failed to enable output: %d", ret);
	}
}

static void cs47l63_stop_output(const struct device *dev)
{
	const struct cs47l63_config *drv_cfg = dev->config;
	int ret;

	ret = cs47l63_write_reg(&drv_cfg->spi, CS47L63_OUTPUT_ENABLE_1, 0x0000);
	if (ret) {
		LOG_ERR("Failed to disable output: %d", ret);
	}
}

static int cs47l63_set_property(const struct device *dev,
				audio_property_t property,
				audio_channel_t channel,
				audio_property_value_t val)
{
	const struct cs47l63_config *drv_cfg = dev->config;

	switch (property) {
	case AUDIO_PROPERTY_OUTPUT_VOLUME: {
		/*
		 * Map the application volume (0..100) to the CS47L63 register
		 * range.  The register encodes 0.5 dB steps; 0x00 = −64 dB,
		 * 0x80 = 0 dB.  Clamp to the valid range.
		 */
		uint32_t reg_vol = (uint32_t)val.vol;

		if (reg_vol > CS47L63_MAX_VOLUME_REG_VAL) {
			reg_vol = CS47L63_MAX_VOLUME_REG_VAL;
		}
		return cs47l63_write_reg(&drv_cfg->spi, CS47L63_OUT1L_VOLUME_1,
					 reg_vol | CS47L63_VOLUME_UPDATE_BIT);
	}
	case AUDIO_PROPERTY_OUTPUT_MUTE: {
		if (val.mute) {
			return cs47l63_write_reg(&drv_cfg->spi,
						 CS47L63_OUTPUT_ENABLE_1, 0x0000);
		}
		return cs47l63_write_reg(&drv_cfg->spi,
					 CS47L63_OUTPUT_ENABLE_1, 0x0002);
	}
	default:
		return -ENOTSUP;
	}
}

static int cs47l63_apply_properties(const struct device *dev)
{
	return 0;
}

/* ------------------------------------------------------------------ */
/* Driver API structure                                                */
/* ------------------------------------------------------------------ */

static const struct audio_codec_api cs47l63_api = {
	.configure = cs47l63_codec_configure,
	.start_output = cs47l63_start_output,
	.stop_output = cs47l63_stop_output,
	.set_property = cs47l63_set_property,
	.apply_properties = cs47l63_apply_properties,
	.start = cs47l63_codec_start,
	.stop = cs47l63_codec_stop,
};

/* ------------------------------------------------------------------ */
/* Init & device instantiation                                         */
/* ------------------------------------------------------------------ */

static int cs47l63_init(const struct device *dev)
{
	const struct cs47l63_config *cfg = dev->config;
	uint32_t devid;
	int ret;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	/* Configure and assert hardware reset */
	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("Reset GPIO not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
		/* Pulse reset: assert low, wait, deassert */
		gpio_pin_set_dt(&cfg->reset_gpio, 1);
		k_msleep(1);
		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_msleep(5);
	}

	/* Configure IRQ GPIO as input (if provided) */
	if (cfg->irq_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
			LOG_ERR("IRQ GPIO not ready");
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
		if (ret) {
			return ret;
		}
	}

	/* Verify device ID */
	ret = cs47l63_read_reg(&cfg->spi, CS47L63_DEVID, &devid);
	if (ret) {
		LOG_ERR("Failed to read device ID (err %d)", ret);
		return -EIO;
	}

	if (devid != CS47L63_DEVID_VAL) {
		LOG_ERR("Unexpected device ID: 0x%05x (expected 0x%05x)",
			devid, CS47L63_DEVID_VAL);
		return -ENODEV;
	}

	LOG_INF("CS47L63 detected (DEVID=0x%05x)", devid);
	return 0;
}

#define CS47L63_INIT(inst)                                                         \
	static struct cs47l63_data cs47l63_data_##inst;                            \
	static const struct cs47l63_config cs47l63_config_##inst = {               \
		.spi = SPI_DT_SPEC_INST_GET(                                       \
			inst,                                                      \
			SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |                    \
			SPI_WORD_SET(8) | SPI_LINES_SINGLE),                      \
		.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}),       \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),   \
	};                                                                         \
	DEVICE_DT_INST_DEFINE(inst, cs47l63_init, NULL,                            \
			      &cs47l63_data_##inst, &cs47l63_config_##inst,        \
			      POST_KERNEL, CONFIG_AUDIO_CODEC_INIT_PRIORITY,       \
			      &cs47l63_api);

DT_INST_FOREACH_STATUS_OKAY(CS47L63_INIT)
