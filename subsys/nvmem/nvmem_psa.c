/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/nvmem/provider.h>
#include <zephyr/sys/minmax.h>

#include <psa/storage_common.h>
#ifdef CONFIG_NVMEM_PSA_ITS
#include <psa/internal_trusted_storage.h>
#endif
#ifdef CONFIG_NVMEM_PSA_PS
#include <psa/protected_storage.h>
#endif

struct nvmem_psa_config {
	psa_storage_uid_t uid_base;
	bool is_ps;
};

static psa_status_t nvmem_psa_get(const struct nvmem_psa_config *config, uint32_t addr, size_t off,
				  size_t len, void *buf, size_t *out_len)
{
	psa_storage_uid_t uid = config->uid_base + addr;

#ifdef CONFIG_NVMEM_PSA_PS
	if (config->is_ps) {
		return psa_ps_get(uid, off, len, buf, out_len);
	}
#endif
#ifdef CONFIG_NVMEM_PSA_ITS
	if (!config->is_ps) {
		return psa_its_get(uid, off, len, buf, out_len);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

static int nvmem_psa_status_to_errno(psa_status_t status)
{
	switch (status) {
	case PSA_SUCCESS:
		return 0;
	case PSA_ERROR_DOES_NOT_EXIST:
		return -ENOENT;
	case PSA_ERROR_NOT_PERMITTED:
		return -EROFS;
	case PSA_ERROR_INVALID_ARGUMENT:
		return -EINVAL;
	case PSA_ERROR_INSUFFICIENT_STORAGE:
		return -ENOSPC;
	case PSA_ERROR_NOT_SUPPORTED:
		return -ENOTSUP;
	default:
		return -EIO;
	}
}

static int nvmem_psa_read(const struct device *dev, uint32_t addr, size_t off, void *data,
			  size_t len)
{
	const struct nvmem_psa_config *config = dev->config;
	size_t out_len;
	psa_status_t status;

	status = nvmem_psa_get(config, addr, off, len, data, &out_len);
	if (status != PSA_SUCCESS) {
		return nvmem_psa_status_to_errno(status);
	}

	/* The cell is a fixed window declared in devicetree; an entry too
	 * short to fill the requested range cannot satisfy the read as the
	 * NVMEM API has no way to return a length.
	 */
	return (out_len == len) ? 0 : -ENODATA;
}

#ifdef CONFIG_NVMEM_PSA_WRITE

static psa_status_t nvmem_psa_set(const struct nvmem_psa_config *config, uint32_t addr, size_t len,
				  const void *buf, psa_storage_create_flags_t flags)
{
	psa_storage_uid_t uid = config->uid_base + addr;

#ifdef CONFIG_NVMEM_PSA_PS
	if (config->is_ps) {
		return psa_ps_set(uid, len, buf, flags);
	}
#endif
#ifdef CONFIG_NVMEM_PSA_ITS
	if (!config->is_ps) {
		return psa_its_set(uid, len, buf, flags);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

static psa_status_t nvmem_psa_get_info(const struct nvmem_psa_config *config, uint32_t addr,
				       struct psa_storage_info_t *info)
{
	psa_storage_uid_t uid = config->uid_base + addr;

#ifdef CONFIG_NVMEM_PSA_PS
	if (config->is_ps) {
		return psa_ps_get_info(uid, info);
	}
#endif
#ifdef CONFIG_NVMEM_PSA_ITS
	if (!config->is_ps) {
		return psa_its_get_info(uid, info);
	}
#endif
	return PSA_ERROR_NOT_SUPPORTED;
}

static K_MUTEX_DEFINE(nvmem_psa_lock);
static uint8_t nvmem_psa_rmw_buf[CONFIG_NVMEM_PSA_WRITE_BUF_SIZE];

static int nvmem_psa_write(const struct device *dev, uint32_t addr, size_t off, const void *data,
			   size_t len)
{
	const struct nvmem_psa_config *config = dev->config;
	struct psa_storage_info_t info;
	psa_status_t status;
	size_t new_size;
	size_t out_len;
	int ret;

	k_mutex_lock(&nvmem_psa_lock, K_FOREVER);

	status = nvmem_psa_get_info(config, addr, &info);
	if (status == PSA_ERROR_DOES_NOT_EXIST) {
		/* Only creating an entry from its start is meaningful. */
		if (off != 0) {
			ret = -ENOENT;
		} else {
			ret = nvmem_psa_status_to_errno(
				nvmem_psa_set(config, addr, len, data, PSA_STORAGE_FLAG_NONE));
		}
		goto out;
	}
	if (status != PSA_SUCCESS) {
		ret = nvmem_psa_status_to_errno(status);
		goto out;
	}

	if (off == 0 && len >= info.size) {
		/* The whole entry is replaced, no need to read it back. */
		ret = nvmem_psa_status_to_errno(nvmem_psa_set(config, addr, len, data, info.flags));
		goto out;
	}

	/* Read-modify-write the whole entry to preserve the stored bytes
	 * outside the written range, including any beyond the cell.
	 */
	new_size = max(info.size, off + len);
	if (new_size > sizeof(nvmem_psa_rmw_buf)) {
		ret = -ENOBUFS;
		goto out;
	}

	status = nvmem_psa_get(config, addr, 0, info.size, nvmem_psa_rmw_buf, &out_len);
	if (status != PSA_SUCCESS) {
		ret = nvmem_psa_status_to_errno(status);
		goto out;
	}
	if (out_len < new_size) {
		memset(&nvmem_psa_rmw_buf[out_len], 0, new_size - out_len);
	}
	memcpy(&nvmem_psa_rmw_buf[off], data, len);

	ret = nvmem_psa_status_to_errno(
		nvmem_psa_set(config, addr, new_size, nvmem_psa_rmw_buf, info.flags));

out:
	/* The shared buffer is long-lived; do not leave a copy of the entry,
	 * which may be sensitive, behind on any return path.
	 */
	memset(nvmem_psa_rmw_buf, 0, sizeof(nvmem_psa_rmw_buf));
	k_mutex_unlock(&nvmem_psa_lock);

	return ret;
}

#endif /* CONFIG_NVMEM_PSA_WRITE */

static DEVICE_API(nvmem, nvmem_psa_api) = {
	.read = nvmem_psa_read,
#ifdef CONFIG_NVMEM_PSA_WRITE
	.write = nvmem_psa_write,
#endif
};

#define NVMEM_PSA_UID_BASE(inst)                                                                   \
	(((uint64_t)DT_INST_PROP_BY_IDX(inst, uid_base, 0) << 32) |                                \
	 (uint64_t)DT_INST_PROP_BY_IDX(inst, uid_base, 1))

#define NVMEM_PSA_DEFINE(inst, name, ps)                                                           \
	BUILD_ASSERT(NVMEM_PSA_UID_BASE(inst) <= (uint64_t)(psa_storage_uid_t)UINT64_MAX,          \
		     "uid-base does not fit the PSA storage UID type");                            \
	static const struct nvmem_psa_config name##_config_##inst = {                              \
		.uid_base = (psa_storage_uid_t)NVMEM_PSA_UID_BASE(inst),                           \
		.is_ps = ps,                                                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &name##_config_##inst, POST_KERNEL,          \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &nvmem_psa_api);

#define DT_DRV_COMPAT zephyr_nvmem_psa_its
#ifdef CONFIG_NVMEM_PSA_ITS
#define NVMEM_PSA_ITS_DEFINE(inst) NVMEM_PSA_DEFINE(inst, nvmem_psa_its, false)
DT_INST_FOREACH_STATUS_OKAY(NVMEM_PSA_ITS_DEFINE)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT zephyr_nvmem_psa_ps
#ifdef CONFIG_NVMEM_PSA_PS
#define NVMEM_PSA_PS_DEFINE(inst) NVMEM_PSA_DEFINE(inst, nvmem_psa_ps, true)
DT_INST_FOREACH_STATUS_OKAY(NVMEM_PSA_PS_DEFINE)
#endif
#undef DT_DRV_COMPAT
