/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/nvmem.h>
#include <zephyr/drivers/nvmem/nvmem_nxp_ocotp.h>

#define DT_DRV_COMPAT nxp_ocotp

LOG_MODULE_REGISTER(nvmem_nxp_ocotp, CONFIG_NVMEM_LOG_LEVEL);

struct nxp_ocotp_cfg {
	uint32_t rom_api_tree_addr;
	uint8_t ocotp_driver_ptr_idx;
};

struct nxp_ocotp_data {
	struct nvmem_info live_info;
};

static int nxp_ocotp_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct nxp_ocotp_cfg *cfg = dev->config;
	uint8_t *dst = (uint8_t *)buf;
	uint32_t word;

	if (!len) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	off_t pos = offset;
	size_t remaining = len;
	volatile const nxp_ocotp_driver_t *rom = NULL;

	if (cfg->rom_api_tree_addr != 0U) {
		const uint32_t *tree = (const uint32_t *)(uintptr_t)cfg->rom_api_tree_addr;
		rom = (const nxp_ocotp_driver_t *)(uintptr_t)tree[cfg->ocotp_driver_ptr_idx];
	}

	while (remaining > 0) {
		uint32_t widx = pos >> 2; /* Convert byte offset to word index. */
		uint32_t within = pos & 0x3; /* Convert byte offset to byte-in-word offset. */
		size_t chunk = MIN(remaining, (size_t)(4 - within));

		if (!rom || !rom->efuse_read) {
			return -ENOSYS;
		}

		int32_t s = rom->efuse_read(widx, &word);

		if (!((s == 0) || (s == (int32_t)0x5ac3c35a))) {
			return -EIO;
		}

		for (size_t i = 0; i < chunk; ++i) {
			*dst++ = (uint8_t)((word >> (8U * (within + i))) & 0xFFU);
		}

		pos += (off_t)chunk;
		remaining -= chunk;
	}

	return 0;
}

static int nxp_ocotp_write(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	/* Default to read-only; enable writes explicitly per-platform */
	if (!IS_ENABLED(CONFIG_NVMEM_NXP_OCOTP_WRITE_ENABLE)) {
		return -EROFS;
	}

	/* Honor provider-level read_only policy as well */
	const struct nvmem_info *pconf = nvmem_get_info(dev);
	if (pconf != NULL && pconf->read_only) {
		return -EROFS;
	}

	const struct nxp_ocotp_cfg *cfg = dev->config;
	const uint8_t *src = (const uint8_t *)buf;
	uint32_t word;

	if (!len) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	off_t pos = offset;
	size_t remaining = len;
	volatile const nxp_ocotp_driver_t *rom = NULL;

	if (cfg->rom_api_tree_addr != 0U) {
		const uint32_t *tree = (const uint32_t *)(uintptr_t)cfg->rom_api_tree_addr;
		rom = (const nxp_ocotp_driver_t *)(uintptr_t)tree[cfg->ocotp_driver_ptr_idx];
	}

	while (remaining > 0) {
		uint32_t widx = pos >> 2; /* Convert byte offset to word index. */
		uint32_t within = pos & 0x3; /* Convert byte offset to byte-in-word offset. */
		size_t chunk = MIN(remaining, (size_t)(4 - within));

		if (!rom || !rom->efuse_read || !rom->efuse_program) {
			return -ENOSYS;
		}

		int32_t s = rom->efuse_read(widx, &word);
		if (!((s == 0) || (s == (int32_t)0x5ac3c35a))) {
			return -EIO;
		}

		uint32_t new_word = word;

		for (size_t i = 0; i < chunk; ++i) {
			uint32_t shift = 8U * (within + i);
			uint32_t mask = 0xFFu << shift;
			uint32_t req_byte = src[i];
			uint32_t cur_byte = (word >> shift) & 0xFFu;
			if ((req_byte | cur_byte) != cur_byte) {
				return -EPERM;
			}
			uint32_t new_byte = cur_byte & req_byte;
			new_word = (new_word & ~mask) | ((new_byte << shift) & mask);
		}

		if (new_word != word) {
			s = rom->efuse_program(widx, new_word);
			if (!((s == 0) || (s == (int32_t)0x5ac3c35a))) {
				return -EIO;
			}
		}

		pos += (off_t)chunk;
		src += chunk;
		remaining -= chunk;
	}

	return 0;
}

static const struct nvmem_info *nxp_ocotp_get_info(const struct device *dev)
{
	struct nxp_ocotp_data *data = dev->data;

	/* The 'read_only' of ocotp can be changed here. For example,
	 * if CONFIG_NVMEM_NXP_OCOTP_WRITE_ENABLE is set, OCOTP may
	 * depend on certain external conditions on some platforms.
	 * We update the 'read_only' here according to these conditions.
	 */

	return &data->live_info;
}

static size_t nxp_ocotp_get_size(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int nxp_ocotp_init(const struct device *dev)
{
	const struct nxp_ocotp_cfg *cfg = dev->config;
	volatile const nxp_ocotp_driver_t *rom = NULL;

	if (cfg->rom_api_tree_addr != 0U) {
		const uint32_t *tree = (const uint32_t *)(uintptr_t)cfg->rom_api_tree_addr;
		rom = (const nxp_ocotp_driver_t *)(uintptr_t)tree[cfg->ocotp_driver_ptr_idx];
	}

	/* Only call init if the ROM driver and its init entrypoint exist. */
	if (rom != NULL && rom->init != NULL) {
		rom->init(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}

	return 0;
}

static DEVICE_API(nvmem, nxp_ocotp_api) = {
	.read = nxp_ocotp_read,
	.write = nxp_ocotp_write,
	.get_size = nxp_ocotp_get_size,
	.get_info = nxp_ocotp_get_info,
};

#define NXP_OCOTP_DEFINE(inst)								\
	static struct nxp_ocotp_data _CONCAT(nxp_ocotp_data, inst) = {			\
		.live_info = {								\
			.type = NVMEM_TYPE_OTP,						\
			.read_only = DT_INST_PROP(inst, read_only) ||			\
				(!IS_ENABLED(CONFIG_NVMEM_NXP_OCOTP_WRITE_ENABLE))	\
		}									\
	};										\
											\
	static const struct nxp_ocotp_cfg _CONCAT(nxp_ocotp_cfg, inst) = {		\
		.rom_api_tree_addr = DT_INST_PROP(inst, rom_api_tree_addr),		\
		.ocotp_driver_ptr_idx = DT_INST_PROP(inst, ocotp_driver_ptr_idx),	\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, nxp_ocotp_init, NULL,				\
		&_CONCAT(nxp_ocotp_data, inst), &_CONCAT(nxp_ocotp_cfg, inst),		\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &nxp_ocotp_api)

DT_INST_FOREACH_STATUS_OKAY(NXP_OCOTP_DEFINE);
