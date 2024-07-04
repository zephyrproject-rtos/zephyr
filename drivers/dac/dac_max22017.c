/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT adi_max22017

#define MAX22017_LDAC_TOGGLE_TIME 200
#define MAX22017_MAX_CHANNEL      2
#define MAX22017_CRC_POLY         0x8c /* reversed 0x31 poly for crc8-maxim */

#define MAX22017_GEN_ID_OFF     0x00
#define MAX22017_GEN_ID_PROD_ID GENMASK(15, 8)
#define MAX22017_GEN_ID_REV_ID  GENMASK(7, 0)

#define MAX22017_GEN_SERIAL_MSB_OFF        0x01
#define MAX22017_GEN_SERIAL_MSB_SERIAL_MSB GENMASK(15, 0)

#define MAX22017_GEN_SERIAL_LSB_OFF        0x02
#define MAX22017_GEN_SERIAL_LSB_SERIAL_LSB GENMASK(15, 0)

#define MAX22017_GEN_CNFG_OFF                0x03
#define MAX22017_GEN_CNFG_OPENWIRE_DTCT_CNFG GENMASK(15, 14)
#define MAX22017_GEN_CNFG_TMOUT_SEL          GENMASK(13, 10)
#define MAX22017_GEN_CNFG_TMOUT_CNFG         BIT(9)
#define MAX22017_GEN_CNFG_TMOUT_EN           BIT(8)
#define MAX22017_GEN_CNFG_THSHDN_CNFG        GENMASK(7, 6)
#define MAX22017_GEN_CNFG_OVC_SHDN_CNFG      GENMASK(5, 4)
#define MAX22017_GEN_CNFG_OVC_CNFG           GENMASK(3, 2)
#define MAX22017_GEN_CNFG_CRC_EN             BIT(1)
#define MAX22017_GEN_CNFG_DACREF_SEL         BIT(0)

#define MAX22017_GEN_GPIO_CTRL_OFF      0x04
#define MAX22017_GEN_GPIO_CTRL_GPIO_EN  GENMASK(13, 8)
#define MAX22017_GEN_GPIO_CTRL_GPIO_DIR GENMASK(5, 0)

#define MAX22017_GEN_GPIO_DATA_OFF      0x05
#define MAX22017_GEN_GPIO_DATA_GPO_DATA GENMASK(13, 8)
#define MAX22017_GEN_GPIO_DATA_GPI_DATA GENMASK(5, 0)

#define MAX22017_GEN_GPI_INT_OFF              0x06
#define MAX22017_GEN_GPI_INT_GPI_POS_EDGE_INT GENMASK(13, 8)
#define MAX22017_GEN_GPI_INT_GPI_NEG_EDGE_INT GENMASK(5, 0)

#define MAX22017_GEN_GPI_INT_STA_OFF                  0x07
#define MAX22017_GEN_GPI_INT_STA_GPI_POS_EDGE_INT_STA GENMASK(13, 8)
#define MAX22017_GEN_GPI_INT_STA_GPI_NEG_EDGE_INT_STA GENMASK(5, 0)

#define MAX22017_GEN_INT_OFF               0x08
#define MAX22017_GEN_INT_FAIL_INT          BIT(15)
#define MAX22017_GEN_INT_CONV_OVF_INT      GENMASK(13, 12)
#define MAX22017_GEN_INT_OPENWIRE_DTCT_INT GENMASK(11, 10)
#define MAX22017_GEN_INT_HVDD_INT          BIT(9)
#define MAX22017_GEN_INT_TMOUT_INT         BIT(8)
#define MAX22017_GEN_INT_THSHDN_INT        GENMASK(7, 6)
#define MAX22017_GEN_INT_THWRNG_INT        GENMASK(5, 4)
#define MAX22017_GEN_INT_OVC_INT           GENMASK(3, 2)
#define MAX22017_GEN_INT_CRC_INT           BIT(1)
#define MAX22017_GEN_INT_GPI_INT           BIT(0)

