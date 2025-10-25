/*
 * Copyright (c) 2025 Suraj Sonawane <surajsonawane0215@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/dai.h>
#include <zephyr/pm/device_runtime.h>
#include <string.h>

#define DT_DRV_COMPAT zephyr_virtual_dai
LOG_MODULE_REGISTER(virtual_dai, CONFIG_DAI_LOG_LEVEL);

struct virtual_dai_data {
	struct dai_config cfg;
};

static int virtual_dai_probe(const struct device *dev)
{
	return 0;
}

static int virtual_dai_remove(const struct device *dev)
{
	return 0;
}

static int virtual_dai_config_set(const struct device *dev,
			  const struct dai_config *cfg,
			  const void *bespoke_data)
{
	if (cfg->type != DAI_VIRTUAL) {
		LOG_ERR("wrong DAI type: %d", cfg->type);
		return -EINVAL;
	}

	return 0;
}

static int virtual_dai_config_get(const struct device *dev,
			  struct dai_config *cfg,
			  enum dai_dir dir)
{
	struct virtual_dai_data *data = dev->data;

	/* dump content of the DAI configuration */
	memcpy(cfg, &data->cfg, sizeof(*cfg));

	return 0;
}

static const struct dai_properties *
virtual_dai_get_properties(const struct device *dev, enum dai_dir dir, int stream_id)
{
	return NULL;
}

static int virtual_dai_trigger(const struct device *dev,
		       enum dai_dir dir,
		       enum dai_trigger_cmd cmd)
{
	switch (cmd) {
	case DAI_TRIGGER_START:
		LOG_DBG("virtual_dai: START (dir=%d)", dir);
		return 0;
	case DAI_TRIGGER_STOP:
		LOG_DBG("virtual_dai: STOP (dir=%d)", dir);
		return 0;
	case DAI_TRIGGER_PAUSE:
		LOG_DBG("virtual_dai: PAUSE (dir=%d)", dir);
		return 0;
	case DAI_TRIGGER_PRE_START:
	case DAI_TRIGGER_COPY:
		LOG_DBG("virtual_dai: DAI_TRIGGER_COPY");
		return 0;
	default:
		LOG_WRN("virtual_dai: Unknown trigger %d", cmd);
		return -EINVAL;
	}

	CODE_UNREACHABLE;
}

static DEVICE_API(dai, virtual_dai_api) = {
	.probe = virtual_dai_probe,
	.remove = virtual_dai_remove,
	.config_set = virtual_dai_config_set,
	.config_get = virtual_dai_config_get,
	.get_properties = virtual_dai_get_properties,
	.trigger = virtual_dai_trigger,
};

static int virtual_dai_init(const struct device *dev)
{
	return 0;
}

#define VIRTUAL_DAI_INIT(inst) \
static struct virtual_dai_data virtual_dai_data_##inst = {					\
	.cfg.type = DAI_VIRTUAL,						\
	.cfg.dai_index = DT_INST_PROP_OR(inst, dai_index, 0),			\
};										\
										\
DEVICE_DT_INST_DEFINE(inst, \
		      &virtual_dai_init, NULL, \
		      &virtual_dai_data_##inst, NULL, \
		      POST_KERNEL, CONFIG_DAI_INIT_PRIORITY, \
		      &virtual_dai_api);

DT_INST_FOREACH_STATUS_OKAY(VIRTUAL_DAI_INIT);
