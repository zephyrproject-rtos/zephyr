/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Linaro Limited
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/flash.h>
#include <string.h>
#include <nrfx_nvmc.h>
#include <nrf_erratas.h>

#include "soc_flash_nrf.h"

#define LOG_LEVEL CONFIG_FLASH_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_nrf);

#if DT_NODE_HAS_STATUS(DT_INST(0, nordic_nrf51_flash_controller), okay)
#define DT_DRV_COMPAT nordic_nrf51_flash_controller
#elif DT_NODE_HAS_STATUS(DT_INST(0, nordic_nrf52_flash_controller), okay)
#define DT_DRV_COMPAT nordic_nrf52_flash_controller
#elif DT_NODE_HAS_STATUS(DT_INST(0, nordic_nrf53_flash_controller), okay)
#define DT_DRV_COMPAT nordic_nrf53_flash_controller
#elif DT_NODE_HAS_STATUS(DT_INST(0, nordic_nrf91_flash_controller), okay)
#define DT_DRV_COMPAT nordic_nrf91_flash_controller
#else
#error No matching compatible for soc_flash_nrf.c
#endif

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
#define FLASH_SLOT_WRITE     7500
#if defined(CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE)
#define FLASH_SLOT_ERASE (MAX(CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE_MS * 1000, \
			      7500))
#else
#define FLASH_SLOT_ERASE FLASH_PAGE_ERASE_MAX_TIME_US
#endif /* CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE */

static int write_op(void *context); /* instance of flash_op_handler_t */
static int write_synchronously(off_t addr, const void *data, size_t len);

static int erase_op(void *context); /* instance of flash_op_handler_t */
static int erase_synchronously(uint32_t addr, uint32_t size);
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */

static const struct flash_parameters flash_nrf_parameters = {
#if defined(CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS)
	.write_block_size = 1,
#else
	.write_block_size = 4,
#endif
	.erase_value = 0xff,
};

#if defined(CONFIG_MULTITHREADING)
/* semaphore for locking flash resources (tickers) */
static struct k_sem sem_lock;
#define SYNC_INIT() k_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK() k_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() k_sem_give(&sem_lock)
#else
#define SYNC_INIT()
#define SYNC_LOCK()
#define SYNC_UNLOCK()
#endif

#if NRF52_ERRATA_242_PRESENT
#include <hal/nrf_power.h>
static int suspend_pofwarn(void);
static void restore_pofwarn(void);

#define SUSPEND_POFWARN() suspend_pofwarn()
#define RESUME_POFWARN()  restore_pofwarn()
#else
#define SUSPEND_POFWARN() 0
#define RESUME_POFWARN()
#endif /* NRF52_ERRATA_242_PRESENT */

static int write(off_t addr, const void *data, size_t len);
static int erase(uint32_t addr, uint32_t size);

static inline bool is_aligned_32(uint32_t data)
{
	return (data & 0x3) ? false : true;
}

static inline bool is_within_bounds(off_t addr, size_t len, off_t boundary_start,
				    size_t boundary_size)
{
	return (addr >= boundary_start &&
			(addr < (boundary_start + boundary_size)) &&
			(len <= (boundary_start + boundary_size - addr)));
}

static inline bool is_regular_addr_valid(off_t addr, size_t len)
{
	return is_within_bounds(addr, len, 0, nrfx_nvmc_flash_size_get());
}

static inline bool is_uicr_addr_valid(off_t addr, size_t len)
{
#ifdef CONFIG_SOC_FLASH_NRF_UICR
	return is_within_bounds(addr, len, (off_t)NRF_UICR, sizeof(*NRF_UICR));
#else
	return false;
#endif /* CONFIG_SOC_FLASH_NRF_UICR */
}

static void nvmc_wait_ready(void)
{
	while (!nrfx_nvmc_write_done_check()) {
	}
}

