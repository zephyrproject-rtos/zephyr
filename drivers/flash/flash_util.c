/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>

LOG_MODULE_REGISTER(flash, CONFIG_FLASH_LOG_LEVEL);

int z_impl_flash_fill(const struct device *dev, uint8_t val, k_off_t offset, size_t size)
{
	uint8_t filler[CONFIG_FLASH_FILL_BUFFER_SIZE];
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;
	const struct flash_parameters *fparams = api->get_parameters(dev);
	int rc = 0;
	size_t stored = 0;

	if (sizeof(filler) < fparams->write_block_size) {
		LOG_ERR("Size of CONFIG_FLASH_FILL_BUFFER_SIZE");
		return -EINVAL;
	}
	/* The flash_write will, probably, check write alignment but this
	 * is too late, as we write datain chunks; data alignment may be
	 * broken by the size of the last chunk, that is why the check
	 * happens here too.
	 * Note that we have no way to check whether offset and size are
	 * are correct, as such info is only available at the level of
	 * a driver, so only basic check on offset.
	 */
	if (offset < 0) {
		LOG_ERR("Negative offset not allowed\n");
		return -EINVAL;
	}
	if ((size | (size_t)offset) & (fparams->write_block_size - 1)) {
		LOG_ERR("Incorrect size or offset alignment, expected %zx\n",
			fparams->write_block_size);
		return -EINVAL;
	}

	memset(filler, val, sizeof(filler));

	while (stored < size) {
		size_t chunk = MIN(sizeof(filler), size - stored);

		rc = api->write(dev, offset + stored, filler, chunk);
		if (rc < 0) {
			LOG_DBG("Fill to dev %p failed at offset 0x%zx\n",
				dev, (size_t)offset + stored);
			break;
		}
		stored += chunk;
	}
	return rc;
}

int z_impl_flash_flatten(const struct device *dev, k_off_t offset, size_t size)
{
	const struct flash_driver_api *api =
		(const struct flash_driver_api *)dev->api;
	__maybe_unused const struct flash_parameters *params = api->get_parameters(dev);

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
	if ((flash_params_get_erase_cap(params) & FLASH_ERASE_C_EXPLICIT) &&
		api->erase != NULL) {
		return api->erase(dev, offset, size);
	}
#endif

#if defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
	return flash_fill(dev, params->erase_value, offset, size);
#else
	return -ENOSYS;
#endif
}
