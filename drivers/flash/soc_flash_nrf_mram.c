/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/cache.h>
#include <ironside/se/versions.h>
#if defined(CONFIG_MRAM_LATENCY)
#include "../soc/nordic/common/mram_latency.h"
#endif
#if defined(CONFIG_HAS_IRONSIDE_SE)
#include <ironside/se/boot_report.h>
#endif
#include <hal/nrf_cache.h>
#include <hal/nrf_mvdma.h>
#include <haly/nrfy_mvdma.h>

LOG_MODULE_REGISTER(flash_nrf_mram, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_mram

#define MRAM_START DT_INST_REG_ADDR(0)
#define MRAM_SIZE  DT_INST_REG_SIZE(0)

#define MRAM_WORD_SIZE 16
#define MRAM_WORD_MASK 0xf

#define WRITE_BLOCK_SIZE DT_INST_PROP_OR(0, write_block_size, MRAM_WORD_SIZE)
#define ERASE_BLOCK_SIZE DT_INST_PROP_OR(0, erase_block_size, WRITE_BLOCK_SIZE)

#if (WRITE_BLOCK_SIZE & MRAM_WORD_MASK)
#warning "write-block-size is not a multiple of 16 bytes. \
This may lead to data corruption in corner cases where partial writes to MRAM words are needed. \
If a partial write fails and a read-modify-write is needed for recovery, the read may return \
corrupted data due to the previous corrupted write, which can cause the recovery to fail as well."
#endif

#define ERASE_VALUE 0xff

#define SOC_NRF_MRAM_BANK_11_OFFSET  (MRAM_SIZE / 2)
#define SOC_NRF_MRAM_BANK_11_ADDRESS (MRAM_START + SOC_NRF_MRAM_BANK_11_OFFSET)

#define IRONSIDE_SE_SUPPORT_READY_VER IRONSIDE_SE_V23_1_2_21

BUILD_ASSERT(MRAM_START > 0, "nordic,mram: start address expected to be non-zero");
BUILD_ASSERT((ERASE_BLOCK_SIZE % WRITE_BLOCK_SIZE) == 0,
	     "erase-block-size expected to be a multiple of write-block-size");

/** MVDMA Attribute field offset. */
#define NRF_MVDMA_ATTR_OFF 24

/** MVDMA Extended attribute field offset. */
#define NRF_MVDMA_EXT_ATTR_OFF 30

/** @brief Default transfer. */
#define NRF_MVDMA_ATTR_DEFAULT 7

#define NRF_MVDMA_JOB_ATTR(_size, _attr, _ext_attr)                                                \
	(((_size) & 0xFFFFFF) | ((_attr) << NRF_MVDMA_ATTR_OFF) |                                  \
	 ((_ext_attr) << NRF_MVDMA_EXT_ATTR_OFF))

#define NRF_MVDMA_BASIC_INIT(_src, _src_len, _src_attr, _src_ext_attr, _sink, _sink_len,           \
			     _sink_attr, _sink_ext_attr)                                           \
	{                                                                                          \
		.source = (uint32_t)(_src),                                                        \
		.source_attr_len = NRF_MVDMA_JOB_ATTR((_src_len), _src_attr, _src_ext_attr),       \
		.source_terminate = 0,                                                             \
		.source_padding = 0,                                                               \
		.sink = (uint32_t)(_sink),                                                         \
		.sink_attr_len = NRF_MVDMA_JOB_ATTR((_sink_len), _sink_attr, _sink_ext_attr),      \
		.sink_terminate = 0,                                                               \
		.sink_padding = 0,                                                                 \
	}

#define NRF_MVDMA_BASIC_MEMCPY_INIT(_dst, _src, _len)                                              \
	NRF_MVDMA_BASIC_INIT(_src, _len, NRF_MVDMA_ATTR_DEFAULT, 0, _dst, _len,                    \
			     NRF_MVDMA_ATTR_DEFAULT, 0)

struct mvdma_basic_desc {
	uint32_t source;
	uint32_t source_attr_len;
	uint32_t source_terminate;
	uint32_t source_padding;
	uint32_t sink;
	uint32_t sink_attr_len;
	uint32_t sink_terminate;
	uint32_t sink_padding;
};

