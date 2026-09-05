/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real mspi driver. It is used to instantiate struct
 * devices for the "vnd,mspi" devicetree compatible used in test code.
 */

#include <zephyr/drivers/mspi.h>

#define DT_DRV_COMPAT vnd_mspi

static int vnd_mspi_config(const struct mspi_dt_spec *spec)
{
	ARG_UNUSED(spec);

	return -ENOTSUP;
}

static int vnd_mspi_dev_config(const struct device *dev, const struct mspi_dev_id *dev_id,
			       const enum mspi_dev_cfg_mask param_mask,
			       const struct mspi_dev_cfg *dev_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(param_mask);
	ARG_UNUSED(dev_cfg);

	return -ENOTSUP;
}

static int vnd_mspi_memmap_config(const struct device *dev, const struct mspi_dev_id *dev_id,
				  const struct mspi_memmap_cfg *memmap_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(memmap_cfg);

	return -ENOTSUP;
}

static int vnd_mspi_scramble_config(const struct device *dev, const struct mspi_dev_id *dev_id,
				    const struct mspi_scramble_cfg *scramble_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(scramble_cfg);

	return -ENOTSUP;
}

static int vnd_mspi_timing_config(const struct device *dev, const struct mspi_dev_id *dev_id,
				  const uint32_t param_mask, void *timing_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(param_mask);
	ARG_UNUSED(timing_cfg);

	return -ENOTSUP;
}

static int vnd_mspi_get_channel_status(const struct device *dev, uint8_t ch)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ch);

	return -ENOTSUP;
}

static int vnd_mspi_register_callback(const struct device *dev, const struct mspi_dev_id *dev_id,
				      const enum mspi_bus_event evt_type,
				      mspi_callback_handler_t cb, struct mspi_callback_context *ctx)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(evt_type);
	ARG_UNUSED(cb);
	ARG_UNUSED(ctx);

	return -ENOTSUP;
}

static int vnd_mspi_transceive(const struct device *dev, const struct mspi_dev_id *dev_id,
			       const struct mspi_xfer *xfer)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(dev_id);
	ARG_UNUSED(xfer);

	return -ENOTSUP;
}

static DEVICE_API(mspi, vnd_mspi_api) = {
	.config = vnd_mspi_config,
	.dev_config = vnd_mspi_dev_config,
	.memmap_config = vnd_mspi_memmap_config,
	.scramble_config = vnd_mspi_scramble_config,
	.timing_config = vnd_mspi_timing_config,
	.get_channel_status = vnd_mspi_get_channel_status,
	.register_callback = vnd_mspi_register_callback,
	.transceive = vnd_mspi_transceive,
};

#define VND_MSPI_INIT(n)                                                                           \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,   \
			      &vnd_mspi_api);

DT_INST_FOREACH_STATUS_OKAY(VND_MSPI_INIT)
