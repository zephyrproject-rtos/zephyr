/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Tom Waldron <tom@baremetal.dev>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Stub MSPI driver for the "vnd,mspi" devicetree compatible used in test code.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mspi.h>

#define DT_DRV_COMPAT vnd_mspi

static int vnd_mspi_dev_config(const struct device *dev, const struct mspi_dev_id *dev_id,
			       const enum mspi_dev_cfg_mask param_mask,
			       const struct mspi_dev_cfg *cfg)
{
	return -ENOTSUP;
}

static int vnd_mspi_transceive(const struct device *dev, const struct mspi_dev_id *dev_id,
			       const struct mspi_xfer *req)
{
	return -ENOTSUP;
}

static DEVICE_API(mspi, vnd_mspi_api) = {
	.dev_config = vnd_mspi_dev_config,
	.transceive = vnd_mspi_transceive,
};

#define VND_MSPI_INIT(n)                                                                           \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,   \
			      &vnd_mspi_api);

DT_INST_FOREACH_STATUS_OKAY(VND_MSPI_INIT)