#define MAX22017_GEN_INTEN_OFF                 0x09
#define MAX22017_GEN_INTEN_CONV_OVF_INTEN      GENMASK(13, 12)
#define MAX22017_GEN_INTEN_OPENWIRE_DTCT_INTEN GENMASK(11, 10)
#define MAX22017_GEN_INTEN_HVDD_INTEN          BIT(9)
#define MAX22017_GEN_INTEN_TMOUT_INTEN         BIT(8)
#define MAX22017_GEN_INTEN_THSHDN_INTEN        GENMASK(7, 6)
#define MAX22017_GEN_INTEN_THWRNG_INTEN        GENMASK(5, 4)
#define MAX22017_GEN_INTEN_OVC_INTEN           GENMASK(3, 2)
#define MAX22017_GEN_INTEN_CRC_INTEN           BIT(1)
#define MAX22017_GEN_INTEN_GPI_INTEN           BIT(0)

#define MAX22017_GEN_RST_CTRL_OFF             0x0A
#define MAX22017_GEN_RST_CTRL_AO_COEFF_RELOAD GENMASK(15, 14)
#define MAX22017_GEN_RST_CTRL_GEN_RST         BIT(9)

#define MAX22017_AO_CMD_OFF        0x40
#define MAX22017_AO_CMD_AO_LD_CTRL GENMASK(15, 14)

#define MAX22017_AO_STA_OFF      0x41
#define MAX22017_AO_STA_BUSY_STA GENMASK(15, 14)
#define MAX22017_AO_STA_SLEW_STA GENMASK(13, 12)
#define MAX22017_AO_STA_FAIL_STA BIT(0)

#define MAX22017_AO_CNFG_OFF                  0x42
#define MAX22017_AO_CNFG_AO_LD_CNFG           GENMASK(15, 14)
#define MAX22017_AO_CNFG_AO_CM_SENSE          GENMASK(13, 12)
#define MAX22017_AO_CNFG_AO_UNI               GENMASK(11, 10)
#define MAX22017_AO_CNFG_AO_MODE              GENMASK(9, 8)
#define MAX22017_AO_CNFG_AO_OPENWIRE_DTCT_LMT GENMASK(5, 4)
#define MAX22017_AO_CNFG_AI_EN                GENMASK(3, 2)
#define MAX22017_AO_CNFG_AO_EN                GENMASK(1, 0)

#define MAX22017_AO_SLEW_RATE_CHn_OFF(n)                (0x43 + n)
#define MAX22017_AO_SLEW_RATE_CHn_AO_SR_EN_CHn          BIT(5)
#define MAX22017_AO_SLEW_RATE_CHn_AO_SR_SEL_CHn         BIT(4)
#define MAX22017_AO_SLEW_RATE_CHn_AO_SR_STEP_SIZE_CHn   GENMASK(3, 2)
#define MAX22017_AO_SLEW_RATE_CHn_AO_SR_UPDATE_RATE_CHn GENMASK(1, 0)

#define MAX22017_AO_DATA_CHn_OFF(n)     (0x45 + n)
#define MAX22017_AO_DATA_CHn_AO_DATA_CH GENMASK(15, 0)

#define MAX22017_AO_OFFSET_CORR_CHn_OFF(n)       (0x47 + (2 * n))
#define MAX22017_AO_OFFSET_CORR_CHn_AO_OFFSET_CH GENMASK(15, 0)

#define MAX22017_AO_GAIN_CORR_CHn_OFF(n)     (0x48 + (2 * n))
#define MAX22017_AO_GAIN_CORR_CHn_AO_GAIN_CH GENMASK(15, 0)

#define MAX22017_SPI_TRANS_ADDR    GENMASK(7, 1)
#define MAX22017_SPI_TRANS_DIR     BIT(0)
#define MAX22017_SPI_TRANS_PAYLOAD GENMASK(15, 0)

