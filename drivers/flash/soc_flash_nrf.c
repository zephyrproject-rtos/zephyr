/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Linaro Limited
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/flash.h>
#include <string.h>
#include <nrfx_nvmc.h>

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
#include <sys/__assert.h>
#include <bluetooth/hci.h>
#include "controller/hal/ticker.h"
#include "controller/ticker/ticker.h"
#include "controller/include/ll.h"

#define FLASH_SLOT_ERASE     FLASH_PAGE_ERASE_MAX_TIME_US
#define FLASH_INTERVAL_ERASE FLASH_SLOT_ERASE
#define FLASH_SLOT_WRITE     7500
#define FLASH_INTERVAL_WRITE FLASH_SLOT_WRITE

#define FLASH_RADIO_ABORT_DELAY_US 500
#define FLASH_TIMEOUT_MS ((FLASH_PAGE_ERASE_MAX_TIME_US)\
			* (FLASH_PAGE_MAX_CNT) / 1000)
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */

#define FLASH_OP_DONE    (0) /* 0 for compliance with the driver API. */
#define FLASH_OP_ONGOING (-1)

struct flash_context {
	u32_t data_addr;  /* Address of data to write. */
	u32_t flash_addr; /* Address of flash to write or erase. */
	u32_t len;        /* Size off data to write or erase [B]. */
#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
	u8_t  enable_time_limit; /* execution limited to timeslot. */
	u32_t interval;   /* timeslot interval. */
	u32_t slot;       /* timeslot length. */
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */
}; /*< Context type for f. @ref write_op @ref erase_op */

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
typedef int (*flash_op_handler_t) (void *context);

struct flash_op_desc {
	flash_op_handler_t handler;
	struct flash_context *context; /* [in,out] */
	int result;
};

/* semaphore for synchronization of flash operations */
static struct k_sem sem_sync;

static int write_op(void *context); /* instance of flash_op_handler_t */
static int write_in_timeslice(off_t addr, const void *data, size_t len);

static int erase_op(void *context); /* instance of flash_op_handler_t */
static int erase_in_timeslice(u32_t addr, u32_t size);
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */

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

static bool write_protect;

static int write(off_t addr, const void *data, size_t len);
static int erase(u32_t addr, u32_t size);

static inline bool is_aligned_32(u32_t data)
{
	return (data & 0x3) ? false : true;
}

static inline bool is_regular_addr_valid(off_t addr, size_t len)
{
	size_t flash_size = nrfx_nvmc_flash_size_get();

	if (addr >= DT_FLASH_BASE_ADDRESS + flash_size ||
	    addr < DT_FLASH_BASE_ADDRESS ||
	    len > flash_size ||
	    (addr - DT_FLASH_BASE_ADDRESS) + len > flash_size) {
		return false;
	}
	return true;
}


static inline bool is_uicr_addr_valid(off_t addr, size_t len)
{
#ifdef CONFIG_SOC_FLASH_NRF_UICR
	if (addr >= (off_t)NRF_UICR + sizeof(*NRF_UICR) ||
	    addr < (off_t)NRF_UICR ||
	    len > sizeof(*NRF_UICR) ||
	    addr + len > (off_t)NRF_UICR + sizeof(*NRF_UICR)) {
		return false;
	}

	return true;
#else
	return false;
#endif /* CONFIG_SOC_FLASH_NRF_UICR */
}

static inline bool is_addr_valid(off_t addr, size_t len)
{
	return is_regular_addr_valid(addr, len) ||
	       is_uicr_addr_valid(addr, len);
}

static void nvmc_wait_ready(void)
{
	while (!nrfx_nvmc_write_done_check()) {
	}
}

