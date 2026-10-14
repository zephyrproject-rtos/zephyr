/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Flash driver for the RRAM of a device booted by IronSide TE.
 *
 * RRAMC is secure-only hardware and IronSide TE holds it in READ mode for the
 * whole life of the non-secure image, so the ordinary nRF RRAM driver cannot
 * serve this target: its first store to RRAM bus-faults. Here reads stay
 * memory-mapped and every write goes through the IronSide TE platform NVM
 * write service, which is the one write path there is and always will be.
 *
 * The API contract is deliberately the same as the RRAM driver's for the same
 * memory - same write-block size, same erase block, no explicit erase, 0xff
 * erase value. What differs is what the hardware will accept:
 *
 *   - Only APPSTORAGE and the update-candidate bank are writable from an
 *     application; the application slot is read-execute once the device is
 *     deployed, so an application stages a candidate in the bank and resets
 *     rather than updating itself in place. A write outside the permitted set,
 *     or one straddling two partitions, comes back -EACCES. The driver cannot
 *     pre-empt that judgement: the permitted set follows the boot mode and the
 *     provisioned partition sizes, neither of which the driver can see.
 *
 *   - A write is not atomic. It lands write line by write line, so a reset
 *     part way through leaves the destination partially updated; a caller that
 *     has to detect that carries its own integrity check.
 *
 *   - Reading the roots of trust or their secure storage from non-secure does
 *     not fail quietly, it takes a SecureFault that IronSide TE turns into a
 *     reset, so the driver refuses those offsets rather than reset the device.
 *
 * See ironsdk/client/include/ironside_client/services/platform.h for the service itself.
 */

#define DT_DRV_COMPAT nordic_ironside_te_rram_controller

#include <errno.h>
#include <inttypes.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/toolchain.h>

#include <ironside_client/error.h>
#include <ironside_client/layout.h>
#include <ironside_client/psa/error.h>
#include <ironside_client/services/platform.h>

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
#include "soc_flash_nrf.h"
#endif

LOG_MODULE_REGISTER(flash_ironside_te_rram, CONFIG_FLASH_LOG_LEVEL);

#define RRAM_NODE DT_INST(0, soc_nv_flash)

BUILD_ASSERT(DT_SAME_NODE(DT_PARENT(RRAM_NODE), DT_DRV_INST(0)),
	     "the soc-nv-flash node must be the IronSide TE RRAM controller's own child");

#define RRAM_START       DT_REG_ADDR(RRAM_NODE)
#define RRAM_SIZE        DT_REG_SIZE(RRAM_NODE)
#define PAGE_SIZE        DT_PROP(RRAM_NODE, erase_block_size)
#define PAGE_COUNT       (RRAM_SIZE / PAGE_SIZE)
#define WRITE_BLOCK_SIZE DT_PROP(RRAM_NODE, write_block_size)
#define ERASE_VALUE      0xff

BUILD_ASSERT(RRAM_SIZE % PAGE_SIZE == 0, "erase-block-size must divide the RRAM size");
BUILD_ASSERT(PAGE_SIZE % WRITE_BLOCK_SIZE == 0,
	     "erase-block-size must be a multiple of write-block-size");

BUILD_ASSERT(IRONSIDE_NVM_APP_ADDR >= RRAM_START && IRONSIDE_NVM_APP_ADDR < RRAM_START + RRAM_SIZE,
	     "the application slot must lie inside the RRAM this driver serves");

BUILD_ASSERT(CONFIG_FLASH_IRONSIDE_TE_RRAM_FILL_CHUNK_SIZE % WRITE_BLOCK_SIZE == 0,
	     "CONFIG_FLASH_IRONSIDE_TE_RRAM_FILL_CHUNK_SIZE must be a whole number of write "
	     "blocks, or a chunk boundary lands inside a write line and rewrites it twice");

/*
 * The client serializes the secure calls, but a fill is many of them, and the
 * radio sync backend runs one operation at a time, so the driver serializes its
 * writers too.
 */
#ifdef CONFIG_MULTITHREADING
static struct k_sem sem_lock;
#define SYNC_INIT()   k_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK()   k_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() k_sem_give(&sem_lock)
#else
#define SYNC_INIT()
#define SYNC_LOCK()
#define SYNC_UNLOCK()
#endif /* CONFIG_MULTITHREADING */

