/*
 * Copyright (c) 2010-2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
#include "max22190.h"

#define DT_DRV_COMPAT adi_max22190

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
	#warning "MAX22190 driver enabled without any devices"
#endif

LOG_MODULE_REGISTER(max22190, CONFIG_SENSOR_LOG_LEVEL);

#define PRINT_ERR_BIT(bit1, bit2) if ((bit1) & (bit2)) LOG_ERR("[%s] %d", #bit1, bit1)

/*
 * @brief Compute the CRC5 value for MAX22190
 * @param data - Data array to calculate CRC for.
 * @return CRC result.
 */
static uint8_t max22190_crc(uint8_t *data)
{
	int length = 19;
	uint8_t crc_step, tmp;
	uint8_t crc_init = 0x7;
	uint8_t crc_poly = 0x35;
	int i;

	/*
	 * This is the C custom implementation of CRC function for MAX22190, and
	 * can be found here:
	 * https://www.analog.com/en/design-notes/guidelines-to-implement-crc-algorithm.html
	 */
	uint32_t datainput = (uint32_t)((data[0] << 16) + (data[1] << 8) + data[2]);

	datainput = (datainput & 0xFFFFE0) + crc_init;

	tmp = (uint8_t)((datainput & 0xFC0000) >> 18);

	if ((tmp & 0x20) == 0x20)
		crc_step = (uint8_t)(tmp ^ crc_poly);
	else
		crc_step = tmp;

	for (i = 0; i < length - 1; i++) {
		tmp = (uint8_t)(((crc_step & 0x1F) << 1) +
				((datainput >> (length - 2 - i)) & 0x01));

		if ((tmp & 0x20) == 0x20)
			crc_step = (uint8_t)(tmp ^ crc_poly);
		else
			crc_step = tmp;
	}

	return (uint8_t)(crc_step & 0x1F);
}

/*
 * @brief Update chan WB state in max22190_data
 *
 * @param dev - MAX22190 device.
 * @param val - value to be set.
 */
static void max22190_update_wb_stat(const struct device *dev, uint8_t val)
{
	struct max22190_data *data = dev->data;

	for (int ch_n = 0; ch_n < 8; ch_n++) {
		data->wb[ch_n] = (val >> ch_n) & 0x1;
	}
}

/*
 * @brief Update chan IN state in max22190_data
 *
 * @param dev - MAX22190 device.
 * @param val - value to be set.
 */
static void max22190_update_in_stat(const struct device *dev, uint8_t val)
{
	struct max22190_data *data = dev->data;

	for (int ch_n = 0; ch_n < 8; ch_n++)	{
		data->channels[ch_n] = (val >> ch_n) & 0x1;
	}
}

/*
 * @brief Register write function for MAX22190
 *
 * @param dev - MAX22190 device config.
 * @param addr - Register value to which data is written.
 * @param val - Value which is to be written to requested register.
 * @return 0 in case of success, negative error code otherwise.
 */
static int max22190_reg_transceive(const struct device *dev, uint8_t addr, uint8_t val, uint8_t rw)
{
	uint8_t crc;
	int ret;

	uint8_t local_rx_buff[MAX22190_MAX_PKT_SIZE] = {0};
	uint8_t local_tx_buff[MAX22190_MAX_PKT_SIZE] = {0};

	const struct max22190_config *config = dev->config;

	struct spi_buf tx_buf = {
				.buf = &local_tx_buff,
				.len = config->pkt_size
				};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	struct spi_buf rx_buf = {
				.buf = &local_rx_buff,
				.len = config->pkt_size
				};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	if (config->crc_en) {
		rx_buf.len++;
	}

	local_tx_buff[0] = FIELD_PREP(MAX22190_ADDR_MASK, addr) | FIELD_PREP(MAX22190_RW_MASK, rw);
	local_tx_buff[1] = val;

	/* If CRC enabled calculate it */
	if (config->crc_en) {
		local_tx_buff[2] = max22190_crc(&local_tx_buff[0]);
	}

	/* write cmd & read resp at once */
	ret = spi_transceive_dt(&config->spi, &tx, &rx);

	if (ret) {
		return ret;
	}

	/* if CRC enabled check readed */
	if (config->crc_en) {
		crc = max22190_crc(&local_rx_buff[0]);
		if (crc != (local_rx_buff[2] & 0x1F)) {
			LOG_ERR("READ CRC ERR (%d)-(%d)\n", crc, (local_rx_buff[2] & 0x1F));
			return -EINVAL;
		}
	}

	/* always (R/W) get DI reg in first byte */
	max22190_update_in_stat(dev, local_rx_buff[0]);

	/* in case of writing register we get as second byte WB reg */
	if (rw == MAX22190_WRITE) {
		max22190_update_wb_stat(dev, local_rx_buff[1]);
	} else {
		/* in case of READ second byte is value we are looking for */
		ret = local_rx_buff[1];
	}

	return ret;
}

