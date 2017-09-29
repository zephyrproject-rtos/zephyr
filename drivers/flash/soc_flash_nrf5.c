/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
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
#include <flash.h>
#include <string.h>

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
#include <misc/__assert.h>
#include <bluetooth/hci.h>
#include "controller/ticker/ticker.h"
#include "controller/include/ll.h"

#define FLASH_SLOT     FLASH_PAGE_ERASE_MAX_TIME_US
#define FLASH_INTERVAL FLASH_SLOT
#define FLASH_RADIO_ABORT_DELAY_US 500
#define FLASH_TIMEOUT_MS ((FLASH_PAGE_ERASE_MAX_TIME_US)\
			* (FLASH_PAGE_MAX_CNT) / 1000)
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */

#define FLASH_OP_DONE    (0) /* 0 for compliance with the driver API. */
#define FLASH_OP_ONGOING (-1)

struct erase_context {
	u32_t addr; /* Address off the 1st page to erase */
	u32_t size; /* Size off area to erase [B] */
#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
	u8_t enable_time_limit; /* execution limited to timeslot */
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */
}; /*< Context type for f. @ref erase_op */

struct write_context {
	u32_t data_addr;
	u32_t flash_addr; /* Address off the 1st page to erase */
	u32_t len;        /* Size off data to write [B] */
#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
	u8_t  enable_time_limit; /* execution limited to timeslot*/
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */
}; /*< Context type for f. @ref write_op */

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
typedef int (*flash_op_handler_t) (void *context);

struct flash_op_desc {
	flash_op_handler_t handler;
	void *context; /* [in,out] */
	int result;
};

/* semaphore for synchronization of flash operations */
static struct k_sem sem_sync;

static int write_op(void *context); /* instance of flash_op_handler_t */
static int write_in_timeslice(off_t addr, const void *data, size_t len);

static int erase_op(void *context); /* instance of flash_op_handler_t */
static int erase_in_timeslice(u32_t addr, u32_t size);
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */

/* semaphore for locking flash resources (tickers) */
static struct k_sem sem_lock;

static int write(off_t addr, const void *data, size_t len);
static int erase(u32_t addr, u32_t size);

static inline bool is_aligned_32(u32_t data)
{
	return (data & 0x3) ? false : true;
}

static inline bool is_addr_valid(off_t addr, size_t len)
{
	if (addr + len > NRF_FICR->CODEPAGESIZE * NRF_FICR->CODESIZE ||
			addr < 0) {
		return false;
	}

	return true;
}

static void nvmc_wait_ready(void)
{
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
		;
	}
}