LOG_MODULE_REGISTER(dac_max22017, CONFIG_DAC_LOG_LEVEL);

struct max22017_config {
	struct spi_dt_spec spi;
	uint8_t resolution;
	uint8_t nchannels;
	const struct gpio_dt_spec gpio_reset;
	const struct gpio_dt_spec gpio_ldac;
	const struct gpio_dt_spec gpio_busy;
	const struct gpio_dt_spec gpio_int;
	uint8_t latch_mode[MAX22017_MAX_CHANNEL];
	uint8_t polarity_mode[MAX22017_MAX_CHANNEL];
	uint8_t dac_mode[MAX22017_MAX_CHANNEL];
	uint8_t ovc_mode[MAX22017_MAX_CHANNEL];
	uint16_t timeout;
	bool crc_mode;
};

struct max22017_data {
	const struct device *dev;
	struct k_sem lock;
	struct k_work int_work;
	struct gpio_callback callback_int;
	bool crc_enabled;
};

static int max22017_reg_read(const struct device *dev, uint8_t addr, uint16_t *value)
{
	int ret;
	const struct max22017_config *config = dev->config;
	const struct max22017_data *data = dev->data;
	uint8_t rxbuffer[4] = {0};
	size_t recv_len = 3;

	*value = 0;
	addr = FIELD_PREP(MAX22017_SPI_TRANS_ADDR, addr) | FIELD_PREP(MAX22017_SPI_TRANS_DIR, 1);

	if (data->crc_enabled) {
		recv_len += 1;
	}

	const struct spi_buf txb[] = {
		{
			.buf = &addr,
			.len = 1,
		},
	};
	const struct spi_buf rxb[] = {
		{
			.buf = rxbuffer,
			.len = recv_len,
		},
	};

	struct spi_buf_set tx = {
		.buffers = txb,
		.count = ARRAY_SIZE(txb),
	};

	struct spi_buf_set rx = {
		.buffers = rxb,
		.count = ARRAY_SIZE(rxb),
	};

	ret = spi_transceive_dt(&config->spi, &tx, &rx);

	if (ret) {
		return ret;
	}

	if (!data->crc_enabled) {
		goto out_skip_crc;
	}

	uint8_t crc_in[] = {addr, rxbuffer[1], rxbuffer[2]};

	ret = crc8(crc_in, 3, MAX22017_CRC_POLY, 0, true);

	if (ret != rxbuffer[3]) {
		LOG_ERR("Reg read: CRC Mismatch calculated / read: %x / %x", ret, rxbuffer[3]);
		return -EINVAL;
	}

out_skip_crc:
	*value = rxbuffer[1] | rxbuffer[2] >> 8;
	*value = sys_be16_to_cpu(*value);

	return 0;
}

static int max22017_reg_write(const struct device *dev, uint8_t addr, uint16_t value)
{
	uint8_t crc;
	size_t crc_len = 0;
	const struct max22017_config *config = dev->config;
	const struct max22017_data *data = dev->data;

	addr = FIELD_PREP(MAX22017_SPI_TRANS_ADDR, addr) | FIELD_PREP(MAX22017_SPI_TRANS_DIR, 0);
	value = sys_cpu_to_be16(value);

	if (data->crc_enabled) {
		uint8_t crc_in[] = {addr, ((uint8_t *)&value)[0], ((uint8_t *)&value)[1]};

		crc_len = 1;
		crc = crc8(crc_in, 3, MAX22017_CRC_POLY, 0, true);
	}

	const struct spi_buf buf[] = {
		{
			.buf = &addr,
			.len = 1,
		},
		{
			.buf = &value,
			.len = 2,
		},
		{
			.buf = &crc,
			.len = crc_len,
		},
	};

	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	return spi_write_dt(&config->spi, &tx);
}

