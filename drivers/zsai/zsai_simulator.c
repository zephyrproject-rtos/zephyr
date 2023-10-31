/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <zephyr/drivers/zsai.h>
#include <zephyr/drivers/zsai_infoword.h>
#include <zephyr/drivers/zsai_ioctl.h>

LOG_MODULE_REGISTER(zsai_sim, CONFIG_ZSAI_SIM_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_zsai_simulator

/* Whether device requires erase or not is indicated by existence of erase-value
 * property.
 */
#define ZSAI_SIM_ERASE_SUPPORT_NEEDED				\
	IS_ENABLED(CONFIG_ZSAI_SIMULATED_DEVICE_WITH_ERASE)

struct zsai_sim_dev_config {
	/* Required by the ZSAI */
	struct zsai_device_generic_config generic;
	uint32_t size;
	/* Currently simulator supports uniform layout only */
#if ZSAI_SIM_ERASE_SUPPORT_NEEDED
	uint32_t erase_block_size;
#endif
	/* Pointer to simulated device buffer */
	uint8_t *buffer;
};

#define ZSAI_SIM_DEV_CONFIG(dev) ((const struct zsai_sim_dev_config *)(dev)->config)
#define ZSAI_SIM_DEV_SIZE(dev) (ZSAI_SIM_DEV_CONFIG(dev)->size)
#define ZSAI_SIM_DEV_BUFFER(de) (ZSAI_SIM_DEV_CONFIG(dev)->buffer)
/* Erase block size */
#define ZSAI_SIM_DEV_EBS(dev) (ZSAI_SIM_DEV_CONFIG(dev)->erase_block_size)
/* Write block size */
#define ZSAI_SIM_DEV_WBS(dev) (ZSAI_WRITE_BLOCK_SIZE(dev)
#define ZSAI_SIM_DT_SIZE(node) DT_PROP(node, simulated_size)

static void *zsai_sim_buffer(const struct device *dev, size_t offset)
{
	return (void *)&(ZSAI_SIM_DEV_BUFFER(dev)[offset]);
}

static int in_range(const struct device *dev, size_t offset, size_t len)
{
	if ((uint32_t)len > ZSAI_SIM_DEV_SIZE(dev)) {
		return 0;
	}

	if ((ZSAI_SIM_DEV_SIZE(dev) - (uint32_t)offset) < (uint32_t)len) {
		return 0;
	}

	return 1;
}

static int zsai_sim_read(const struct device *dev, void *data, size_t offset,
			 size_t len)
{
	if (!in_range(dev, offset, len)) {
		return -EINVAL;
	}

	memcpy(data, zsai_sim_buffer(dev, offset), len);

	return 0;
}

static int zsai_sim_write(const struct device *dev, const void *data,
			  size_t offset, size_t len)
{
	if (!in_range(dev, offset, len)) {
		return -EINVAL;
	}

	if ((offset | len) % ZSAI_WRITE_BLOCK_SIZE(dev)) {
		return -EINVAL;
	}

	memcpy(zsai_sim_buffer(dev, offset), data, len);

	return 0;
}

#if ZSAI_SIM_ERASE_SUPPORT_NEEDED
static int zsai_sim_erase_range(const struct device *dev,
				const struct zsai_ioctl_range *range)
{
	const off_t offset = range->offset;
	const size_t size = range->size;

	if (!in_range(dev, offset, size)) {
		return -EINVAL;
	}

	if ((offset | size) % ZSAI_SIM_DEV_EBS(dev)) {
		return -EINVAL;
	}
	memset(zsai_sim_buffer(dev, offset), ZSAI_ERASE_VALUE(dev), size);

	return 0;
}
#endif

static int zsai_sim_sys_ioctl(const struct device *dev, uint32_t id,
			      const uintptr_t in, uintptr_t in_out)
{
	switch (id) {
	case ZSAI_IOCTL_GET_SIZE: {
		size_t *out = (size_t *)in_out;

		*out = ZSAI_SIM_DEV_SIZE(dev);

		return 0;
	}
#if ZSAI_SIM_ERASE_SUPPORT_NEEDED
	case ZSAI_IOCTL_GET_PAGE_INFO: {
		uint32_t offset = in;
		struct zsai_ioctl_range *pi = (struct zsai_ioctl_range *)in_out;

		/* If device does not require erase, it does not provide page info */
		if (!ZSAI_ERASE_REQUIRED(dev)) {
			return -ENOTSUP;
		}

		if (pi == NULL) {
			return -EINVAL;
		}

		/* The device has unform layout of 4k pages */
		/* This should not be checked at runtime, instead init should take care
		 * of that.
		 */
		pi->offset = offset & ~(ZSAI_SIM_DEV_EBS(dev) - 1);
		pi->size = ZSAI_SIM_DEV_EBS(dev);

		return 0;
	}
#endif
	}
	return -ENOTSUP;
}

const struct zsai_driver_api zsai_sim_api = {
	.read = zsai_sim_read,
	.write = zsai_sim_write,
	.sys_ioctl = zsai_sim_sys_ioctl,
#if ZSAI_SIM_ERASE_SUPPORT_NEEDED
	.erase_range = zsai_sim_erase_range,
#endif
};

static int zsai_sim_init(const struct device *dev)
{
	LOG_DBG("dev %p buffer @%p of size %lu\n", dev, ZSAI_SIM_DEV_BUFFER(dev),
		(unsigned long)ZSAI_SIM_DEV_SIZE(dev));
	return 0;
}

#if ZSAI_SIM_ERASE_SUPPORT_NEEDED
#define MK_ERASE_BLOCK_SIZE(size) .erase_block_size = (size),
#else
#define MK_ERASE_BLOCK_SIZE(size)
#endif

/* The instance parameter in instance number not identifier */
/* Note on ZSAI_MK_INFOWORD parameters: existence of erase-value determines need for erase
 * and also meanst that simulated device provides erase implementation; therefore the
 * DT_NODE_HAS_PROP(DT_DRV_INST(instance), erase_value) is used to set es and er
 * parameters for ZSAI_MK_INFOWORD.
 */
#define ZSAI_DEFINE_SIM_DEV_CONFIG(instance)							\
	struct zsai_sim_dev_config zsai_sim_dev_config_##instance  = {				\
		.generic = {									\
			.infoword = ZSAI_MK_INFOWORD(						\
				DT_PROP_OR(DT_DRV_INST(instance), write_block_size, 1),		\
				DT_NODE_HAS_PROP(DT_DRV_INST(instance), erase_value),		\
				DT_NODE_HAS_PROP(DT_DRV_INST(instance), erase_value),		\
				(DT_PROP_OR(DT_DRV_INST(instance), erase_value, 0) & 1),	\
				/* Unifrom page access is set by need to erase value */		\
				DT_NODE_HAS_PROP(DT_DRV_INST(instance), write_block_size))	\
		},										\
		MK_ERASE_BLOCK_SIZE(DT_PROP_OR(DT_DRV_INST(instance), erase_block_size, 0))	\
		.buffer = &zsai_sim_dev_buffer_##instance[0],					\
		.size = ZSAI_SIM_DT_SIZE(DT_DRV_INST(instance)),				\
	}

#define ZSAI_DEFINE_BUFFER(instance, size)					\
	static uint8_t zsai_sim_dev_buffer_##instance[size];

/* The instance parameter in instance number not identifier */
#define ZSAI_DEFINE_SIM_DEV(instance)							\
	ZSAI_DEFINE_BUFFER(instance, ZSAI_SIM_DT_SIZE(DT_DRV_INST(instance)));		\
	static const ZSAI_DEFINE_SIM_DEV_CONFIG(instance);				\
	DEVICE_DT_INST_DEFINE(instance, zsai_sim_init, NULL, NULL,			\
			      &zsai_sim_dev_config_##instance,				\
			      POST_KERNEL, CONFIG_ZSAI_INIT_PRIORITY, &zsai_sim_api);

/* Define the device for each compatible node in DTS */
DT_INST_FOREACH_STATUS_OKAY(ZSAI_DEFINE_SIM_DEV)
