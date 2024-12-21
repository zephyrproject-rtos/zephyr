/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real mipi-dsi driver. It is used to instantiate struct
 * devices for the "vnd,mipi-dsi" devicetree compatible used in test code.
 */

#include <zephyr/drivers/mipi_dsi.h>

#define DT_DRV_COMPAT vnd_mipi_dsi

static int vnd_mipi_dsi_attach(const struct device *dev, uint8_t channel,
			       const struct mipi_dsi_device *mdev)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(mdev);

	return -ENOTSUP;
}

static ssize_t vnd_mipi_dsi_transfer(const struct device *dev, uint8_t channel,
				     struct mipi_dsi_msg *msg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(msg);

	return -1;
}

static int vnd_mipi_dsi_detach(const struct device *dev, uint8_t channel,
			       const struct mipi_dsi_device *mdev)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(mdev);

	return -ENOTSUP;
}

static struct mipi_dsi_driver_api vnd_mipi_dsi_api = {
	.attach = vnd_mipi_dsi_attach,
	.transfer = vnd_mipi_dsi_transfer,
	.detach = vnd_mipi_dsi_detach,
};

#define VND_MIPI_DSI_INIT(n)                                                                       \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                              \
			      CONFIG_MIPI_DSI_INIT_PRIORITY, &vnd_mipi_dsi_api);

DT_INST_FOREACH_STATUS_OKAY(VND_MIPI_DSI_INIT)
