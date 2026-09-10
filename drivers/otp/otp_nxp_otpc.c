/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#define DT_DRV_COMPAT nxp_otpc

LOG_MODULE_REGISTER(nxp_otpc, CONFIG_OTP_LOG_LEVEL);

#define OTPC_READ_TIMEOUT_US 1000U

/* RWC[ADDR] is a 7-bit field, so at most 128 fuse words are addressable. */
#define OTPC_MAX_WORDS (OTPC_RWC_ADDR_MASK + 1U)

struct nxp_otpc_config {
	DEVICE_MMIO_ROM;
};

struct nxp_otpc_data {
	DEVICE_MMIO_RAM;
	size_t size;
};

static K_MUTEX_DEFINE(nxp_otpc_mutex);

static inline OTPC_Type *nxp_otpc_get_base(const struct device *dev)
{
	return (OTPC_Type *)DEVICE_MMIO_GET(dev);
}

static inline void nxp_otpc_lock(void)
{
	if (!k_is_pre_kernel()) {
		(void)k_mutex_lock(&nxp_otpc_mutex, K_FOREVER);
	}
}

static inline void nxp_otpc_unlock(void)
{
	if (!k_is_pre_kernel()) {
		(void)k_mutex_unlock(&nxp_otpc_mutex);
	}
}

static bool nxp_otpc_range_is_valid(const struct device *dev, off_t offset, size_t len)
{
	const struct nxp_otpc_data *data = dev->data;
	size_t start;

	if (offset < 0) {
		return false;
	}

	start = (size_t)offset;

	return start <= data->size && len <= data->size - start;
}

/* Read one 32-bit fuse word at word index. Must be called with the lock held. */
static int nxp_otpc_read_word(OTPC_Type *base, uint32_t idx, uint32_t *out)
{
	k_timepoint_t timeout;

	timeout = sys_timepoint_calc(K_USEC(OTPC_READ_TIMEOUT_US));
	while (base->SR & OTPC_SR_BUSY_MASK) {
		if (sys_timepoint_expired(timeout)) {
			return -EBUSY;
		}
		k_busy_wait(1);
	}

	/* Trigger a direct eFuse read at the requested word index. */
	base->RWC = OTPC_RWC_ADDR(idx) | OTPC_RWC_READ_EFUSE_MASK;

	timeout = sys_timepoint_calc(K_USEC(OTPC_READ_TIMEOUT_US));
	while (base->SR & OTPC_SR_BUSY_MASK) {
		if (sys_timepoint_expired(timeout)) {
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	if (base->SR & (OTPC_SR_ERROR_MASK | OTPC_SR_RD_FUSE_LOCK_MASK | OTPC_SR_ECC_DF_MASK)) {
		return -EIO;
	}

	*out = base->RDATA;

	return 0;
}

static int nxp_otpc_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	OTPC_Type *base = nxp_otpc_get_base(dev);
	uint8_t *buf_u8 = buf;
	uint32_t addr, skip;
	int ret = 0;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (len == 0) {
		return 0;
	}

	if (!nxp_otpc_range_is_valid(dev, offset, len)) {
		LOG_ERR("Read out of range (offset=0x%lx len=0x%zx)", offset, len);
		return -EINVAL;
	}

	/* Compute the word index and the byte offset within the first word. */
	addr = (uint32_t)offset / sizeof(uint32_t);
	skip = (uint32_t)offset % sizeof(uint32_t);

	nxp_otpc_lock();

	while (len > 0) {
		uint32_t raw;
		size_t part;

		ret = nxp_otpc_read_word(base, addr, &raw);
		if (ret < 0) {
			LOG_ERR("Fuse read failed at word %u (%d)", addr, ret);
			break;
		}

		part = MIN(len + skip, sizeof(uint32_t)) - skip;
		memcpy(buf_u8, (uint8_t *)&raw + skip, part);

		buf_u8 += part;
		len -= part;
		addr++;

		/* skip only applies to the first word. */
		skip = 0;
	}

	nxp_otpc_unlock();

	return ret;
}

static DEVICE_API(otp, nxp_otpc_api) = {
	.read = nxp_otpc_read,
};

static int nxp_otpc_init(const struct device *dev)
{
	struct nxp_otpc_data *data = dev->data;
	OTPC_Type *base;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	base = nxp_otpc_get_base(dev);

	/* PARAM[NUM_FUSE] reports the size of the fuse array in bytes. */
	data->size = (base->PARAM & OTPC_PARAM_NUM_FUSE_MASK) >> OTPC_PARAM_NUM_FUSE_SHIFT;
	if (data->size == 0U) {
		data->size = OTPC_MAX_WORDS * sizeof(uint32_t);
	}

	return 0;
}

/* The OTPC controller is always a single instance on NXP SoCs. */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Only a single nxp,otpc instance is supported");

static struct nxp_otpc_data nxp_otpc_dev_data;

static const struct nxp_otpc_config nxp_otpc_dev_config = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
};

DEVICE_DT_INST_DEFINE(0, nxp_otpc_init, NULL, &nxp_otpc_dev_data,
		      &nxp_otpc_dev_config, PRE_KERNEL_1,
		      CONFIG_OTP_INIT_PRIORITY, &nxp_otpc_api);
