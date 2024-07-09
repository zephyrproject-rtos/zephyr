/*
 * Copyright (c) 2017-2024 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementations for FLASH driver APIs.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FLASH_H_
#error "Should only be included by zephyr/drivers/flash.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_FLASH_INTERNAL_FLASH_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_FLASH_INTERNAL_FLASH_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

static inline int z_impl_flash_read(const struct device *dev, off_t offset,
				    void *data,
				    size_t len)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	return api->read(dev, offset, data, len);
}

static inline int z_impl_flash_write(const struct device *dev, off_t offset,
				     const void *data, size_t len)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;
	int rc;

	rc = api->write(dev, offset, data, len);

	return rc;
}

static inline int z_impl_flash_erase(const struct device *dev, off_t offset,
				     size_t size)
{
	int rc = -ENOSYS;

	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	if (api->erase != NULL) {
		rc = api->erase(dev, offset, size);
	}

	return rc;
}

#if defined(CONFIG_FLASH_JESD216_API)
static inline int z_impl_flash_sfdp_read(const struct device *dev,
					 off_t offset,
					 void *data, size_t len)
{
	int rv = -ENOTSUP;
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	if (api->sfdp_read != NULL) {
		rv = api->sfdp_read(dev, offset, data, len);
	}
	return rv;
}

static inline int z_impl_flash_read_jedec_id(const struct device *dev,
					     uint8_t *id)
{
	int rv = -ENOTSUP;
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	if (api->read_jedec_id != NULL) {
		rv = api->read_jedec_id(dev, id);
	}
	return rv;
}
#endif /* CONFIG_FLASH_JESD216_API */

static inline size_t z_impl_flash_get_write_block_size(const struct device *dev)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	return api->get_parameters(dev)->write_block_size;
}

static inline const struct flash_parameters *z_impl_flash_get_parameters(const struct device *dev)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	return api->get_parameters(dev);
}

static inline int z_impl_flash_ex_op(const struct device *dev, uint16_t code,
				     const uintptr_t in, void *out)
{
#if defined(CONFIG_FLASH_EX_OP_ENABLED)
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;

	if (api->ex_op == NULL) {
		return -ENOTSUP;
	}

	return api->ex_op(dev, code, in, out);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(code);
	ARG_UNUSED(in);
	ARG_UNUSED(out);

	return -ENOSYS;
#endif /* CONFIG_FLASH_EX_OP_ENABLED */
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_FLASH_INTERNAL_FLASH_IMPL_H_ */
