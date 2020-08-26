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
#include <drivers/i2c.h>
#include <logging/log.h>

#include "iis2mdc.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(IIS2MDC);

static int iis2mdc_i2c_read(struct device *dev, uint8_t reg_addr,
				 uint8_t *value, uint16_t len)
{
	struct iis2mdc_data *data = dev->data;
	const struct iis2mdc_config *cfg = dev->config;

	return i2c_burst_read(data->bus, cfg->i2c_slv_addr,
			      reg_addr, value, len);
}

static int iis2mdc_i2c_write(struct device *dev, uint8_t reg_addr,
				  uint8_t *value, uint16_t len)
{
	struct iis2mdc_data *data = dev->data;
	const struct iis2mdc_config *cfg = dev->config;

	return i2c_burst_write(data->bus, cfg->i2c_slv_addr,
			       reg_addr, value, len);
}

int iis2mdc_i2c_init(struct device *dev)
{
	struct iis2mdc_data *data = dev->data;

	data->ctx_i2c.read_reg = (stmdev_read_ptr) iis2mdc_i2c_read;
	data->ctx_i2c.write_reg = (stmdev_write_ptr) iis2mdc_i2c_write;

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = dev;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
