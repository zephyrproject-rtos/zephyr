/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT root

#include <zephyr/platform.h>


#define LOG_LEVEL CONFIG_PLATFORM_ABST_SUPPORT_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(platform_abst);

#if defined(CONFIG_PLATFORM_ABST_SUPPORT)
__attribute__((weak)) int platform_specific_dev_config(const struct device *dev,
		struct platform_resource *res)
{
	const struct platform_config  *platform_cfg = dev->platform_cfg;

	LOG_DBG("");

	if (!res->io_mapped) {
#if defined(CONFIG_MMU)
		if (dev->flag & DEV_FLAG_BUS_ADD) {
			dev->mmio->mem = res->reg_base_phy;
		} else if (res->reg_base_phy) {
			device_map(&dev->mmio->mem, res->reg_base_phy,
			   res->reg_size, K_MEM_CACHE_NONE);
		}
#else
		dev->mmio->mem = res->reg_base_phy;
#endif
		LOG_DBG("memory mapped: %p\n", (void *)dev->mmio->mem);
	} else {
		dev->mmio->port =  res->port;
		LOG_DBG("IO Mapped:%x\n", dev->mmio->port);
	}
	if (platform_cfg->isr) {
		LOG_DBG("irq:%d, prio:%x, flag:%x\n", res->irq_num, res->irq_prio, res->irq_flag);
		irq_connect_dynamic(res->irq_num, res->irq_prio,
			    platform_cfg->isr, dev, res->irq_flag);
		irq_enable(res->irq_num);
	}

	return 0;
}

__attribute__((weak)) int platform_specific_dev_update(const struct device *dev,
			struct platform_resource *res,  int status)
{
	LOG_DBG("");

	if (status != 0) {
		if (status < 0) {
			status = -status;
		}
		if (status > UINT8_MAX) {
			status = UINT8_MAX;
		}
		dev->state->init_res = status;
	}

	dev->state->initialized = true;

	return 0;
}

void platform_node_init(const struct init_entry *entry)
{
	const struct device *dev;
 
	if (!entry || !entry->dev || !entry->init_fn.dev) {
		return;
	}

	dev = entry->dev;

	if (((dev->flag & BOOT_DTS_DEVICE) != BOOT_DTS_DEVICE) ||
		(dev->state->initialized == true) ||
		(BOOT_FLAG_LEVEL(dev) > current_boot_stage_get())) {
		return;
	}

	entry->init_fn.dev(dev);
}

static void prob_child_device(void)
{
	DEVICE_PROB_CHILD_NODE(root);
}

static void platform_boot_notfy(enum notify_boot_level)
{
	prob_child_device();
}

static int platform_init(const struct device *dev)
{
	device_boot_notify_register(NOTIFY_LEVEL_POST_KERNEL, platform_boot_notfy);
	device_boot_notify_register(NOTIFY_LEVEL_APPLICATION, platform_boot_notfy);

	prob_child_device();

	return 0;
}

/* we need to define device object for root node as well. */
DEVICE_DT_INST_DEFINE(0, platform_init, NULL, NULL, NULL, POST_KERNEL, 0, NULL);

#endif
