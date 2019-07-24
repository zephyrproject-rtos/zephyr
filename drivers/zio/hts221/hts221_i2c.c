/* ST Microelectronics HTS221 humidity sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/hts221.pdf
 */

#include <string.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#include <drivers/zio/hts221.h>

#ifdef DT_ST_HTS221_BUS_I2C

LOG_MODULE_DECLARE(HTS221, CONFIG_ZIO_LOG_LEVEL);

static int hts221_i2c_read(struct device *dev, u8_t reg_addr,
				 u8_t *value, u16_t len)
{
	struct hts221_data *data = dev->driver_data;
	const struct hts221_config *cfg = dev->config->config_info;

	return i2c_burst_read(data->bus, cfg->i2c_slv_addr,
			      0x80 | reg_addr, value, len);
}

static int hts221_i2c_write(struct device *dev, u8_t reg_addr,
				  u8_t *value, u16_t len)
{
	struct hts221_data *data = dev->driver_data;
	const struct hts221_config *cfg = dev->config->config_info;

	return i2c_burst_write(data->bus, cfg->i2c_slv_addr,
			       0x80 | reg_addr, value, len);
}

int hts221_i2c_init(struct device *dev)
{
	struct hts221_data *data = dev->driver_data;

	data->ctx_i2c.read_reg = (hts221_read_ptr) hts221_i2c_read;
	data->ctx_i2c.write_reg = (hts221_write_ptr) hts221_i2c_write;

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = dev;

	return 0;
}
#endif /* DT_ST_HTS221_BUS_I2C */
