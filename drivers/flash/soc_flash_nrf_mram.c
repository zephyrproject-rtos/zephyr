/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <ironside/se/versions.h>
#if defined(CONFIG_MRAM_LATENCY)
#include "../soc/nordic/common/mram_latency.h"
#endif
#if defined(CONFIG_HAS_IRONSIDE_SE)
#include <ironside/se/boot_report.h>
#endif

LOG_MODULE_REGISTER(flash_nrf_mram, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_mram

#define MRAM_NVM_CHILD_NODE DT_INST_CHILD(0, mram_memory_0)

#define MRAM_START DT_REG_ADDR(MRAM_NVM_CHILD_NODE)
#define MRAM_SIZE  DT_REG_SIZE(MRAM_NVM_CHILD_NODE)

#define MRAM_WORD_SIZE 16
#define MRAM_WORD_MASK 0xf

#define WRITE_BLOCK_SIZE DT_PROP_OR(MRAM_NVM_CHILD_NODE, write_block_size, MRAM_WORD_SIZE)
#define ERASE_BLOCK_SIZE DT_PROP_OR(MRAM_NVM_CHILD_NODE, erase_block_size, WRITE_BLOCK_SIZE)

#define ERASE_VALUE 0xff

#define SOC_NRF_MRAM_BANK_11_ADDRESS (DT_REG_ADDR(DT_NODELABEL(mram11)))

#define IRONSIDE_SE_SUPPORT_READY_VER IRONSIDE_SE_V23_1_2_21

BUILD_ASSERT(MRAM_START > 0, "nordic,mram: start address expected to be non-zero");
BUILD_ASSERT((WRITE_BLOCK_SIZE % MRAM_WORD_SIZE) == 0,
	     "write-block-size expected to be a multiple of MRAM_WORD_SIZE");
BUILD_ASSERT((ERASE_BLOCK_SIZE % WRITE_BLOCK_SIZE) == 0,
	     "erase-block-size expected to be a multiple of write-block-size");

struct nrf_mram_data_t {
	uint32_t ironside_se_ver;
	struct k_mutex nrf_mram_mutex;
};

#if defined(CONFIG_NRF_MRAM_USE_MUTEX_HW) && DT_INST_NODE_HAS_PROP(0, nordic_mutexes)

/* Maps an MRAM bank address range to the hardware mutex that serializes it. */
struct mram_bank_mutex {
	uint32_t start;
	uint32_t end;
	NRF_MUTEX_Type *reg;
	uint8_t index;
};

/* One mutex specifier per bank; each bank selects its mutex via its
 * "nordic,mutex-index" into the controller's "nordic,mutexes" list. Only
 * "nordic,mram-bank" children that define "nordic,mutex-index" are mapped;
 * banks without it (and the partitions child) are left unlocked.
 */
#define MRAM_BANK_MUTEX_ENTRY(node_id)                                                             \
	COND_CODE_1(UTIL_AND(DT_NODE_HAS_COMPAT(node_id, nordic_mram_bank),                        \
			     DT_NODE_HAS_PROP(node_id, nordic_mutex_index)),                       \
		    ({                                                                             \
			     .start = DT_REG_ADDR(node_id),                                        \
			     .end = DT_REG_ADDR(node_id) + DT_REG_SIZE(node_id),                   \
			     .reg = (NRF_MUTEX_Type *)DT_REG_ADDR(DT_INST_PHANDLE_BY_IDX(          \
				     0, nordic_mutexes, DT_PROP(node_id, nordic_mutex_index))),    \
			     .index = DT_INST_PHA_BY_IDX(0, nordic_mutexes,                        \
							 DT_PROP(node_id, nordic_mutex_index),     \
							 index),                                   \
		     },),                                                                          \
		    ())

static const struct mram_bank_mutex mram_bank_mutexes[] = {
	DT_FOREACH_CHILD(MRAM_NVM_CHILD_NODE, MRAM_BANK_MUTEX_ENTRY)
};

BUILD_ASSERT(ARRAY_SIZE(mram_bank_mutexes) == DT_INST_PROP_LEN(0, nordic_mutexes),
	     "nordic,mram: each listed mutex must be claimed by exactly one mram bank");

/* Verify that each claimed mutex index is within its MUTEX instance's range. */
#define MRAM_MUTEX_RANGE_ASSERT(node_id, prop, idx)                                                \
	BUILD_ASSERT(DT_PHA_BY_IDX(node_id, prop, idx, index) <                                    \
			     DT_PROP(DT_PHANDLE_BY_IDX(node_id, prop, idx), nordic_mutex_count),   \
		     "nordic,mram: claimed mutex index is out of range for the MUTEX instance");

DT_INST_FOREACH_PROP_ELEM(0, nordic_mutexes, MRAM_MUTEX_RANGE_ASSERT)

static inline bool hw_mutex_lock(NRF_MUTEX_Type *p_reg, uint8_t mutex)
{
	return (p_reg->MUTEX[mutex] == MUTEX_MUTEX_MUTEX_Unlocked);
}

static inline void hw_mutex_unlock(NRF_MUTEX_Type *p_reg, uint8_t mutex)
{
	p_reg->MUTEX[mutex] = MUTEX_MUTEX_MUTEX_Unlocked;
}

static const struct mram_bank_mutex *nrf_mram_bank_mutex(uint32_t addr)
{
	for (size_t i = 0; i < ARRAY_SIZE(mram_bank_mutexes); i++) {
		if (addr >= mram_bank_mutexes[i].start && addr < mram_bank_mutexes[i].end) {
			return &mram_bank_mutexes[i];
		}
	}

	return NULL;
}

static inline void nrf_mram_hw_lock(uint32_t addr)
{
	const struct mram_bank_mutex *bank = nrf_mram_bank_mutex(addr);

	if (bank == NULL) {
		return;
	}

	while (!hw_mutex_lock(bank->reg, bank->index)) {
		/* Spin until the hardware mutex is acquired by this domain */
	}
}

static inline void nrf_mram_hw_unlock(uint32_t addr)
{
	const struct mram_bank_mutex *bank = nrf_mram_bank_mutex(addr);

	if (bank == NULL) {
		return;
	}

	hw_mutex_unlock(bank->reg, bank->index);
}

#else

static inline void nrf_mram_hw_lock(uint32_t addr)
{
	ARG_UNUSED(addr);
}

static inline void nrf_mram_hw_unlock(uint32_t addr)
{
	ARG_UNUSED(addr);
}

#endif /* CONFIG_NRF_MRAM_USE_MUTEX_HW && DT_INST_NODE_HAS_PROP(0, nordic_mutexes) */

/**
 * Safely probes one MRAM word to detect a BusFault.
 *
 * The function temporarily masks fault handling and ignores BusFault escalation
 * while reading one aligned MRAM word. It then checks and clears BFSR status
 * bits to determine whether the read faulted.
 *
 * @param addr Address to probe. The read is performed on the 16-byte-aligned
 *             base address that contains this location.
 *
 * @retval 0 Read completed without BusFault.
 * @retval non-zero BusFault was observed during the read.
 */
#if defined(CONFIG_CPU_CORTEX_M)
static uint32_t nrf_mram_detect_corrupt_word(uint32_t addr)
{
	uint32_t rdata;
	uint32_t faulted;
	unsigned int irq_lock_key = irq_lock();

	/* Clear any pre-existing BusFault status bits (write-1-to-clear) */
	SCB->CFSR = SCB_CFSR_BUSFAULTSR_Msk;   /* 0x0000FF00 — entire BFSR byte */

	__set_FAULTMASK(1);
	SCB->CCR |= SCB_CCR_BFHFNMIGN_Msk;

	/* Ensure that all operations are in effect before doing the risky read */
	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	/* Read operation that may fault */
	rdata = *(volatile uint32_t *)(addr & ~MRAM_WORD_MASK);

	/* DSB ensures the load and any resulting fault status write complete */
	barrier_dsync_fence_full();

	/* Read fault status BEFORE re-enabling faults */
	faulted = (SCB->CFSR & SCB_CFSR_BUSFAULTSR_Msk);

	/* Clear whatever was recorded so we don't leave stale status */
	SCB->CFSR = SCB_CFSR_BUSFAULTSR_Msk;

	__set_FAULTMASK(0);
	SCB->CCR &= ~SCB_CCR_BFHFNMIGN_Msk;

	barrier_dsync_fence_full();
	barrier_isync_fence_full();

	irq_unlock(irq_lock_key);

	return faulted;    /* 0 = read succeeded, non-zero = bus fault occurred */
}
#endif /* CONFIG_CPU_CORTEX_M */

#if defined(CONFIG_NRF_MRAM_CHECK_READY_BIT) || defined(CONFIG_NRF_MRAM_CHECK_READYNEXT_BIT)
static inline NRF_MRAMC_Type *nrf_mram_ctrl(uint32_t addr)
{
	return (addr < SOC_NRF_MRAM_BANK_11_ADDRESS) ? NRF_MRAMC110 : NRF_MRAMC111;
}

static int nrf_mram_poll(volatile const uint32_t *reg, uint32_t addr, const char *name)
{
	for (uint32_t spins = CONFIG_NRF_MRAM_READY_MAX_SPINS; spins > 0U; spins--) {
		if (*reg) {
			return 0;
		}
	}

	LOG_ERR("MRAMC %s timeout at 0x%x", name, addr);

	return -ETIMEDOUT;
}
#endif

static inline int nrf_mram_wait_ready(uint32_t addr, uint32_t ironside_se_ver)
{
	ARG_UNUSED(ironside_se_ver);

#if defined(CONFIG_NRF_MRAM_CHECK_READY_BIT)
	return nrf_mram_poll(&nrf_mram_ctrl(addr)->READY, addr, "READY");
#else
	ARG_UNUSED(addr);
	return 0;
#endif
}

/* READYNEXT is only valid once a write has been issued, so poll it afterwards. */
static inline int nrf_mram_wait_write_done(uint32_t addr)
{
#if defined(CONFIG_NRF_MRAM_CHECK_READYNEXT_BIT)
	barrier_dsync_fence_full();

	int ret = nrf_mram_poll(&nrf_mram_ctrl(addr)->READYNEXT, addr, "READYNEXT");

	if (ret) {
		return ret;
	}
#endif
	return nrf_mram_wait_ready(addr, 0);
}

/**
 * Write data to an aligned MRAM word and verify the write succeeded.
 *
 * Writes @p len bytes to @p addr and then calls nrf_mram_detect_corrupt_word()
 * to confirm the write did not leave a corrupted word. The operation is retried
 * up to CONFIG_NRF_MRAM_MAX_RETRIES times before returning an error.
 *
 * @param addr  Destination address. Must be aligned to MRAM_WORD_SIZE (16 bytes).
 * @param data  Pointer to the source data buffer.
 * @param len   Number of bytes to write. Must be a multiple of MRAM_WORD_SIZE.
 *
 * @retval 0       Write succeeded and no corruption was detected.
 * @retval -EIO    Write failed after all retries.
 */
static int nrf_mram_write_and_verify_word(uint32_t addr, const void *data, size_t len)
{
	uint8_t retries = CONFIG_NRF_MRAM_MAX_RETRIES;
	int ret;

	while (retries--) {
		/* Check if data address is aligned to 4 bytes */
		if (((uintptr_t)data % 4) == 0) {
			/* Aligned: use fast uint32_t copies */
			for (size_t i = 0; i < len / 4; i++) {
				((uint32_t *)addr)[i] = ((const uint32_t *)data)[i];
			}
		} else {
			/* Not aligned: use memcpy */
			memcpy((void *)addr, data, len);
		}

		ret = nrf_mram_wait_write_done(addr);
		if (ret) {
			return ret;
		}

#if defined(CONFIG_CPU_CORTEX_M)
		if (!nrf_mram_detect_corrupt_word(addr)) {
			return 0;
		}
#else
		/* VPR (RISC-V) has no BusFault probe; verify by reading back. */
		if (memcmp((const void *)addr, data, len) == 0) {
			return 0;
		}
#endif
		LOG_ERR("MRAM write verification failed at address 0x%x, retrying... (%u retries "
			"left)",
			addr, retries);
	}

	return -EIO;
}

/**
 * Erase an aligned MRAM word region and verify the erase succeeded.
 *
 * Fills @p len bytes starting at @p addr with ERASE_VALUE (0xFF) and then
 * calls nrf_mram_detect_corrupt_word() to confirm the operation did not leave
 * a corrupted word. The operation is retried up to CONFIG_NRF_MRAM_MAX_RETRIES
 * times before returning an error.
 *
 * @param addr  Destination address. Must be aligned to MRAM_WORD_SIZE (16 bytes).
 * @param len   Number of bytes to erase. Must be a multiple of MRAM_WORD_SIZE.
 *
 * @retval 0       Erase succeeded and no corruption was detected.
 * @retval -EIO    Erase failed after all retries.
 */
static int nrf_mram_erase_and_verify_word(uint32_t addr, size_t len)
{
	uint8_t retries = CONFIG_NRF_MRAM_MAX_RETRIES;
	int ret;

	while (retries--) {
		for (size_t i = 0; i < len / 4; i++) {
			((uint32_t *)addr)[i] = 0xffffffffU;
		}

		ret = nrf_mram_wait_write_done(addr);
		if (ret) {
			return ret;
		}

#if defined(CONFIG_CPU_CORTEX_M)
		if (!nrf_mram_detect_corrupt_word(addr)) {
			return 0;
		}
#else
		/* VPR (RISC-V) has no BusFault probe; verify the erased pattern. */
		bool erased = true;

		for (size_t i = 0; i < len / 4; i++) {
			if (((volatile uint32_t *)addr)[i] != 0xffffffffU) {
				erased = false;
				break;
			}
		}
		if (erased) {
			return 0;
		}
#endif
		LOG_ERR("MRAM erase verification failed at address 0x%x, retrying... (%u retries "
			"left)",
			addr, retries);
	}

	return -EIO;
}

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
		LOG_ERR("invalid offset: %ld:%zu", (long)offset, len);
		return 0;
	}

	const uintptr_t addr = MRAM_START + offset;

	if (WRITE_BLOCK_SIZE > 1 && must_align &&
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
	uint32_t ironside_se_ver = nrf_mram_data->ironside_se_ver;
	int ret = 0;

	const uintptr_t addr = validate_and_map_addr(offset, len, true);

	if (!addr) {
		return -EINVAL;
	}

	LOG_DBG("write: %p:%zu", (void *)addr, len);

	k_mutex_lock(&nrf_mram_data->nrf_mram_mutex, K_FOREVER);

	if (ironside_se_ver >= IRONSIDE_SE_SUPPORT_READY_VER) {
#if defined(CONFIG_MRAM_LATENCY)
		int rc = mram_no_latency_sync_request();

		if (rc) {
			LOG_ERR("Failed to request no-latency mode for MRAM write %d", rc);
			ret = -EIO;
			goto unlock;
		}
#endif
	}

	for (uint32_t i = 0; i < (len / MRAM_WORD_SIZE); i++) {
		nrf_mram_hw_lock(addr + (i * MRAM_WORD_SIZE));
		ret = nrf_mram_wait_ready(addr + (i * MRAM_WORD_SIZE), ironside_se_ver);
		if (!ret) {
			ret = nrf_mram_write_and_verify_word(
				addr + (i * MRAM_WORD_SIZE),
				(void *)((uintptr_t)data + (i * MRAM_WORD_SIZE)), MRAM_WORD_SIZE);
		}
		nrf_mram_hw_unlock(addr + (i * MRAM_WORD_SIZE));
		if (ret) {
			goto latency_release;
		}
	}

latency_release:
	if (ironside_se_ver >= IRONSIDE_SE_SUPPORT_READY_VER) {
#if defined(CONFIG_MRAM_LATENCY)
		int rc = mram_no_latency_sync_release();

		if (rc) {
			LOG_ERR("Failed to release no-latency mode for MRAM write");
			ret = -EIO;
		}
#endif
	}
#if defined(CONFIG_MRAM_LATENCY)
unlock:
#endif
	k_mutex_unlock(&nrf_mram_data->nrf_mram_mutex);
	return ret;
}