static int status_to_errno(int32_t status)
{
	switch (status) {
	case PSA_SUCCESS:
		return 0;
	case PSA_ERROR_NOT_PERMITTED:
		return -EACCES;
	case PSA_ERROR_NOT_SUPPORTED:
		return -ENOTSUP;
	case PSA_ERROR_INVALID_ARGUMENT:
		return -EINVAL;
	case PSA_ERROR_HARDWARE_FAILURE:
	default:
		/* The write failed, or the service answered something this driver
		 * has no mapping for. Nothing landed either way.
		 */
		return -EIO;
	}
}

static bool range_is_in_rram(off_t offset, size_t len)
{
	return (offset >= 0) && ((size_t)offset <= RRAM_SIZE) &&
	       (len <= (RRAM_SIZE - (size_t)offset));
}

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE

#define SYNC_CHUNK_SIZE (CONFIG_FLASH_IRONSIDE_TE_RRAM_SYNC_WRITE_LINES * WRITE_BLOCK_SIZE)
#define FLASH_SLOT_WRITE_US NRF_RRAM_WRITE_WINDOW_US(CONFIG_FLASH_IRONSIDE_TE_RRAM_SYNC_WRITE_LINES)

struct sync_write {
	struct flash_context ctx;
	int32_t status; /* The service's answer to the chunk that failed, if one did. */
};

static void shift_write_context(uint32_t shift, struct flash_context *w_ctx)
{
	w_ctx->flash_addr += shift;
	w_ctx->data_addr += shift;
	w_ctx->len -= shift;
}

static int write_op(void *context)
{
	struct flash_context *w_ctx = context;
	uint32_t i = 0;

	nrf_flash_sync_get_timestamp_begin();

	while (w_ctx->len > 0) {
		size_t len = MIN(SYNC_CHUNK_SIZE, w_ctx->len);
		int32_t status = ironside_platform_nvm_write(w_ctx->flash_addr,
							     (const void *)w_ctx->data_addr, len);

		if (status == IRONSIDE_ERROR_CALL_IN_FLIGHT) {
			/*
			 * This window preempted a thread inside a call of its own, and
			 * that call cannot finish until this handler returns. The client
			 * skips its lock in interrupt context, so retry in the next window
			 * rather than wait here.
			 */
			return FLASH_OP_ONGOING;
		}

		if (status != PSA_SUCCESS) {
			CONTAINER_OF(w_ctx, struct sync_write, ctx)->status = status;
			return status_to_errno(status);
		}

		shift_write_context(len, w_ctx);

		if (w_ctx->len == 0) {
			break;
		}

		++i;

		if (nrf_flash_sync_check_time_limit(i)) {
			return FLASH_OP_ONGOING;
		}
	}

	return FLASH_OP_DONE;
}

static int write_synchronously(uint32_t addr, const void *data, size_t len)
{
	struct sync_write w = {
		.ctx = {
			.data_addr = (uint32_t)data,
			.flash_addr = addr,
			.len = len,
		},
		.status = PSA_SUCCESS,
	};
	struct flash_op_desc flash_op_desc = {.handler = write_op, .context = &w.ctx};
	int err;

	nrf_flash_sync_set_context(FLASH_SLOT_WRITE_US);
	err = nrf_flash_sync_exe(&flash_op_desc);

	if (err != 0) {
		const bool failed = (w.status != PSA_SUCCESS);

		LOG_ERR("write of %zu bytes at 0x%x %s: %d", len, addr,
			failed ? "failed" : "did not complete", failed ? (int)w.status : err);
	}

	return err;
}

#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */

/* The caller holds the driver lock. */
static int do_write(off_t offset, const void *src, size_t len)
{
	const uint32_t addr = (uint32_t)(RRAM_START + offset);
	int32_t status;

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
	if (nrf_flash_sync_is_required()) {
		return write_synchronously(addr, src, len);
	}
#endif

	status = ironside_platform_nvm_write(addr, src, (uint32_t)len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("write of %zu bytes at 0x%x %s: %d", len, addr, "failed", (int)status);
	}

	return status_to_errno(status);
}

static int rram_write(off_t offset, const void *src, size_t len)
{
	int err;

	SYNC_LOCK();
	err = do_write(offset, src, len);
	SYNC_UNLOCK();

	return err;
}