static int flash_nrf_read(struct device *dev, off_t addr,
			    void *data, size_t len)
{
	if (!is_addr_valid(addr, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	memcpy(data, (void *)addr, len);

	return 0;
}

static int flash_nrf_write(struct device *dev, off_t addr,
			     const void *data, size_t len)
{
	int ret;

	if (write_protect) {
		return -EACCES;
	}

	if (!is_addr_valid(addr, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	SYNC_LOCK();

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
	if (ticker_is_initialized(0)) {
		ret = write_in_timeslice(addr, data, len);
	} else
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */
	{
		ret = write(addr, data, len);
	}

	SYNC_UNLOCK();

	return ret;
}

static int flash_nrf_erase(struct device *dev, off_t addr, size_t size)
{
	u32_t pg_size = nrfx_nvmc_flash_page_size_get();
	u32_t n_pages = size / pg_size;
	int ret;

	if (write_protect) {
		return -EACCES;
	}

	if (is_regular_addr_valid(addr, size)) {
		/* Erase can only be done per page */
		if (((addr % pg_size) != 0) || ((size % pg_size) != 0)) {
			return -EINVAL;
		}

		if (!n_pages) {
			return 0;
		}
#ifdef CONFIG_SOC_FLASH_NRF_UICR
	} else if (addr != (off_t)NRF_UICR || size != sizeof(*NRF_UICR)) {
		return -EINVAL;
	}
#else
	} else {
		return -EINVAL;
	}
#endif /* CONFIG_SOC_FLASH_NRF_UICR */

	SYNC_LOCK();

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
	if (ticker_is_initialized(0)) {
		ret = erase_in_timeslice(addr, size);
	} else
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */
	{
		ret = erase(addr, size);
	}

	SYNC_UNLOCK();

	return ret;
}

static int flash_nrf_write_protection(struct device *dev, bool enable)
{
	/* virtual write-erase protection */
	write_protect = enable;

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static struct flash_pages_layout dev_layout;

static void flash_nrf_pages_layout(struct device *dev,
				     const struct flash_pages_layout **layout,
				     size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_nrf_api = {
	.read = flash_nrf_read,
	.write = flash_nrf_write,
	.erase = flash_nrf_erase,
	.write_protection = flash_nrf_write_protection,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_nrf_pages_layout,
#endif
	.write_block_size = 1,
};

static int nrf_flash_init(struct device *dev)
{
	SYNC_INIT();

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
	k_sem_init(&sem_sync, 0, 1);
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	dev_layout.pages_count = nrfx_nvmc_flash_page_count_get();
	dev_layout.pages_size = nrfx_nvmc_flash_page_size_get();
#endif
	write_protect = true;

	return 0;
}

DEVICE_AND_API_INIT(nrf_flash, DT_FLASH_DEV_NAME, nrf_flash_init,
		NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&flash_nrf_api);

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)

static void time_slot_callback_work(u32_t ticks_at_expire, u32_t remainder,
				    u16_t lazy, void *context)
{
	struct flash_op_desc *op_desc;
	u8_t instance_index;
	u8_t ticker_id;
	int result;

	__ASSERT(ll_radio_state_is_idle(),
		 "Radio is on during flash operation.\n");

	op_desc = context;
	if (op_desc->handler(op_desc->context) == FLASH_OP_DONE) {
		ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

		/* Stop the time slot ticker */
		result = ticker_stop(instance_index,
				     0,
				     ticker_id,
				     NULL,
				     NULL);

		if (result != TICKER_STATUS_SUCCESS &&
		    result != TICKER_STATUS_BUSY) {
			__ASSERT(0, "Failed to stop ticker.\n");
		}

		((struct flash_op_desc *)context)->result = 0;

		/* notify thread that data is available */
		k_sem_give(&sem_sync);
	}
}

static void time_slot_callback_helper(u32_t ticks_at_expire, u32_t remainder,
		u16_t lazy, void *context)
{
	u8_t instance_index;
	u8_t ticker_id;
	int err;

	ll_radio_state_abort();

	ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

	/* start a secondary one-shot ticker after ~ 500 us, */
	/* this will let any radio role to gracefully release the Radio h/w */

	err = ticker_start(instance_index, /* Radio instance ticker */
			   0, /* user_id */
			   0, /* ticker_id */
			   ticks_at_expire, /* current tick */
			   HAL_TICKER_US_TO_TICKS(FLASH_RADIO_ABORT_DELAY_US),
			   0, /* periodic (on-shot) */
			   0, /* per. remaind. (on-shot) */
			   0, /* lazy, voluntary skips */
			   0,
			   time_slot_callback_work, /* handler for executing */
						    /* the flash operation */
			   context, /* the context for the flash operation */
			   NULL, /* no op callback */
			   NULL);

	if (err != TICKER_STATUS_SUCCESS && err != TICKER_STATUS_BUSY) {
		((struct flash_op_desc *)context)->result = -ECANCELED;

		/* abort flash timeslots */
		err = ticker_stop(instance_index, 0, ticker_id, NULL, NULL);
		if (err != TICKER_STATUS_SUCCESS &&
		    err != TICKER_STATUS_BUSY) {
			__ASSERT(0, "Failed to stop ticker.\n");
		}

		/* notify thread that data is available */
		k_sem_give(&sem_sync);
	}
}

static int work_in_time_slice(struct flash_op_desc *p_flash_op_desc)
{
	u8_t instance_index;
	u8_t ticker_id;
	int result;
	u32_t err;
	struct flash_context *context = p_flash_op_desc->context;

	ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

	err = ticker_start(instance_index,
			   3, /* user id for thread mode */
			      /* (MAYFLY_CALL_ID_PROGRAM) */
			   ticker_id, /* flash ticker id */
			   ticker_ticks_now_get(), /* current tick */
			   0, /* first int. immediately */
			   /* period */
			   HAL_TICKER_US_TO_TICKS(context->interval),
			   /* period remainder */
			   HAL_TICKER_REMAINDER(context->interval),
			   0, /* lazy, voluntary skips */
			   HAL_TICKER_US_TO_TICKS(context->slot),
			   time_slot_callback_helper,
			   p_flash_op_desc,
			   NULL, /* no op callback */
			   NULL);

	if (err != TICKER_STATUS_SUCCESS && err != TICKER_STATUS_BUSY) {
		result = -ECANCELED;
	} else if (k_sem_take(&sem_sync, K_MSEC(FLASH_TIMEOUT_MS)) != 0) {
		/* wait for operation's complete overrun*/
		result = -ETIMEDOUT;
	} else {
		result = p_flash_op_desc->result;
	}

	return result;
}

static int erase_in_timeslice(u32_t addr, u32_t size)
{
	struct flash_context context = {
		.flash_addr = addr,
		.len = size,
		.enable_time_limit = 1, /* enable time limit */
		.interval = FLASH_INTERVAL_ERASE,
		.slot = FLASH_SLOT_ERASE
	};

	struct flash_op_desc flash_op_desc = {
		.handler = erase_op,
		.context = &context
	};

	return work_in_time_slice(&flash_op_desc);
}

static int write_in_timeslice(off_t addr, const void *data, size_t len)
{
	struct flash_context context = {
		.data_addr = (u32_t) data,
		.flash_addr = addr,
		.len = len,
		.enable_time_limit = 1, /* enable time limit */
		.interval = FLASH_INTERVAL_WRITE,
		.slot = FLASH_SLOT_WRITE
	};

	struct flash_op_desc flash_op_desc = {
		.handler = write_op,
		.context = &context
	};

	return  work_in_time_slice(&flash_op_desc);
}

#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */

static int erase_op(void *context)
{
	u32_t pg_size = nrfx_nvmc_flash_page_size_get();
	struct flash_context *e_ctx = context;

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
	u32_t ticks_begin = 0U;
	u32_t ticks_diff;
	u32_t i = 0U;

	if (e_ctx->enable_time_limit) {
		ticks_begin = ticker_ticks_now_get();
	}
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */

#ifdef CONFIG_SOC_FLASH_NRF_UICR
	if (e_ctx->flash_addr == (off_t)NRF_UICR) {
		(void)nrfx_nvmc_uicr_erase();
		return FLASH_OP_DONE;
	}
#endif

	do {
		(void)nrfx_nvmc_page_erase(e_ctx->flash_addr);

		e_ctx->len -= pg_size;
		e_ctx->flash_addr += pg_size;

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
		i++;

		if (e_ctx->enable_time_limit) {
			ticks_diff =
				ticker_ticks_diff_get(ticker_ticks_now_get(),
						      ticks_begin);
			if (ticks_diff + ticks_diff/i >
			    HAL_TICKER_US_TO_TICKS(e_ctx->slot)) {
				break;
			}
		}
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */

	} while (e_ctx->len > 0);

	return (e_ctx->len > 0) ? FLASH_OP_ONGOING : FLASH_OP_DONE;
}

static void shift_write_context(u32_t shift, struct flash_context *w_ctx)
{
	w_ctx->flash_addr += shift;
	w_ctx->data_addr += shift;
	w_ctx->len -= shift;
}

static int write_op(void *context)
{
	struct flash_context *w_ctx = context;
	u32_t count;

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
	u32_t ticks_begin = 0U;
	u32_t ticks_diff;
	u32_t i = 1U;

	if (w_ctx->enable_time_limit) {
		ticks_begin = ticker_ticks_now_get();
	}
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */

	/* If not aligned, write unaligned beginning */
	if (!is_aligned_32(w_ctx->flash_addr)) {
		count = sizeof(u32_t) - (w_ctx->flash_addr & 0x3);
		if (count > w_ctx->len) {
			count = w_ctx->len;
		}

		nrfx_nvmc_bytes_write(w_ctx->flash_addr,
				      (const void *)w_ctx->data_addr,
				      count);

		shift_write_context(count, w_ctx);

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
		if (w_ctx->enable_time_limit) {
			ticks_diff =
				ticker_ticks_diff_get(ticker_ticks_now_get(),
						      ticks_begin);
			if (ticks_diff * 2U >
			    HAL_TICKER_US_TO_TICKS(w_ctx->slot)) {
				nvmc_wait_ready();
				return FLASH_OP_ONGOING;
			}
		}
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */
	}

	/* Write all the 4-byte aligned data */
	while (w_ctx->len >= sizeof(u32_t)) {
		nrfx_nvmc_word_write(w_ctx->flash_addr,
				     UNALIGNED_GET((u32_t *)w_ctx->data_addr));

		shift_write_context(sizeof(u32_t), w_ctx);

#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
		i++;

		if (w_ctx->enable_time_limit) {
			ticks_diff =
				ticker_ticks_diff_get(ticker_ticks_now_get(),
						      ticks_begin);
			if (ticks_diff + ticks_diff/i >
			    HAL_TICKER_US_TO_TICKS(w_ctx->slot)) {
				nvmc_wait_ready();
				return FLASH_OP_ONGOING;
			}
		}
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */
	}

	/* Write remaining unaligned data */
	if (w_ctx->len) {
		nrfx_nvmc_bytes_write(w_ctx->flash_addr,
				      (const void *)w_ctx->data_addr,
				      w_ctx->len);

		shift_write_context(w_ctx->len, w_ctx);
	}

	nvmc_wait_ready();

	return FLASH_OP_DONE;
}

static int erase(u32_t addr, u32_t size)
{
	struct flash_context context = {
		.flash_addr = addr,
		.len = size,
#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
		.enable_time_limit = 0 /* disable time limit */
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */
	};

	return	erase_op(&context);
}

static int write(off_t addr, const void *data, size_t len)
{
	struct flash_context context = {
		.data_addr = (u32_t) data,
		.flash_addr = addr,
		.len = len,
#if defined(CONFIG_SOC_FLASH_NRF_RADIO_SYNC)
		.enable_time_limit = 0 /* disable time limit */
#endif /* CONFIG_SOC_FLASH_NRF_RADIO_SYNC */
	};

	return write_op(&context);
}
