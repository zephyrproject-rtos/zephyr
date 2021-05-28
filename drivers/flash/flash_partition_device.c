/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT fixed_partitions

/*
 * The flash partition device is not a real device, this is range checker over real flash device.
 */

#include <device.h>
#include <drivers/flash.h>
#include "flash_partition_device_priv.h"
#include <zephyr/types.h>
#include <init.h>
#include <kernel.h>
#include <sys/util.h>

#define FPD(fpdp) ((const struct flash_partition_device *)fpdp)
#define FPD_PRIV(fpdp) ((struct flash_partition_device_priv *)fpdp)
#define FPD_SIZE(dev) (FPD(dev->config)->size)
#define FPD_PAGE_COUNT(dev) (FPD_PRIV(dev->data)->page_count)
#define FPD_OFFSET(dev, change) (FPD(dev->config)->offset + (change))
#define FPD_PARAMETERS(dev) (FPD_PRIV(dev->data)->parameters)
#define FPD_REAL_DEV(dev) (FPD(dev->config)->real_dev)

#define FLASH_API(dev) ((const struct flash_driver_api *)dev->api)

#define FLASH_API_CALL(dev, ...) \
	FLASH_API(dev)->GET_ARG_N(1, __VA_ARGS__)(dev, GET_ARGS_LESS_N(1, __VA_ARGS__))

static inline bool is_within_fpd_range(const struct device *dev, off_t offset, size_t len)
{
	return (offset >= 0) && ((offset + len) <= FPD_SIZE(dev));
}

static int fpd_init(const struct device *dev)
{
	struct flash_page_info pi;
	int ret_code = 0;
	off_t test_offset = FPD_OFFSET(dev, 0);		/* This is offset on real device */
	const struct flash_parameters *real_dev_params = flash_get_parameters(FPD_REAL_DEV(dev));

	/* Check if device's start offset is aligned with flash page start */
	ret_code = FLASH_API_CALL(FPD_REAL_DEV(dev), get_page_info, test_offset, &pi);
	if (ret_code != 0) {
		/* TODO: Log error, page is not aligned with real device page */
		return ret_code;
	}

	FPD_PARAMETERS(dev).erase_value = real_dev_params->erase_value;
	FPD_PARAMETERS(dev).write_block_size = real_dev_params->write_block_size;
	FPD_PARAMETERS(dev).flags = 0;

	if (!(real_dev_params->flags & FPF_NON_UNIFORM_LAYOUT)) {
		const size_t real_max_page_size = real_dev_params->max_page_size;

		if (FPD_SIZE(dev) % real_max_page_size) {
			/* TODO: Log error, page is not aligned with real device page */
			return -ERANGE;
		}

		FPD_PARAMETERS(dev).max_page_size = real_max_page_size;
		FPD_PAGE_COUNT(dev) = FPD_SIZE(dev) / real_max_page_size;

	} else {
		/* Check if can read last page of the area */
		off_t end_offset = test_offset + FPD_SIZE(dev);
		struct flash_page_info end_pi;

		ret_code = FLASH_API_CALL(FPD_REAL_DEV(dev), get_page_info, end_offset, &end_pi);

		if (ret_code != 0) {
			/* TODO: Log error, probably passing past the device */
			return ret_code;
		}
		/* Check area size is aligned to last page */
		if ((test_offset + FPD_SIZE(dev)) != (end_pi.offset  + end_pi.size)) {
			return -ERANGE;
		}

		/*
		 * The first page already read, so there is at least one, prepare offset for next
		 * one, and it is largest page found so far
		 */
		FPD_PAGE_COUNT(dev) = 1;
		test_offset += pi.size;
		FPD_PARAMETERS(dev).max_page_size = pi.size;

		while (FPD_OFFSET(dev, test_offset) < FPD_SIZE(dev)) {
			ret_code = FLASH_API_CALL(FPD_REAL_DEV(dev), get_page_info, test_offset,
						  &pi);

			if (ret_code != 0) {
				/* TODO: Log error, page is not aligned with real device page */
				return ret_code;
			}

			FPD_PAGE_COUNT(dev)++;

			test_offset = pi.offset + pi.size;

			/*
			 * If current largest page differs from current page, we have non-uniform
			 * layout.
			 */
			if (FPD_PARAMETERS(dev).max_page_size != pi.size) {
				FPD_PARAMETERS(dev).flags |= FPF_NON_UNIFORM_LAYOUT;
			}

			/* New largest page? */
			if (pi.size > FPD_PARAMETERS(dev).max_page_size) {
				FPD_PARAMETERS(dev).max_page_size = pi.size;
			}
		};
	}

	return 0;
}

static int fpd_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	if (is_within_fpd_range(dev, offset, len)) {
		return FLASH_API_CALL(FPD_REAL_DEV(dev), write, FPD_OFFSET(dev, offset), data, len);
	}
	return -EINVAL;
}

static int fpd_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	if (is_within_fpd_range(dev, offset, len)) {
		return FLASH_API_CALL(FPD_REAL_DEV(dev), read, FPD_OFFSET(dev, offset), data, len);
	}

	return -EINVAL;
}

static int fpd_erase(const struct device *dev, off_t offset, size_t len)
{
	if (is_within_fpd_range(dev, offset, len)) {
		return FLASH_API_CALL(FPD_REAL_DEV(dev), erase, FPD_OFFSET(dev, offset), len);
	}

	return -EINVAL;
}

static int fpd_get_page_info(const struct device *dev, off_t offset, struct flash_page_info *fpi)
{
	int ret = -EINVAL;

	if (is_within_fpd_range(dev, offset, 1)) {
		/* Get page info for the real device */
		ret = FLASH_API_CALL(FPD_REAL_DEV(dev), get_page_info, FPD_OFFSET(dev, offset),
					 fpi);
		if (ret == 0) {
			/* Convert the offset to partition device */
			fpi->offset -= FPD_OFFSET(dev, 0);
		}
	}

	return ret;
}

static ssize_t fpd_get_size(const struct device *dev)
{
	return FPD_SIZE(dev);
}

static ssize_t fpd_get_page_count(const struct device *dev)
{
	return FPD_PAGE_COUNT(dev);
}

static const struct flash_parameters *fpd_get_parameters(const struct device *dev)
{
	return &FPD_PARAMETERS(dev);
}

static const struct flash_driver_api fpd_api = {
	.read = fpd_read,
	.write = fpd_write,
	.erase = fpd_erase,
	.get_parameters = fpd_get_parameters,
	.get_page_info = fpd_get_page_info,
	.get_page_count = fpd_get_page_count,
	.get_size = fpd_get_size,
};

#ifdef CONFIG_FLASH_PARTITION_GENERATE_DEVICE_AT_STARTUP
#define FLASH_AREA_DEVICE_GEN(node)							\
	static struct flash_partition_device_priv fpd_priv_##node;				\
	static const struct flash_partition_device fpd_##node = {				\
		.real_dev = DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(node)),		\
		.offset = DT_REG_ADDR(node),						\
		.size = DT_REG_SIZE(node),						\
	};										\
	DEVICE_DT_DEFINE(node, fpd_init, NULL, &fpd_priv_##node, &fpd_##node, POST_KERNEL,	\
			 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fpd_api);

#define FOREACH_PARTITION(n) DT_FOREACH_CHILD(DT_DRV_INST(n), FLASH_AREA_DEVICE_GEN)

DT_INST_FOREACH_STATUS_OKAY(FOREACH_PARTITION);
#endif /* CONFIG_FLASH_PARTITION_GENERATE_DEVICE_AT_STARTUP */