/*
 * @brief Register update function for MAX22190
 *
 * @param dev - MAX22190 device.
 * @param addr - Register valueto wich data is updated.
 * @param mask - Corresponding mask to the data that will be updated.
 * @param val - Updated value to be written in the register at update.
 * @return 0 in case of success, negative error code otherwise.
 */
static int max22190_reg_update(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	int ret;
	uint32_t reg_val = 0;

	ret = max22190_reg_transceive(dev, addr, 0, MAX22190_READ);

	reg_val = ret;
	reg_val &= ~mask;
	reg_val |= mask & val;

	return max22190_reg_transceive(dev, addr, reg_val, MAX22190_WRITE);
}

#define max22190_clean_por(dev) max22190_reg_update(dev, MAX22190_FAULT1_REG, MAX22190_POR_MASK,\
							 FIELD_PREP(MAX22190_POR_MASK, 0))

/*
 * @brief Check FAULT1 and FAULT2
 *
 * @param dev - MAX22190 device
 */
static void max22190_fault_check(const struct device *dev)
{
	/* FAULT1 */
	uint8_t _fault1 = max22190_reg_transceive(dev, MAX22190_FAULT1_REG, 0,
							MAX22190_READ);

	if (_fault1) {
		max22190_fault1 fault1;

		memcpy(&fault1, &_fault1, 1);

		/* FAULT1_EN */
		uint8_t _fault1_en = max22190_reg_transceive(dev, MAX22190_FAULT1_EN_REG, 0,
								MAX22190_READ);
		max22190_fault1_en fault1_en;

		memcpy(&fault1_en, &_fault1_en, 1);

		PRINT_ERR_BIT(fault1.max22190_CRC, fault1_en.max22190_CRCE);
		PRINT_ERR_BIT(fault1.max22190_POR, fault1_en.max22190_PORE);
		PRINT_ERR_BIT(fault1.max22190_FAULT2, fault1_en.max22190_FAULT2E);
		PRINT_ERR_BIT(fault1.max22190_ALRMT2, fault1_en.max22190_ALRMT2E);
		PRINT_ERR_BIT(fault1.max22190_ALRMT1, fault1_en.max22190_ALRMT1E);
		PRINT_ERR_BIT(fault1.max22190_24VL, fault1_en.max22190_24VLE);
		PRINT_ERR_BIT(fault1.max22190_24VM, fault1_en.max22190_24VME);
		PRINT_ERR_BIT(fault1.max22190_WBG, fault1_en.max22190_WBGE);

		if (fault1.max22190_FAULT2) {
			/* FAULT2 */
			uint8_t _fault2 = max22190_reg_transceive(dev, MAX22190_FAULT2_REG, 0,
								MAX22190_READ);
			max22190_fault2 fault2;

			memcpy(&fault2, &_fault2, 1);

			/* FAULT2_EN */
			uint8_t _fault2_en = max22190_reg_transceive(dev, MAX22190_FAULT2_EN_REG, 0,
								MAX22190_READ);
			max22190_fault2_en fault2_en;

			memcpy(&fault2_en, &_fault2_en, 1);
			PRINT_ERR_BIT(fault2.max22190_RFWBS, fault2_en.max22190_RFWBSE);
			PRINT_ERR_BIT(fault2.max22190_RFWBO, fault2_en.max22190_RFWBOE);
			PRINT_ERR_BIT(fault2.max22190_RFDIS, fault2_en.max22190_RFDISE);
			PRINT_ERR_BIT(fault2.max22190_RFDIO, fault2_en.max22190_RFDIOE);
			PRINT_ERR_BIT(fault2.max22190_OTSHDN, fault2_en.max22190_OTSHDNE);
			PRINT_ERR_BIT(fault2.max22190_FAULT8CK, fault2_en.max22190_FAULT8CKE);
		}
	}
}

static void max22190_state_get(const struct device *dev)
{
	const struct max22190_config *config = dev->config;

	if (gpio_pin_get_dt(&config->fault_gpio))
		max22190_fault_check(dev);

	/* We are reading WB reg because on first byte will be clocked out DI reg
	 * on second byte we will ge WB value.
	 */
	uint8_t wb_val = max22190_reg_transceive(dev, MAX22190_WB_REG, 0, MAX22190_READ);

	max22190_update_wb_stat(dev, (uint8_t)wb_val);
}