static int max22017_channel_setup(const struct device *dev,
				  const struct dac_channel_cfg *channel_cfg)
{
	int ret;
	uint16_t ao_cnfg, gen_cnfg;
	uint8_t chan = channel_cfg->channel_id;
	const struct max22017_config *config = dev->config;
	struct max22017_data *data = dev->data;

	if (chan > config->nchannels - 1) {
		LOG_ERR("Unsupported channel %d", chan);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %d", chan);
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);
	ret = max22017_reg_read(dev, MAX22017_AO_CNFG_OFF, &ao_cnfg);
	if (ret) {
		goto fail;
	}

	ao_cnfg |= FIELD_PREP(MAX22017_AO_CNFG_AO_EN, BIT(chan));

	if (!config->latch_mode[chan]) {
		ao_cnfg |= FIELD_PREP(MAX22017_AO_CNFG_AO_LD_CNFG, BIT(chan));
	}

	if (config->polarity_mode[chan]) {
		ao_cnfg |= FIELD_PREP(MAX22017_AO_CNFG_AO_UNI, BIT(chan));
	}

	if (config->dac_mode[chan]) {
		ao_cnfg |= FIELD_PREP(MAX22017_AO_CNFG_AO_MODE, BIT(chan));
	}

	ret = max22017_reg_write(dev, MAX22017_AO_CNFG_OFF, ao_cnfg);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_read(dev, MAX22017_GEN_CNFG_OFF, &gen_cnfg);
	if (ret) {
		goto fail;
	}

	if (config->ovc_mode[chan]) {
		gen_cnfg |= FIELD_PREP(MAX22017_GEN_CNFG_OVC_CNFG, BIT(chan));
		/* Over current shutdown mode */
		if (config->ovc_mode[chan] == 2) {
			gen_cnfg |= FIELD_PREP(MAX22017_GEN_CNFG_OVC_SHDN_CNFG, BIT(chan));
		}
	}

	ret = max22017_reg_write(dev, MAX22017_GEN_CNFG_OFF, gen_cnfg);
fail:
	k_sem_give(&data->lock);
	return ret;
}

static int max22017_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	int ret;
	uint16_t ao_sta;
	const struct max22017_config *config = dev->config;
	struct max22017_data *data = dev->data;

	if (channel > config->nchannels - 1) {
		LOG_ERR("unsupported channel %d", channel);
		return ENOTSUP;
	}

	if (value >= (1 << config->resolution)) {
		LOG_ERR("Value %d out of range", value);
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	if (config->gpio_busy.port) {
		if (gpio_pin_get_dt(&config->gpio_busy)) {
			ret = -EBUSY;
			goto fail;
		}
	} else {
		ret = max22017_reg_read(dev, MAX22017_AO_STA_OFF, &ao_sta);
		if (ret) {
			goto fail;
		}
		if (FIELD_GET(MAX22017_AO_STA_BUSY_STA, ao_sta)) {
			ret = -EBUSY;
			goto fail;
		}
	}

	ret = max22017_reg_write(dev, MAX22017_AO_DATA_CHn_OFF(channel),
				 FIELD_PREP(MAX22017_AO_DATA_CHn_AO_DATA_CH, value));
	if (ret) {
		goto fail;
	}

	if (config->latch_mode[channel]) {
		if (config->gpio_ldac.port) {
			gpio_pin_set_dt(&config->gpio_ldac, false);
			k_sleep(K_USEC(MAX22017_LDAC_TOGGLE_TIME));
			gpio_pin_set_dt(&config->gpio_ldac, true);
		} else {
			ret = max22017_reg_write(
				dev, MAX22017_AO_CMD_OFF,
				FIELD_PREP(MAX22017_AO_CMD_AO_LD_CTRL, BIT(channel)));
		}
	}
fail:
	k_sem_give(&data->lock);
	return ret;
}