static int flash_nrf_read(const struct device *dev, off_t addr,
			    void *data, size_t len)
{
	if (is_regular_addr_valid(addr, len)) {
		addr += DT_REG_ADDR(SOC_NV_FLASH_NODE);
	} else if (!is_uicr_addr_valid(addr, len)) {
		LOG_ERR("invalid address: 0x%08lx:%zu",
				(unsigned long)addr, len);
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	memcpy(data, (void *)addr, len);

	return 0;
}

static int flash_nrf_write(const struct device *dev, off_t addr,
			     const void *data, size_t len)
{
	int ret;

	if (is_regular_addr_valid(addr, len)) {
		addr += DT_REG_ADDR(SOC_NV_FLASH_NODE);
	} else if (!is_uicr_addr_valid(addr, len)) {
		LOG_ERR("invalid address: 0x%08lx:%zu",
				(unsigned long)addr, len);
		return -EINVAL;
	}

#if !IS_ENABLED(CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS)
	if (!is_aligned_32(addr) || (len % sizeof(uint32_t))) {
		LOG_ERR("not word-aligned: 0x%08lx:%zu",
				(unsigned long)addr, len);
		return -EINVAL;
	}
#endif

	if (!len) {
		return 0;
	}

	SYNC_LOCK();

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
	if (nrf_flash_sync_is_required()) {
		ret = write_synchronously(addr, data, len);
	} else
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
	{
		ret = write(addr, data, len);
	}

	SYNC_UNLOCK();

	return ret;
}

static int flash_nrf_erase(const struct device *dev, off_t addr, size_t size)
{
	uint32_t pg_size = nrfx_nvmc_flash_page_size_get();
	uint32_t n_pages = size / pg_size;
	int ret;

	if (is_regular_addr_valid(addr, size)) {
		/* Erase can only be done per page */
		if (((addr % pg_size) != 0) || ((size % pg_size) != 0)) {
			LOG_ERR("unaligned address: 0x%08lx:%zu",
					(unsigned long)addr, size);
			return -EINVAL;
		}

		if (!n_pages) {
			return 0;
		}

		addr += DT_REG_ADDR(SOC_NV_FLASH_NODE);
#ifdef CONFIG_SOC_FLASH_NRF_UICR
	} else if (addr != (off_t)NRF_UICR || size != sizeof(*NRF_UICR)) {
		LOG_ERR("invalid address: 0x%08lx:%zu",
				(unsigned long)addr, size);
		return -EINVAL;
	}
#else
	} else {
		LOG_ERR("invalid address: 0x%08lx:%zu",
				(unsigned long)addr, size);
		return -EINVAL;
	}
#endif /* CONFIG_SOC_FLASH_NRF_UICR */

	SYNC_LOCK();

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
	if (nrf_flash_sync_is_required()) {
		ret = erase_synchronously(addr, size);
	} else
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
	{
		ret = erase(addr, size);
	}

	SYNC_UNLOCK();

	return ret;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static struct flash_pages_layout dev_layout;

static void flash_nrf_pages_layout(const struct device *dev,
				     const struct flash_pages_layout **layout,
				     size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
flash_nrf_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_nrf_parameters;
}

static const struct flash_driver_api flash_nrf_api = {
	.read = flash_nrf_read,
	.write = flash_nrf_write,
	.erase = flash_nrf_erase,
	.get_parameters = flash_nrf_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_nrf_pages_layout,
#endif
};

static int nrf_flash_init(const struct device *dev)
{
	SYNC_INIT();

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
	nrf_flash_sync_init();
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	dev_layout.pages_count = nrfx_nvmc_flash_page_count_get();
	dev_layout.pages_size = nrfx_nvmc_flash_page_size_get();
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0, nrf_flash_init, NULL,
		 NULL, NULL,
		 POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		 &flash_nrf_api);

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE

static int erase_synchronously(uint32_t addr, uint32_t size)
{
	struct flash_context context = {
		.flash_addr = addr,
		.len = size,
		.enable_time_limit = 1, /* enable time limit */
#if defined(CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE)
		.flash_addr_next = addr
#endif
	};

	struct flash_op_desc flash_op_desc = {
		.handler = erase_op,
		.context = &context
	};

	nrf_flash_sync_set_context(FLASH_SLOT_ERASE);
	return nrf_flash_sync_exe(&flash_op_desc);
}

static int write_synchronously(off_t addr, const void *data, size_t len)
{
	struct flash_context context = {
		.data_addr = (uint32_t) data,
		.flash_addr = addr,
		.len = len,
		.enable_time_limit = 1 /* enable time limit */
	};

	struct flash_op_desc flash_op_desc = {
		.handler = write_op,
		.context = &context
	};

	nrf_flash_sync_set_context(FLASH_SLOT_WRITE);
	return nrf_flash_sync_exe(&flash_op_desc);
}

#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */

static int erase_op(void *context)
{
	uint32_t pg_size = nrfx_nvmc_flash_page_size_get();
	struct flash_context *e_ctx = context;

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
	uint32_t i = 0U;

	if (e_ctx->enable_time_limit) {
		nrf_flash_sync_get_timestamp_begin();
	}
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */

#ifdef CONFIG_SOC_FLASH_NRF_UICR
	if (e_ctx->flash_addr == (off_t)NRF_UICR) {
		if (SUSPEND_POFWARN()) {
			return -ECANCELED;
		}

		(void)nrfx_nvmc_uicr_erase();
		RESUME_POFWARN();
		return FLASH_OP_DONE;
	}
#endif

	do {
		if (SUSPEND_POFWARN()) {
			return -ECANCELED;
		}

#if defined(CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE)
		if (e_ctx->flash_addr == e_ctx->flash_addr_next) {
			nrfx_nvmc_page_partial_erase_init(e_ctx->flash_addr,
				CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE_MS);
			e_ctx->flash_addr_next += pg_size;
		}

		if (nrfx_nvmc_page_partial_erase_continue()) {
			e_ctx->len -= pg_size;
			e_ctx->flash_addr += pg_size;
		}
#else
		(void)nrfx_nvmc_page_erase(e_ctx->flash_addr);
		e_ctx->len -= pg_size;
		e_ctx->flash_addr += pg_size;
#endif /* CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE */

		RESUME_POFWARN();

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
		i++;

		if (e_ctx->enable_time_limit) {
			if (nrf_flash_sync_check_time_limit(i)) {
				break;
			}

		}
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */

	} while (e_ctx->len > 0);

	return (e_ctx->len > 0) ? FLASH_OP_ONGOING : FLASH_OP_DONE;
}

static void shift_write_context(uint32_t shift, struct flash_context *w_ctx)
{
	w_ctx->flash_addr += shift;
	w_ctx->data_addr += shift;
	w_ctx->len -= shift;
}

static int write_op(void *context)
{
	struct flash_context *w_ctx = context;

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
	uint32_t i = 1U;

	if (w_ctx->enable_time_limit) {
		nrf_flash_sync_get_timestamp_begin();
	}
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
#if defined(CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS)
	/* If not aligned, write unaligned beginning */
	if (!is_aligned_32(w_ctx->flash_addr)) {
		uint32_t count = sizeof(uint32_t) - (w_ctx->flash_addr & 0x3);

		if (count > w_ctx->len) {
			count = w_ctx->len;
		}

		if (SUSPEND_POFWARN()) {
			return -ECANCELED;
		}

		nrfx_nvmc_bytes_write(w_ctx->flash_addr,
				      (const void *)w_ctx->data_addr,
				      count);

		RESUME_POFWARN();
		shift_write_context(count, w_ctx);

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
		if (w_ctx->enable_time_limit) {
			if (nrf_flash_sync_check_time_limit(1)) {
				nvmc_wait_ready();
				return FLASH_OP_ONGOING;
			}
		}
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
	}
#endif /* CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS */
	/* Write all the 4-byte aligned data */
	while (w_ctx->len >= sizeof(uint32_t)) {
		if (SUSPEND_POFWARN()) {
			return -ECANCELED;
		}

		nrfx_nvmc_word_write(w_ctx->flash_addr,
				     UNALIGNED_GET((uint32_t *)w_ctx->data_addr));
		RESUME_POFWARN();
		shift_write_context(sizeof(uint32_t), w_ctx);

#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
		i++;

		if (w_ctx->enable_time_limit) {
			if (nrf_flash_sync_check_time_limit(i)) {
				nvmc_wait_ready();
				return FLASH_OP_ONGOING;
			}
		}
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
	}
#if defined(CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS)
	/* Write remaining unaligned data */
	if (w_ctx->len) {
		if (SUSPEND_POFWARN()) {
			return -ECANCELED;
		}

		nrfx_nvmc_bytes_write(w_ctx->flash_addr,
				      (const void *)w_ctx->data_addr,
				      w_ctx->len);
		RESUME_POFWARN();
		shift_write_context(w_ctx->len, w_ctx);
	}
#endif /* CONFIG_SOC_FLASH_NRF_EMULATE_ONE_BYTE_WRITE_ACCESS */
	nvmc_wait_ready();

	return FLASH_OP_DONE;
}

static int erase(uint32_t addr, uint32_t size)
{
	struct flash_context context = {
		.flash_addr = addr,
		.len = size,
#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
		.enable_time_limit = 0, /* disable time limit */
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
#if defined(CONFIG_SOC_FLASH_NRF_PARTIAL_ERASE)
		.flash_addr_next = addr
#endif
	};

	return	erase_op(&context);
}

static int write(off_t addr, const void *data, size_t len)
{
	struct flash_context context = {
		.data_addr = (uint32_t) data,
		.flash_addr = addr,
		.len = len,
#ifndef CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE
		.enable_time_limit = 0 /* disable time limit */
#endif /* !CONFIG_SOC_FLASH_NRF_RADIO_SYNC_NONE */
	};

	return write_op(&context);
}

#if NRF52_ERRATA_242_PRESENT
/* Disable POFWARN by writing POFCON before a write or erase operation.
 * Do not attempt to write or erase if EVENTS_POFWARN is already asserted.
 */
static bool pofcon_enabled;

static int suspend_pofwarn(void)
{
	if (!nrf52_errata_242()) {
		return 0;
	}

	bool enabled;
	nrf_power_pof_thr_t pof_thr;

	pof_thr = nrf_power_pofcon_get(NRF_POWER, &enabled);

	if (enabled) {
		nrf_power_pofcon_set(NRF_POWER, false, pof_thr);

		/* This check need to be reworked once POFWARN event will be
		 * served by zephyr.
		 */
		if (nrf_power_event_check(NRF_POWER, NRF_POWER_EVENT_POFWARN)) {
			nrf_power_pofcon_set(NRF_POWER, true, pof_thr);
			return -ECANCELED;
		}

		pofcon_enabled = enabled;
	}

	return 0;
}

static void restore_pofwarn(void)
{
	nrf_power_pof_thr_t pof_thr;

	if (pofcon_enabled) {
		pof_thr = nrf_power_pofcon_get(NRF_POWER, NULL);

		nrf_power_pofcon_set(NRF_POWER, true, pof_thr);
		pofcon_enabled = false;
	}
}
#endif  /* NRF52_ERRATA_242_PRESENT */