static void max22190_set_filter_in(const struct device *dev, uint8_t ch_idx, max22190_flt flt)
{
	uint8_t reg = 0;

	memcpy(&reg, &flt, 1);
	max22190_reg_transceive(dev, MAX22190_FILTER_IN_REG(ch_idx), reg, MAX22190_WRITE);
}

static int max22190_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	if (chan == SENSOR_CHAN_ALL) {
		max22190_state_get(dev);
		return 0;
	}
	return -1;
}

static int max22190_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	if (chan == SENSOR_CHAN_ALL) {
		struct max22190_data *data = dev->data;

		val->val1 = 0;
		val->val2 = 0;
		for (int ch_n = 0; ch_n < 8; ch_n++) {
			/* IN ch state */
			val->val1 |= data->channels[ch_n] << ch_n;
			/* WB ch state */
			val->val2 |= data->wb[ch_n] << ch_n;
		}
		return 0;
	}
	return -1;
}

static const struct sensor_driver_api max22190_api = {
	.sample_fetch = &max22190_sample_fetch,
	.channel_get = &max22190_channel_get,
};

static int max22190_init(const struct device *dev)
{
	const struct max22190_config *config = dev->config;

	int err = 0;
	uint8_t fault1_en_reg = 0;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready\n");
		return -ENODEV;
	}

	/* setup READY gpio - normal low */
	if (!gpio_is_ready_dt(&config->ready_gpio)) {
		LOG_ERR("READY GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->ready_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure reset GPIO");
		return err;
	}

	/* setup FAULT gpio - normal high */
	if (!gpio_is_ready_dt(&config->fault_gpio)) {
		LOG_ERR("FAULT GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->fault_gpio, GPIO_INPUT | GPIO_PULL_UP);
	if (err < 0) {
		LOG_ERR("Failed to configure DC GPIO");
		return err;
	}

	/* setup LATCH gpio - normal high */
	if (!gpio_is_ready_dt(&config->latch_gpio)) {
		LOG_ERR("LATCH GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->latch_gpio, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure busy GPIO");
		return err;
	}

	LOG_DBG(" > MAX22190 MODE: %x", config->mode);
	LOG_DBG(" > MAX22190 PKT SIZE: %dbits (%dbytes)", config->pkt_size * 8, config->pkt_size);
	LOG_DBG(" > MAX22190 CRC: %s", config->crc_en ? "enable" : "disable");

	max22190_flt flt = {
		.max22190_DELAY = MAX22190_DELAY_50US,
		.max22190_FBP = 0,
		.max22190_WBE = 1,
	};

	/* Enable WB on all chan */
	for (int ch = 0; ch < MAX22190_CHANNELS ; ch++) {
		max22190_set_filter_in(dev, ch, flt);
	}

	max22190_fault1_en fault1_en = {
		.max22190_WBGE = 1, /* BIT 0 */
		.max22190_24VME = 0,
		.max22190_24VLE = 0,
		.max22190_ALRMT1E = 0,
		.max22190_ALRMT2E = 0,
		.max22190_FAULT2E = 0,
		.max22190_PORE = 1,
		.max22190_CRCE = 0, /* BIT 7 */
	};

	memcpy(&fault1_en_reg, &fault1_en, 1);

	max22190_reg_transceive(dev, MAX22190_FAULT1_EN_REG, fault1_en_reg, MAX22190_WRITE);

	/* POR bit need to be cleared after start */
	max22190_clean_por(dev);

	return 0;
}

#define max22190_INIT(n)							\
	static struct max22190_data max22190_data_##n;				\
	static const struct max22190_config max22190_config_##n = {		\
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER |		\
						SPI_WORD_SET(8U), 0U),		\
		.ready_gpio = GPIO_DT_SPEC_INST_GET(n, drdy_gpios),		\
		.fault_gpio = GPIO_DT_SPEC_INST_GET(n, fault_gpios),		\
		.latch_gpio = GPIO_DT_SPEC_INST_GET(n, latch_gpios),		\
		.mode = DT_INST_PROP(n, max22190_mode),				\
		.crc_en = !(DT_INST_PROP(n, max22190_mode) & 0x1),		\
		.pkt_size = !(DT_INST_PROP(n, max22190_mode) & 0x1) ? 3 : 2,	\
	};									\
	SENSOR_DEVICE_DT_INST_DEFINE(n, &max22190_init, NULL,			\
					&max22190_data_##n,			\
					&max22190_config_##n,			\
					POST_KERNEL,				\
					CONFIG_SENSOR_INIT_PRIORITY,		\
					&max22190_api);

DT_INST_FOREACH_STATUS_OKAY(max22190_INIT);
