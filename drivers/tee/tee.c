/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/tee.h>

int tee_add_shm(const struct device *dev, void *addr, size_t align, size_t size,
		uint32_t flags, struct tee_shm **shmp)
{
	int rc;
	void *p = addr;
	struct tee_shm *shm;

	if (!shmp) {
		return -EINVAL;
	}

	if (flags & TEE_SHM_ALLOC) {
		if (align) {
			p = k_aligned_alloc(align, size);
		} else {
			p = k_malloc(size);
		}
	}

	if (!p) {
		return -ENOMEM;
	}

	shm = k_malloc(sizeof(struct tee_shm));
	if (!shm) {
		rc = -ENOMEM;
		goto err;
	}

	shm->addr = p;
	shm->size = size;
	shm->flags = flags;
	shm->dev = dev;

	if (flags & TEE_SHM_REGISTER) {
		const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

		if (!api->shm_register) {
			rc = -ENOSYS;
			goto err;
		}

		rc = api->shm_register(dev, shm);
		if (rc) {
			goto err;
		}
	}

	*shmp = shm;

	return 0;
err:
	k_free(shm);
	if (flags & TEE_SHM_ALLOC) {
		k_free(p);
	}

	return rc;
}

int tee_rm_shm(const struct device *dev, struct tee_shm *shm)
{
	int rc = 0;

	if (!shm) {
		return -EINVAL;
	}

	if (shm->flags & TEE_SHM_REGISTER) {
		const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

		if (api->shm_unregister) {
			/*
			 * We don't return immediately if callback returned error,
			 * just return this code after cleanup.
			 */
			rc = api->shm_unregister(dev, shm);
		} else {
			/*
			 * Set ENOSYS is SHM_REGISTER flag was set, but callback
			 * is not set.
			 */
			rc = -ENOSYS;
		}
	}

	if (shm->flags & TEE_SHM_ALLOC) {
		k_free(shm->addr);
	}

	k_free(shm);

	return rc;
}