static int flash_nrf5_read(struct device *dev, off_t addr,
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

static int flash_nrf5_write(struct device *dev, off_t addr,
			     const void *data, size_t len)
{
	int ret;

	if (!is_addr_valid(addr, len)) {
		return -EINVAL;
	}

	if (!len) {
		return 0;
	}

	k_sem_take(&sem_lock, K_FOREVER);

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
	if (ticker_is_initialized(0)) {
		ret = write_in_timeslice(addr, data, len);
	} else
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */
	{
		ret = write(addr, data, len);
	}

	k_sem_give(&sem_lock);

	return ret;
}

static int flash_nrf5_erase(struct device *dev, off_t addr, size_t size)
{
	u32_t pg_size = NRF_FICR->CODEPAGESIZE;
	u32_t n_pages = size / pg_size;
	int ret;

	/* Erase can only be done per page */
	if (((addr % pg_size) != 0) || ((size % pg_size) != 0)) {
		return -EINVAL;
	}

	if (!is_addr_valid(addr, size)) {
		return -EINVAL;
	}

	if (!n_pages) {
		return 0;
	}

	k_sem_take(&sem_lock, K_FOREVER);

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
	if (ticker_is_initialized(0)) {
		ret = erase_in_timeslice(addr, size);
	} else
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */
	{
		ret = erase(addr, size);
	}

	k_sem_give(&sem_lock);

	return ret;
}

static int flash_nrf5_write_protection(struct device *dev, bool enable)
{
	k_sem_take(&sem_lock, K_FOREVER);

	if (enable) {
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
	} else {
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
	}
	nvmc_wait_ready();

	k_sem_give(&sem_lock);

	return 0;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static struct flash_pages_layout dev_layout;

static void flash_nrf5_pages_layout(struct device *dev,
				     const struct flash_pages_layout **layout,
				     size_t *layout_size)
{
	*layout = &dev_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_nrf5_api = {
	.read = flash_nrf5_read,
	.write = flash_nrf5_write,
	.erase = flash_nrf5_erase,
	.write_protection = flash_nrf5_write_protection,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_nrf5_pages_layout,
#endif
	.write_block_size = 1,
};

static int nrf5_flash_init(struct device *dev)
{
	dev->driver_api = &flash_nrf5_api;

	k_sem_init(&sem_lock, 1, 1);

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
	k_sem_init(&sem_sync, 0, 1);
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	dev_layout.pages_count = NRF_FICR->CODESIZE;
	dev_layout.pages_size = NRF_FICR->CODEPAGESIZE;
#endif

	return 0;
}

DEVICE_INIT(nrf5_flash, CONFIG_SOC_FLASH_NRF5_DEV_NAME, nrf5_flash_init,
	     NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)

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
			   TICKER_US_TO_TICKS(FLASH_RADIO_ABORT_DELAY_US),
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

	ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

	err = ticker_start(instance_index,
			   3, /* user id for thread mode */
			      /* (MAYFLY_CALL_ID_PROGRAM) */
			   ticker_id, /* flash ticker id */
			   ticker_ticks_now_get(), /* current tick */
			   0, /* first int. immediately */
			   TICKER_US_TO_TICKS(FLASH_INTERVAL), /* periodic */
			   TICKER_REMAINDER(FLASH_INTERVAL), /* per. remaind.*/
			   0, /* lazy, voluntary skips */
			   TICKER_US_TO_TICKS(FLASH_SLOT),
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
	struct erase_context context = {
		.addr = addr,
		.size = size,
		.enable_time_limit = 1 /* enable time limit */
	};

	struct flash_op_desc flash_op_desc = {
		.handler = erase_op,
		.context = &context
	};

	return work_in_time_slice(&flash_op_desc);
}

static int write_in_timeslice(off_t addr, const void *data, size_t len)
{
	struct write_context context = {
		.data_addr = (u32_t) data,
		.flash_addr = addr,
		.len = len,
		.enable_time_limit = 1 /* enable time limit */
	};

	struct flash_op_desc flash_op_desc = {
		.handler = write_op,
		.context = &context
	};

	return  work_in_time_slice(&flash_op_desc);
}

#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */

static int erase_op(void *context)
{
	u32_t prev_nvmc_cfg = NRF_NVMC->CONFIG;
	u32_t pg_size = NRF_FICR->CODEPAGESIZE;
	struct erase_context *e_ctx = context;

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
	u32_t ticks_begin = 0;
	u32_t ticks_diff;
	u32_t i = 0;

	if (e_ctx->enable_time_limit) {
		ticks_begin = ticker_ticks_now_get();
	}
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */

	/* Erase uses a specific configuration register */
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
	nvmc_wait_ready();

	do {
		NRF_NVMC->ERASEPAGE = e_ctx->addr;
		nvmc_wait_ready();

		e_ctx->size -= pg_size;
		e_ctx->addr += pg_size;

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
		i++;

		if (e_ctx->enable_time_limit) {
			ticks_diff = ticker_ticks_now_get() - ticks_begin;
			if (ticks_diff + ticks_diff/i > FLASH_SLOT) {
				break;
			}
		}
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */

	} while (e_ctx->size > 0);

	NRF_NVMC->CONFIG = prev_nvmc_cfg;
	nvmc_wait_ready();

	return (e_ctx->size > 0) ? FLASH_OP_ONGOING : FLASH_OP_DONE;
}

static void shift_write_context(u32_t shift, struct write_context *w_ctx)
{
	w_ctx->flash_addr += shift;
	w_ctx->data_addr += shift;
	w_ctx->len -= shift;
}

static int write_op(void *context)
{
	struct write_context *w_ctx = context;
	u32_t addr_word;
	u32_t tmp_word;
	u32_t count;

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
	u32_t ticks_begin = 0;
	u32_t ticks_diff;
	u32_t i = 1;

	if (w_ctx->enable_time_limit) {
		ticks_begin = ticker_ticks_now_get();
	}
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */

	/* Start with a word-aligned address and handle the offset */
	addr_word = (u32_t)w_ctx->flash_addr & ~0x3;

	/* If not aligned, read first word, update and write it back */
	if (!is_aligned_32(w_ctx->flash_addr)) {
		tmp_word = *(u32_t *)(addr_word);

		count = sizeof(u32_t) - (w_ctx->flash_addr & 0x3);
		if (count > w_ctx->len) {
			count = w_ctx->len;
		}

		memcpy((u8_t *)&tmp_word + (w_ctx->flash_addr & 0x3),
		       (void *)w_ctx->data_addr,
		       count);
		nvmc_wait_ready();
		*(u32_t *)addr_word = tmp_word;

		shift_write_context(count, w_ctx);

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
		if (w_ctx->enable_time_limit) {
			ticks_diff = ticker_ticks_now_get() - ticks_begin;
			if (2 * ticks_diff > FLASH_SLOT) {
				nvmc_wait_ready();
				return FLASH_OP_ONGOING;
			}
		}
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */
	}

	/* Write all the 4-byte aligned data */
	while (w_ctx->len >= sizeof(u32_t)) {
		nvmc_wait_ready();
		*(u32_t *)w_ctx->flash_addr =
				UNALIGNED_GET((u32_t *)w_ctx->data_addr);

		shift_write_context(sizeof(u32_t), w_ctx);

#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
		i++;

		if (w_ctx->enable_time_limit) {
			ticks_diff = ticker_ticks_now_get() - ticks_begin;

			if (ticks_diff + ticks_diff/i > FLASH_SLOT) {
				nvmc_wait_ready();
				return FLASH_OP_ONGOING;
			}
		}
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */
	}

	/* Write remaining data */
	if (w_ctx->len) {
		tmp_word = *(u32_t *)(w_ctx->flash_addr);
		memcpy((u8_t *)&tmp_word, (void *)w_ctx->data_addr, w_ctx->len);
		nvmc_wait_ready();
		*(u32_t *)w_ctx->flash_addr = tmp_word;

		shift_write_context(w_ctx->len, w_ctx);
	}

	nvmc_wait_ready();

	return FLASH_OP_DONE;
}

static int erase(u32_t addr, u32_t size)
{
	struct erase_context context = {
		.addr = addr,
		.size = size,
#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
		.enable_time_limit = 0 /* disable time limit */
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */
	};

	return	erase_op(&context);
}

static int write(off_t addr, const void *data, size_t len)
{
	struct write_context context = {
		.data_addr = (u32_t) data,
		.flash_addr = addr,
		.len = len,
#if defined(CONFIG_SOC_FLASH_NRF5_RADIO_SYNC)
		.enable_time_limit = 0 /* disable time limit */
#endif /* CONFIG_SOC_FLASH_NRF5_RADIO_SYNC */
	};

	return write_op(&context);
}
