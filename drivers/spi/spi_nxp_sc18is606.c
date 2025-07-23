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

#define DT_DRV_COMPAT nxp_sc18is606


LOG_MODULE_REGISTER(nxp_sc18is606, CONFIG_I2C_LOG_LEVEL);


#define CONFIG_SPI		0xF0
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
};


struct nxp_sc18is606_config
{
	struct i2c_dt_spec bus;
};



int sc18is606_write_to_spi(const struct device *dev, uint8_t func_id, uint8_t *data, size_t len)
{
	const struct nxp_sc18is606_config *config = dev->config;
	// The data buffer is only 1024 bytes deep
	if(len > 1024)
		return -1;

	// Construct a data packet by bundling the address and the data
	uint8_t data_packet[1025] = {func_id};

	memcpy(&data_packet[1], data, sizeof(data_packet));



	int ret = i2c_write_dt(&config->bus, data_packet, len+1);

	return  ret;
}


static DEVICE_API(i2c, sc18is606_driver_api) = {
	.write_data = sc18is606_write_to_spi,
};

