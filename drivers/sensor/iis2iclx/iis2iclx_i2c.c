/* ST Microelectronics IIS2ICLX 2-axis accelerometer sensor driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2iclx.pdf
 */

#define DT_DRV_COMPAT st_iis2iclx

#include <logging/log.h>
#include "iis2iclx.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)

LOG_MODULE_DECLARE(IIS2ICLX, CONFIG_SENSOR_LOG_LEVEL);

int iis2iclx_i2c_init(const struct device *dev)
{
	struct iis2iclx_data *data = dev->data;
	const struct iis2iclx_config *cfg = dev->config;

	/* Use generic stmemsc routine for read/write I2C bus */
	data->ctx_i2c.read_reg = (stmdev_read_ptr) stmemsc_i2c_read;
	data->ctx_i2c.write_reg = (stmdev_write_ptr) stmemsc_i2c_write;

	data->ctx = &data->ctx_i2c;
	data->ctx->handle = (void *)&cfg->stmemsc_cfg.i2c;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */
