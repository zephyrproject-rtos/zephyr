/* ST Microelectronics IIS2MDC 3-axis magnetometer sensor
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2mdc.pdf
 */

#define DT_DRV_COMPAT st_iis2mdc

#include <string.h>
#include <zephyr/logging/log.h>

#include "iis2mdc.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(IIS2MDC, CONFIG_SENSOR_LOG_LEVEL);

static int iis2mdc_i2c_read(const struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint16_t len)
{
	const struct iis2mdc_dev_config *cfg = dev->config;

	return i2c_burst_read_dt(&cfg->i2c, reg_addr, value, len);
}

static int iis2mdc_i2c_write(const struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint16_t len)
{
	const struct iis2mdc_dev_config *cfg = dev->config;

	return i2c_burst_write_dt(&cfg->i2c, reg_addr, value, len);
}

int iis2mdc_i2c_init(const struct device *dev)
{
	struct iis2mdc_data *data = dev->data;
	const struct iis2mdc_dev_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus is not ready");
		return -ENODEV;
	}

	data->ctx_i2c.read_reg = (stmdev_read_ptr) iis2mdc_i2c_read;
	data->ctx_i2c.write_reg = (stmdev_write_ptr) iis2mdc_i2c_write;
	data->ctx_i2c.mdelay = (stmdev_mdelay_ptr) stmemsc_mdelay;

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = (void *)dev;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
