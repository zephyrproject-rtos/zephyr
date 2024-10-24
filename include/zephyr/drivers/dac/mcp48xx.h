/*
 * Copyright (c) 2024 Novatech LLC
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DAC_MCP48XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_DAC_MCP48XX_H_

#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>

typedef int (*mcp48xx_latch_outputs_t)(const struct device *dev); /* May want to pass in GPIO */

struct mcp48xx_chip_api {
	struct dac_driver_api dac_api;
	mcp48xx_latch_outputs_t latch_outputs;
};

static inline int mcp48xx_latch_outputs(const struct device *dev)
{
	struct mcp48xx_chip_api *api = (struct mcp48xx_chip_api *)dev->api;

	return api->latch_outputs(dev);
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_DAC_MCP48XX_H_ */