static void max22017_isr(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	struct max22017_data *data = CONTAINER_OF(gpio_cb, struct max22017_data, callback_int);
	int ret;

	k_sem_take(&data->lock, K_FOREVER);

	ret = k_work_submit(&data->int_work);
	if (ret < 0) {
		LOG_WRN("Could not submit int work: %d", ret);
	}

	k_sem_give(&data->lock);
}

static void max22017_int_worker(struct k_work *work)
{
	struct max22017_data *data = CONTAINER_OF(work, struct max22017_data, int_work);
	const struct device *dev = data->dev;
	int ret;
	uint16_t gen_int;

	k_sem_take(&data->lock, K_FOREVER);

	ret = max22017_reg_read(dev, MAX22017_GEN_INT_OFF, &gen_int);
	if (ret) {
		LOG_ERR("Unable to read GEN_INT register");
		goto fail;
	}

	if (FIELD_GET(MAX22017_GEN_INT_FAIL_INT, gen_int)) {
		LOG_ERR("Boot failure");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_CONV_OVF_INT, gen_int);
	if (ret) {
		LOG_ERR("Conversion failure on channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_OPENWIRE_DTCT_INT, gen_int);
	if (ret) {
		LOG_ERR("Openwire detected on channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	if (FIELD_GET(MAX22017_GEN_INT_HVDD_INT, gen_int)) {
		LOG_ERR("HVDD/HVSS voltage difference below 1.5V");
	}

	if (FIELD_GET(MAX22017_GEN_INT_TMOUT_INT, gen_int)) {
		LOG_ERR("SPI transaction timeout");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_THSHDN_INT, gen_int);
	if (ret) {
		LOG_ERR("Thermal shutdown AO channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_THWRNG_INT, gen_int);
	if (ret) {
		LOG_ERR("Thermal warning AO channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_OVC_INT, gen_int);
	if (ret) {
		LOG_ERR("Over current on channels: %s %s", (ret & BIT(0)) ? "0" : "",
			(ret & BIT(1)) ? "1" : "");
	}

	if (FIELD_GET(MAX22017_GEN_INT_CRC_INT, gen_int)) {
		LOG_ERR("CRC Error");
	}

	ret = FIELD_GET(MAX22017_GEN_INT_GPI_INT, gen_int);
	if (ret) {
		LOG_INF("GPI Interrupt: %d", ret);
	}
fail:
	k_sem_give(&data->lock);
}

static int max22017_init(const struct device *dev)
{
	const struct max22017_config *config = dev->config;
	struct max22017_data *data = dev->data;
	uint16_t version;
	int ret;
	uint16_t gen_cnfg = 0, gen_int_en = 0;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI spi %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	if (config->gpio_reset.port != NULL) {
		ret = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_ERR("failed to initialize GPIO reset pin");
			return ret;
		}
		k_sleep(K_MSEC(100));
		gpio_pin_set_dt(&config->gpio_reset, 1);
		k_sleep(K_MSEC(500));
	}

	data->dev = dev;
	k_work_init(&data->int_work, max22017_int_worker);
	k_sem_init(&data->lock, 0, 1);

	if (config->gpio_int.port) {
		ret = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
		if (ret) {
			LOG_ERR("failed to initialize GPIO interrupt pin");
			goto fail;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret) {
			LOG_ERR("failed to configure interrupt pin");
			goto fail;
		}

		gpio_init_callback(&data->callback_int, max22017_isr, BIT(config->gpio_int.pin));
		ret = gpio_add_callback(config->gpio_int.port, &data->callback_int);
		if (ret) {
			LOG_ERR("failed to add data ready callback");
			goto fail;
		}
	}

	ret = max22017_reg_write(dev, MAX22017_AO_CNFG_OFF, 0);
	if (ret) {
		return ret;
	}

	ret = max22017_reg_write(dev, MAX22017_AO_DATA_CHn_OFF(0), 0);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_write(dev, MAX22017_AO_DATA_CHn_OFF(1), 0);
	if (ret) {
		goto fail;
	}

	if (config->crc_mode) {
		gen_cnfg |= FIELD_PREP(MAX22017_GEN_CNFG_CRC_EN, 1);
		gen_int_en |= FIELD_PREP(MAX22017_GEN_INTEN_CRC_INTEN, 1);
	}

	if (config->timeout) {
		gen_cnfg |= FIELD_PREP(MAX22017_GEN_CNFG_TMOUT_EN, 1) |
			    FIELD_PREP(MAX22017_GEN_CNFG_TMOUT_SEL, (config->timeout / 100) - 1);
		gen_int_en |= FIELD_PREP(MAX22017_GEN_INTEN_TMOUT_INTEN, 1);
	}

	ret = max22017_reg_write(dev, MAX22017_GEN_INTEN_OFF, gen_int_en);
	if (ret) {
		goto fail;
	}

	ret = max22017_reg_write(dev, MAX22017_GEN_CNFG_OFF, gen_cnfg);
	if (ret) {
		goto fail;
	}

	if (config->crc_mode) {
		data->crc_enabled = true;
	}

	if (config->gpio_ldac.port) {
		ret = gpio_pin_configure_dt(&config->gpio_ldac, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			LOG_ERR("failed to initialize GPIO ldac pin");
			goto fail;
		}
	}

	if (config->gpio_busy.port) {
		ret = gpio_pin_configure_dt(&config->gpio_busy, GPIO_INPUT);
		if (ret) {
			LOG_ERR("failed to initialize GPIO busy pin");
			goto fail;
		}
	}

	ret = max22017_reg_read(dev, MAX22017_GEN_ID_OFF, &version);
	if (ret) {
		LOG_ERR("Unable to read MAX22017 version over SPI: %d", ret);
		goto fail;
	}

	LOG_INF("MAX22017 version: 0x%lx 0x%lx", FIELD_GET(MAX22017_GEN_ID_PROD_ID, version),
		FIELD_GET(MAX22017_GEN_ID_REV_ID, version));

fail:
	k_sem_give(&data->lock);
	return ret;
}

static const struct dac_driver_api max22017_driver_api = {
	.channel_setup = max22017_channel_setup,
	.write_value = max22017_write_value,
};

#define INST_DT_MAX22017(index)                                                                    \
	static const struct max22017_config max22017_config_##index = {                            \
		.spi = SPI_DT_SPEC_INST_GET(index, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U), 0U),     \
		.resolution = DT_INST_PROP_OR(index, resolution, 16),                              \
		.nchannels = DT_INST_PROP_OR(index, num_channels, 2),                              \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(index, int_gpios, {0}),                       \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(index, rst_gpios, {0}),                     \
		.gpio_busy = GPIO_DT_SPEC_INST_GET_OR(index, busy_gpios, {0}),                     \
		.gpio_ldac = GPIO_DT_SPEC_INST_GET_OR(index, ldac_gpios, {0}),                     \
		.latch_mode = DT_INST_PROP_OR(index, latch_mode, {0}),                             \
		.polarity_mode = DT_INST_PROP_OR(index, polarity_mode, {0}),                       \
		.dac_mode = DT_INST_PROP_OR(index, dac_mode, {0}),                                 \
		.ovc_mode = DT_INST_PROP_OR(index, overcurrent_mode, {0}),                         \
		.timeout = DT_INST_PROP_OR(index, timeout, 0),                                     \
		.crc_mode = DT_INST_PROP_OR(index, crc_mode, 0),                                   \
	};                                                                                         \
                                                                                                   \
	static struct max22017_data max22017_data_##index;                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, max22017_init, NULL, &max22017_data_##index,                  \
			      &max22017_config_##index, POST_KERNEL,                               \
			      CONFIG_DAC_MAX22017_INIT_PRIORITY, &max22017_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INST_DT_MAX22017);
