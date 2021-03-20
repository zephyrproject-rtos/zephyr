/* ST Microelectronics LSM6DSO 6-axis IMU sensor driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

#define DT_DRV_COMPAT st_lsm6dso

#include "lsm6dso.h"
#include <logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

LOG_MODULE_DECLARE(LSM6DSO, CONFIG_SENSOR_LOG_LEVEL);

int lsm6dso_spi_init(const struct device *dev)
{
	struct lsm6dso_data *data = dev->data;
	const struct lsm6dso_config *cfg = dev->config;

	if (!device_is_ready(cfg->stmemsc_cfg.spi.bus)) {
		LOG_ERR("Cannot get pointer to bus device");
		return -ENODEV;
	}

	/* Use generic stmemsc routine for read/write SPI bus */
	data->ctx.read_reg = (stmdev_read_ptr) stmemsc_spi_read;
	data->ctx.write_reg = (stmdev_write_ptr) stmemsc_spi_write;

	data->ctx.handle = (void *)&cfg->stmemsc_cfg.spi;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