struct nrf_mram_data_t {
	uint8_t ironside_se_ver;
	struct k_mutex nrf_mram_mutex;
};

/**
 * @brief Verify MRAM word using MVDMA read operation.
 *
 * Uses the MVDMA peripheral to read one MRAM_WORD_SIZE (16 bytes) from the
 * specified address. This operation serves as a verification mechanism after
 * write/erase operations by checking if a bus fault occurs or if the operation
 * completes successfully.
 *
 * @param addr The MRAM address to read from.
 * @return 0 on success, -EIO if a bus error is detected.
 */
static int nrf_mram_mvdma_read(uint32_t addr)
{
	uint8_t rbuf[MRAM_WORD_SIZE];
	struct mvdma_basic_desc desc __aligned(4) =
		NRF_MVDMA_BASIC_MEMCPY_INIT(rbuf, addr, MRAM_WORD_SIZE);

	sys_cache_data_flush_range(&desc, sizeof(desc));
	nrf_mvdma_event_clear(NRF_MVDMA, NRF_MVDMA_EVENT_COMPLETED0);
	nrf_mvdma_source_list_ptr_set(NRF_MVDMA, (void *)&desc.source);
	nrf_mvdma_sink_list_ptr_set(NRF_MVDMA, (void *)&desc.sink);
	nrf_mvdma_task_trigger(NRF_MVDMA, NRF_MVDMA_TASK_START0);
	while (!nrf_mvdma_event_check(NRF_MVDMA, NRF_MVDMA_EVENT_COMPLETED0)) {
		/* Wait until transfer is complete */
		if (nrf_mvdma_event_check(NRF_MVDMA, NRF_MVDMA_EVENT_SOURCEBUSERROR)) {
			nrf_mvdma_event_clear(NRF_MVDMA, NRF_MVDMA_EVENT_SOURCEBUSERROR);
			LOG_ERR("MVDMA source bus error");
			nrfy_mvdma_reset(NRF_MVDMA, true);
			return -EIO;
		}
	}

	return 0;
}

static int nrf_mram_write_and_verify(uint32_t addr, const void *data, size_t len)
{
	uint8_t retries = CONFIG_NRF_MRAM_MAX_RETRIES;
#if (WRITE_BLOCK_SIZE & MRAM_WORD_MASK)
	uint8_t rbuf[MRAM_WORD_SIZE];

	if (len % MRAM_WORD_SIZE) {
		/* For partial word writes, we need to read the existing data first to avoid
		 * overwriting the unwritten bytes in the same word.
		 */
		memcpy(rbuf + len, (void *)(addr + len), MRAM_WORD_SIZE - len);
		memcpy(rbuf, data, len);
		data = rbuf;
		len = MRAM_WORD_SIZE;
	}
#endif

	while (retries--) {
		memcpy((void *)addr, data, len);
		if (!nrf_mram_mvdma_read(addr)) {
			return 0;
		}
		LOG_ERR("MRAM write verification failed at address 0x%x, retrying... (%u retries "
			"left)",
			addr, retries);
	}

	return -EIO;
}

static int nrf_mram_erase_and_verify(uint32_t addr, size_t len)
{
	uint8_t retries = CONFIG_NRF_MRAM_MAX_RETRIES;
#if (WRITE_BLOCK_SIZE & MRAM_WORD_MASK)
	uint8_t rbuf[MRAM_WORD_SIZE];

	if (len % MRAM_WORD_SIZE) {
		/* For partial word writes, we need to read the existing data first to avoid
		 * overwriting the unwritten bytes in the same word.
		 */
		memcpy(rbuf, (void *)(addr + len), MRAM_WORD_SIZE - len);
	}
#endif

	while (retries--) {
		memset((void *)addr, ERASE_VALUE, len);
#if (WRITE_BLOCK_SIZE & MRAM_WORD_MASK)
		if (len % MRAM_WORD_SIZE) {
			memcpy((void *)addr, rbuf, MRAM_WORD_SIZE - len);
		}
#endif
		if (!nrf_mram_mvdma_read(addr)) {
			return 0;
		}
		LOG_ERR("MRAM erase verification failed at address 0x%x, retrying... (%u retries "
			"left)",
			addr, retries);
	}

	return -EIO;
}