static int nrf_mram_erase(const struct device *dev, off_t offset, size_t size)
{
	struct nrf_mram_data_t *nrf_mram_data = dev->data;
	uint32_t ironside_se_ver = nrf_mram_data->ironside_se_ver;
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
		int rc = mram_no_latency_sync_request();

		if (rc) {
			LOG_ERR("Failed to request no-latency mode for MRAM erase");
			ret = -EIO;
			goto unlock;
		}
#endif
	}

	for (uint32_t i = 0; i < (size / MRAM_WORD_SIZE); i++) {
		nrf_mram_hw_lock(addr + (i * MRAM_WORD_SIZE));
		ret = nrf_mram_wait_ready(addr + (i * MRAM_WORD_SIZE), ironside_se_ver);
		if (!ret) {
			ret = nrf_mram_erase_and_verify_word(addr + (i * MRAM_WORD_SIZE),
							     MRAM_WORD_SIZE);
		}
		nrf_mram_hw_unlock(addr + (i * MRAM_WORD_SIZE));
		if (ret) {
			goto latency_release;
		}
	}

latency_release:
	if (ironside_se_ver >= IRONSIDE_SE_SUPPORT_READY_VER) {
#if defined(CONFIG_MRAM_LATENCY)
		int rc = mram_no_latency_sync_release();

		if (rc) {
			LOG_ERR("Failed to release no-latency mode for MRAM erase");
			ret = -EIO;
		}
#endif
	}
#if defined(CONFIG_MRAM_LATENCY)
unlock:
#endif
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