static int rram_fill(off_t offset, uint8_t value, size_t len)
{
	uint8_t chunk[CONFIG_FLASH_IRONSIDE_TE_RRAM_FILL_CHUNK_SIZE];
	size_t filled = 0;
	int err = 0;

	if (!range_is_in_rram(offset, len)) {
		return -EINVAL;
	}

	memset(chunk, value, sizeof(chunk));

	SYNC_LOCK();

	while (filled < len) {
		size_t n = MIN(sizeof(chunk), len - filled);

		err = do_write(offset + filled, chunk, n);
		if (err != 0) {
			break;
		}

		filled += n;
	}

	SYNC_UNLOCK();

	return err;
}

static int flash_ironside_te_rram_read(const struct device *dev, off_t offset,
				       void *data, size_t len)
{
	ARG_UNUSED(dev);

	if (!range_is_in_rram(offset, len)) {
		return -EINVAL;
	}

	if (RRAM_START + offset < IRONSIDE_NVM_APP_ADDR ||
	    (RRAM_START + offset < IRONSIDE_NVM_UROTSTORAGE_ADDR + IRONSIDE_NVM_UROTSTORAGE_SIZE &&
	     RRAM_START + offset + len > IRONSIDE_NVM_UROTSTORAGE_ADDR)) {
		LOG_DBG("%zu bytes at 0x%" PRIxPTR " reach into IronSide TE", len,
			(uintptr_t)(RRAM_START + offset));
		return -EACCES;
	}

	memcpy(data, (const void *)(RRAM_START + offset), len);

	return 0;
}

static int flash_ironside_te_rram_write(const struct device *dev, off_t offset, const void *data,
				    size_t len)
{
	ARG_UNUSED(dev);

	if (offset % WRITE_BLOCK_SIZE != 0 || len % WRITE_BLOCK_SIZE != 0) {
		return -EINVAL;
	}

	if (!range_is_in_rram(offset, len)) {
		return -EINVAL;
	}

	return rram_write(offset, data, len);
}

static int flash_ironside_te_rram_erase(const struct device *dev, off_t offset, size_t len)
{
	ARG_UNUSED(dev);

	if (offset % PAGE_SIZE != 0 || len % PAGE_SIZE != 0) {
		return -EINVAL;
	}

	return rram_fill(offset, ERASE_VALUE, len);
}

#if defined(CONFIG_FLASH_HAS_DRIVER_FILL)
static int flash_ironside_te_rram_fill(const struct device *dev, uint8_t value,
				       off_t offset, size_t len)
{
	ARG_UNUSED(dev);

	if (offset % WRITE_BLOCK_SIZE != 0 || len % WRITE_BLOCK_SIZE != 0) {
		return -EINVAL;
	}

	return rram_fill(offset, value, len);
}
#endif /* CONFIG_FLASH_HAS_DRIVER_FILL */

static int flash_ironside_te_rram_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = RRAM_SIZE;
	return 0;
}

static const struct flash_parameters *
flash_ironside_te_rram_get_parameters(const struct device *dev)
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
static void flash_ironside_te_rram_page_layout(const struct device *dev,
					       const struct flash_pages_layout **layout,
					       size_t *layout_size)
{
	ARG_UNUSED(dev);

	static const struct flash_pages_layout pages_layout = {
		.pages_count = PAGE_COUNT,
		.pages_size = PAGE_SIZE,
	};

	*layout = &pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static DEVICE_API(flash, flash_ironside_te_rram_api) = {
	.read = flash_ironside_te_rram_read,
	.write = flash_ironside_te_rram_write,
	.erase = flash_ironside_te_rram_erase,
#if defined(CONFIG_FLASH_HAS_DRIVER_FILL)
	.fill = flash_ironside_te_rram_fill,
#endif
	.get_size = flash_ironside_te_rram_get_size,
	.get_parameters = flash_ironside_te_rram_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_ironside_te_rram_page_layout,
#endif
};

static int flash_ironside_te_rram_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	SYNC_INIT();

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
	return nrf_flash_sync_init();
#else
	return 0;
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
}

DEVICE_DT_INST_DEFINE(0, flash_ironside_te_rram_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_ironside_te_rram_api);