#ifdef CONFIG_MRAM_LATENCY
static inline bool nrf_mram_ready(uint32_t addr, uint8_t ironside_se_ver)
{
	if (ironside_se_ver < IRONSIDE_SE_SUPPORT_READY_VER) {
		return true;
	}

	if (addr < SOC_NRF_MRAM_BANK_11_ADDRESS) {
		return (bool)NRF_MRAMC110->READY;
	} else {
		return (bool)NRF_MRAMC111->READY;
	}
}
#else
static inline bool nrf_mram_ready(uint32_t addr, uint8_t ironside_se_ver)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(ironside_se_ver);
	return true;
}
#endif

/**
 * @param[in,out] offset      Relative offset into memory, from the driver API.
 * @param[in]     len         Number of bytes for the intended operation.
 * @param[in]     must_align  Require MRAM word alignment, if applicable.
 *
 * @return Absolute address in MRAM, or NULL if @p offset or @p len are not
 *         within bounds or appropriately aligned.
 */
static uintptr_t validate_and_map_addr(off_t offset, size_t len, bool must_align)
{
	if (unlikely(offset < 0 || offset >= MRAM_SIZE || len > MRAM_SIZE - offset)) {
		LOG_ERR("invalid offset: %ld:%zu", offset, len);
		return 0;
	}

	const uintptr_t addr = MRAM_START + offset;

	if ((WRITE_BLOCK_SIZE > 1) && must_align &&
	    unlikely((addr % WRITE_BLOCK_SIZE) != 0 || (len % WRITE_BLOCK_SIZE) != 0)) {
		LOG_ERR("invalid alignment: %p:%zu", (void *)addr, len);
		return 0;
	}

	return addr;
}

static int nrf_mram_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	ARG_UNUSED(dev);

	const uintptr_t addr = validate_and_map_addr(offset, len, false);

	if (!addr) {
		return -EINVAL;
	}

	LOG_DBG("read: %p:%zu", (void *)addr, len);

	memcpy(data, (void *)addr, len);

	return 0;
}

static int nrf_mram_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct nrf_mram_data_t *nrf_mram_data = dev->data;
	uint8_t ironside_se_ver = nrf_mram_data->ironside_se_ver;
	int ret = 0;

	const uintptr_t addr = validate_and_map_addr(offset, len, true);

	if (!addr) {
		return -EINVAL;
	}

	LOG_DBG("write: %p:%zu", (void *)addr, len);

	k_mutex_lock(&nrf_mram_data->nrf_mram_mutex, K_FOREVER);

	if (ironside_se_ver >= IRONSIDE_SE_SUPPORT_READY_VER) {
#if defined(CONFIG_MRAM_LATENCY)
		mram_no_latency_sync_request();
#endif
	}

	for (uint32_t i = 0; i < (len / MRAM_WORD_SIZE); i++) {
		while (!nrf_mram_ready(addr + (i * MRAM_WORD_SIZE), ironside_se_ver)) {
			/* Wait until MRAM controller is ready */
		}
		ret = nrf_mram_write_and_verify(addr + (i * MRAM_WORD_SIZE),
						(void *)((uintptr_t)data + (i * MRAM_WORD_SIZE)),
						MRAM_WORD_SIZE);
		if (ret) {
			goto unlock;
		}
	}
#if (WRITE_BLOCK_SIZE & MRAM_WORD_MASK)
	if (len % MRAM_WORD_SIZE) {
		while (!nrf_mram_ready(addr + (len & ~MRAM_WORD_MASK), ironside_se_ver)) {
			/* Wait until MRAM controller is ready */
		}
		ret = nrf_mram_write_and_verify(addr + (len & ~MRAM_WORD_MASK),
						(void *)((uintptr_t)data + (len & ~MRAM_WORD_MASK)),
						len & MRAM_WORD_MASK);
		if (ret) {
			goto unlock;
		}
	}
#endif
	if (ironside_se_ver >= IRONSIDE_SE_SUPPORT_READY_VER) {
#if defined(CONFIG_MRAM_LATENCY)
		mram_no_latency_sync_release();
#endif
	}

