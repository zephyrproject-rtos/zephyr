/*
 * Copyright (c) 2026 Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C gates of the ADI GMSL2 serializer/deserializer pair.
 *
 * A gate is an I2C controller that forwards every transfer, unchanged, to the bus its owner sits
 * on. It gives the devices behind a GMSL2 link their place in the devicetree, host bus ->
 * deserializer gate -> serializer -> serializer gate -> sensor, mirroring the path the
 * transactions take through the deserializer's and the serializer's I2C forwarding.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_gmsl_gate, CONFIG_I2C_LOG_LEVEL);

struct i2c_gmsl_gate_config {
	/* Bus the owner sits on; every transfer is forwarded there. */
	const struct device *bus;
	/* The GMSL2 device whose forwarding this gate represents. */
	const struct device *owner;
	/* Attempts after a refused transfer. */
	uint8_t retry_num;
};

static int i2c_gmsl_gate_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_gmsl_gate_config *cfg = dev->config;

	return i2c_configure(cfg->bus, dev_config);
}

static int i2c_gmsl_gate_get_config(const struct device *dev, uint32_t *dev_config)
{
	const struct i2c_gmsl_gate_config *cfg = dev->config;

	return i2c_get_config(cfg->bus, dev_config);
}

static int i2c_gmsl_gate_recover_bus(const struct device *dev)
{
	const struct i2c_gmsl_gate_config *cfg = dev->config;

	return i2c_recover_bus(cfg->bus);
}

static int i2c_gmsl_gate_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				  uint16_t addr)
{
	const struct i2c_gmsl_gate_config *cfg = dev->config;
	int ret;

	/*
	 * On some board paths the deserializer refuses a transfer to the far side that starts
	 * right after the previous one ended, with a NAK of the address. A retry recovers it.
	 */
	for (unsigned int i = 0; i <= cfg->retry_num; i++) {
		ret = i2c_transfer(cfg->bus, msgs, num_msgs, addr);
		if (ret == 0) {
			return 0;
		}
	}

	return ret;
}

static int i2c_gmsl_gate_init(const struct device *dev)
{
	const struct i2c_gmsl_gate_config *cfg = dev->config;

	if (!device_is_ready(cfg->bus)) {
		LOG_ERR("%s: bus %s not ready", dev->name, cfg->bus->name);
		return -ENODEV;
	}

	if (!device_is_ready(cfg->owner)) {
		LOG_ERR("%s: %s not ready", dev->name, cfg->owner->name);
		return -ENODEV;
	}

	LOG_DBG("%s: initialized, owner %s, bus %s", dev->name, cfg->owner->name, cfg->bus->name);

	return 0;
}

static DEVICE_API(i2c, i2c_gmsl_gate_api) = {
	.configure = i2c_gmsl_gate_configure,
	.get_config = i2c_gmsl_gate_get_config,
	.transfer = i2c_gmsl_gate_transfer,
	.recover_bus = i2c_gmsl_gate_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
};

#define I2C_GMSL_GATE_DEFINE(node_id, prio, retries)                                               \
	static const struct i2c_gmsl_gate_config i2c_gmsl_gate_cfg_##node_id = {                   \
		.bus = DEVICE_DT_GET(DT_BUS(DT_PARENT(node_id))),                                  \
		.owner = DEVICE_DT_GET(DT_PARENT(node_id)),                                        \
		.retry_num = retries,                                                              \
	};                                                                                         \
	I2C_DEVICE_DT_DEFINE(node_id, i2c_gmsl_gate_init, NULL, NULL,                              \
			     &i2c_gmsl_gate_cfg_##node_id, POST_KERNEL, prio, &i2c_gmsl_gate_api);

/*
 * A gate is built only with its owner's driver: disabling a deserializer or serializer node
 * leaves its gate child enabled in devicetree.
 */

/* The deserializer's gates: the refusals happen on the deserializer, so the retries are here. */
#if DT_HAS_COMPAT_STATUS_OKAY(adi_max96724_i2c_gate) && defined(CONFIG_VIDEO_MAX96724)
BUILD_ASSERT(CONFIG_I2C_GMSL_GATE_MAX96724_INIT_PRIORITY > CONFIG_VIDEO_MAX96724_INIT_PRIORITY,
	     "the deserializer gate must initialize after the deserializer");
#ifdef CONFIG_VIDEO_MAX96717_INIT_PRIORITY
BUILD_ASSERT(CONFIG_I2C_GMSL_GATE_MAX96724_INIT_PRIORITY < CONFIG_VIDEO_MAX96717_INIT_PRIORITY,
	     "the deserializer gate must initialize before the serializer it carries");
#endif
DT_FOREACH_STATUS_OKAY_VARGS(adi_max96724_i2c_gate, I2C_GMSL_GATE_DEFINE,
			     CONFIG_I2C_GMSL_GATE_MAX96724_INIT_PRIORITY,
			     CONFIG_I2C_GMSL_GATE_RETRY_NUM)
#endif

/* The serializer's gates: plain forwarders, the deserializer's gate below them retries. */
#if DT_HAS_COMPAT_STATUS_OKAY(adi_max96717_i2c_gate) && defined(CONFIG_VIDEO_MAX96717)
BUILD_ASSERT(CONFIG_I2C_GMSL_GATE_MAX96717_INIT_PRIORITY > CONFIG_VIDEO_MAX96717_INIT_PRIORITY,
	     "the serializer gate must initialize after the serializer");
BUILD_ASSERT(CONFIG_I2C_GMSL_GATE_MAX96717_INIT_PRIORITY < CONFIG_VIDEO_INIT_PRIORITY,
	     "the serializer gate must initialize before the camera sensors it carries");
DT_FOREACH_STATUS_OKAY_VARGS(adi_max96717_i2c_gate, I2C_GMSL_GATE_DEFINE,
			     CONFIG_I2C_GMSL_GATE_MAX96717_INIT_PRIORITY, 0)
#endif
