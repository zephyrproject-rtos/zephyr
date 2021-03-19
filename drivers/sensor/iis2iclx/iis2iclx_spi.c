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

#include "iis2iclx.h"
#include <logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

LOG_MODULE_DECLARE(IIS2ICLX, CONFIG_SENSOR_LOG_LEVEL);

int iis2iclx_spi_init(const struct device *dev)
{
	struct iis2iclx_data *data = dev->data;
	const struct iis2iclx_config *cfg = dev->config;

	data->ctx_spi.read_reg = (stmdev_read_ptr) stmemsc_spi_read;
	data->ctx_spi.write_reg = (stmdev_write_ptr) stmemsc_spi_write;

	data->ctx = &data->ctx_spi;
	data->ctx->handle = (void *)&cfg->stmemsc_cfg.spi;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