unlock:
	k_mutex_unlock(&nrf_mram_data->nrf_mram_mutex);
	return ret;
}

static int nrf_mram_erase(const struct device *dev, off_t offset, size_t size)
{
	struct nrf_mram_data_t *nrf_mram_data = dev->data;
	uint8_t ironside_se_ver = nrf_mram_data->ironside_se_ver;
	int ret = 0;

	const uintptr_t addr = validate_and_map_addr(offset, size, true);

	if (!addr) {
		return -EINVAL;
	}

	LOG_DBG("erase: %p:%zu", (void *)addr, size);

	k_mutex_lock(&nrf_mram_data->nrf_mram_mutex, K_FOREVER);

	/* Ensure that the mramc banks are powered on */
	if (ironside_se_ver >= IRONSIDE_SE_SUPPORT_READY_VER) {
#if defined(CONFIG_MRAM_LATENCY)
		mram_no_latency_sync_request();
#endif
	}
	for (uint32_t i = 0; i < (size / MRAM_WORD_SIZE); i++) {
		while (!nrf_mram_ready(addr + (i * MRAM_WORD_SIZE), ironside_se_ver)) {
			/* Wait until MRAM controller is ready */
		}
		ret = nrf_mram_erase_and_verify(addr + (i * MRAM_WORD_SIZE), MRAM_WORD_SIZE);
		if (ret) {
			goto unlock;
		}
	}

#if (WRITE_BLOCK_SIZE & MRAM_WORD_MASK)
	if (size % MRAM_WORD_SIZE) {
		while (!nrf_mram_ready(addr + (size & ~MRAM_WORD_MASK), ironside_se_ver)) {
			/* Wait until MRAM controller is ready */
		}
		ret = nrf_mram_erase_and_verify(addr + (size & ~MRAM_WORD_MASK),
						size & MRAM_WORD_MASK);
		if (ret) {
			goto unlock;
		}
	}
#endif

	if (ironside_se_ver >= IRONSIDE_SE_SUPPORT_READY_VER) {
#if defined(CONFIG_MRAM_LATENCY)
		mram_no_latency_sync_release();
#endif
	}

unlock:
	k_mutex_unlock(&nrf_mram_data->nrf_mram_mutex);
	return ret;
}

static int nrf_mram_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = MRAM_SIZE;

	return 0;
}

static const struct flash_parameters *nrf_mram_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters parameters = {
		.write_block_size = WRITE_BLOCK_SIZE,
		.erase_value = ERASE_VALUE,
		.caps = {
			.no_explicit_erase = true,
		},
	};

	return &parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void nrf_mram_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	ARG_UNUSED(dev);

	static const struct flash_pages_layout pages_layout = {
		.pages_count = (MRAM_SIZE) / (ERASE_BLOCK_SIZE),
		.pages_size = ERASE_BLOCK_SIZE,
	};

	*layout = &pages_layout;
	*layout_size = 1;
}
#endif

static DEVICE_API(flash, nrf_mram_api) = {
	.read = nrf_mram_read,
	.write = nrf_mram_write,
	.erase = nrf_mram_erase,
	.get_size = nrf_mram_get_size,
	.get_parameters = nrf_mram_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = nrf_mram_page_layout,
#endif
};

static int nrf_mram_init(const struct device *dev)
{
	struct nrf_mram_data_t *nrf_mram_data = dev->data;
#if defined(CONFIG_HAS_IRONSIDE_SE) && defined(IRONSIDE_SE_BOOT_REPORT)
	nrf_mram_data->ironside_se_ver = IRONSIDE_SE_BOOT_REPORT->ironside_se_version_int;
#else
	nrf_mram_data->ironside_se_ver = 0;
#endif
	LOG_DBG("Ironside SE version: %u", nrf_mram_data->ironside_se_ver);

	k_mutex_init(&nrf_mram_data->nrf_mram_mutex);

	return 0;
}

static struct nrf_mram_data_t nrf_mram_data;

DEVICE_DT_INST_DEFINE(0, nrf_mram_init, NULL, &nrf_mram_data, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &nrf_mram_api);
