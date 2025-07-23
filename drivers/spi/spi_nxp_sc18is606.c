/*
 * Copyright (c) 2025, tinyvision.ai
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>

#define DT_DRV_COMPAT nxp_sc18is606


LOG_MODULE_REGISTER(nxp_sc18is606, CONFIG_SPI_LOG_LEVEL);


#define SC18IS606_CONFIG_SPI		0xF0
#define CLEAR_INTERRUPT		0xF1
#define IDLE_MODE		0xF2
#define GPIO_CONFIGURATION	0xF7
#define GPIO_ENABLE		0xF6
#define GPIO_WRITE		0xF4
#define GPIO_READ		0xF5
#define READ_VERSION		0xFE


#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "Nxp SC18IS606 driver enabled without any devices"
#endif


struct nxp_sc18is606_data
{
	const struct device *i2c_dev;
	uint8_t i2c_addr;
	uint32_t spi_clock_freq;
	uint8_t spi_mode;
};


struct nxp_sc18is606_config
{
	struct i2c_dt_spec i2c_controller;
};


static int sc18is606_write_reg(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t value)
{
	uint8_t  buffer[2] = {reg, value};
	return i2c_write_dt(i2c, buffer, sizeof(buffer));
}


static int sc18is606_spi_transceive(const struct device *dev,
		const struct spi_config *spi_cfg,
		const struct spi_buf_set *tx_buffer_set,
		const struct spi_buf_set *rx_buffer_set)
{
	const struct nxp_sc18is606_config *cfg = dev->config;
	const struct nxp_sc18is606_data *data = dev->data;
	const struct device *i2c_dev = data->i2c_dev;
	int ret;


	//our buffers are real?
	if(!tx_buffer_set || !rx_buffer_set) {
		LOG_ERR("SC18IS606 Invalid Buffers");
		return -EINVAL;

	}

	//TX
	const struct spi_buf *tx_buf = &tx_buffer_set->buffers[0];
	const size_t len = tx_buf->len;

	uint8_t sc18_buf[1 + len];
	sc18_buf[0] = 0x00;
	memcpy(&sc18_buf[1], tx_buf->buf, len);

	ret = i2c_write(i2c_dev, sc18_buf, sizeof(sc18_buf), data->i2c_addr);
		if(ret) {
			LOG_ERR("SPI write failed: %d", ret);
			return ret;
		}


	//RX
	if(rx_buffer_set && rx_buffer_set->buffers && rx_buffer_set->count > 0){
		const struct spi_buf *buf = &rx_buffer_set->buffers[0];
		ret = i2c_read(i2c_dev, buf->buf, buf->len, data->i2c_addr);
		if(ret) {
			LOG_ERR("SPI read failed: %d", ret);
			return ret;
		}
	}

	return 0;
}


// expose driver
static const struct spi_driver_api sc18is606_api = {
	.transceive = sc18is606_spi_transceive,
};


// Init  the driver
static int sc18is606_init(const struct device *dev)
{
	const struct nxp_sc18is606_config *cfg = dev->config;
	struct nxp_sc18is606_data *data = dev->data;
	int ret;

	// Get Parent
	data->i2c_dev = cfg->i2c_controller.bus;
	if(!data->i2c_dev) {
		LOG_ERR("I2C controller %s not found", data->i2c_dev->name);
		return -ENODEV;
	}

	LOG_INF("Using I2C controller: %s", data->i2c_dev->name);

	ret = sc18is606_write_reg(&cfg->i2c_controller, SC18IS606_CONFIG_SPI, data->spi_mode);
	if(ret) {
		LOG_ERR("Failed to CONFIGURE the SC18IS606: %d", ret);
		return ret;
	}

	LOG_INF("SC18IS606 initialized");
	return 0;
}

//define our driver instance
#define NXP_SC18IS606_DEFINE(inst) \
	static struct nxp_sc18is606_data sc18is606_data_##inst = { \
		.i2c_addr = DT_INST_REG_ADDR(inst), \
		.spi_clock_freq = DT_INST_PROP(inst, spi_clock_frequency), \
		.spi_mode = DT_INST_PROP(inst, spi_mode), \
	}; \
	static const struct nxp_sc18is606_config sc18is606_config_##inst = { \
		.i2c_controller = I2C_DT_SPEC_INST_GET(inst), \
	}; \
	DEVICE_DT_INST_DEFINE(inst, sc18is606_init, NULL, \
			&sc18is606_data_##inst, &sc18is606_config_##inst, \
			POST_KERNEL, CONFIG_SPI_INIT_PRIORITY, \
			&sc18is606_api);
//when status is ok make an instance
DT_INST_FOREACH_STATUS_OKAY(NXP_SC18IS606_DEFINE)


