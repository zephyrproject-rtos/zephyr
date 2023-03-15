/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/bluetooth/hci.h>

#include "hal/ticker.h"
#include "ticker/ticker.h"
#include "ll.h"

#include "soc_flash_nrf.h"

#define FLASH_RADIO_ABORT_DELAY_US 1500
#define FLASH_RADIO_WORK_DELAY_US  200

/* delay needed for start execution-window */
#define FLASH_SYNC_SWITCHING_TIME (FLASH_RADIO_ABORT_DELAY_US +\
				   FLASH_RADIO_WORK_DELAY_US)

struct ticker_sync_context {
	uint32_t interval;    /* timeslot interval. */
	uint32_t slot;        /* timeslot length. */
	uint32_t ticks_begin; /* timeslot begin timestamp */
	int result;
};

static struct ticker_sync_context _ticker_sync_context;

/* semaphore for synchronization of flash operations */
static struct k_sem sem_sync;

static void ticker_stop_work_cb(uint32_t status, void *param)
{
	__ASSERT((status == TICKER_STATUS_SUCCESS ||
		  status == TICKER_STATUS_FAILURE),
		 "Failed to stop work ticker, ticker job busy.\n");

	/* notify thread that data is available */
	k_sem_give(&sem_sync);
}

static void ticker_stop_prepare_cb(uint32_t status, void *param)
{
	uint8_t instance_index;
	uint8_t ticker_id;
	uint32_t ret;

	__ASSERT(status == TICKER_STATUS_SUCCESS,
		 "Failed to stop prepare ticker.\n");

	/* Get the ticker instance and ticker id for flash operations */
	ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

	/* Stop the work ticker, from ULL_LOW context */
	ret = ticker_stop(instance_index, 2U, (ticker_id + 1U),
			  ticker_stop_work_cb, NULL);
	__ASSERT((ret == TICKER_STATUS_SUCCESS ||
		  ret == TICKER_STATUS_BUSY),
		 "Failed to request the work ticker to stop.\n");
}

static void time_slot_callback_work(uint32_t ticks_at_expire,
				    uint32_t ticks_drift,
				    uint32_t remainder,
				    uint16_t lazy, uint8_t force,
				    void *context)
{
	struct flash_op_desc *op_desc;
	int rc;

	__ASSERT(ll_radio_state_is_idle(),
		 "Radio is on during flash operation.\n");

	op_desc = context;
	rc = op_desc->handler(op_desc->context);
	if (rc != FLASH_OP_ONGOING) {
		uint8_t instance_index;
		uint8_t ticker_id;
		uint32_t ret;

		/* Get the ticker instance and ticker id for flash operations */
		ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

		/* Stop the prepare ticker, from ULL_HIGH context */
		ret = ticker_stop(instance_index, 1U, ticker_id,
				  ticker_stop_prepare_cb, NULL);
		__ASSERT((ret == TICKER_STATUS_SUCCESS ||
			  ret == TICKER_STATUS_BUSY),
			 "Failed to stop ticker.\n");

		_ticker_sync_context.result = (rc == FLASH_OP_DONE) ? 0 : rc;
	}
}

static void time_slot_delay(uint32_t ticks_at_expire, uint32_t ticks_delay,
			    ticker_timeout_func callback, void *context)
{
	uint8_t instance_index;
	uint8_t ticker_id;
	uint32_t ret;

	/* Get the ticker instance and ticker id for flash operations */
	ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

	/* Start a secondary one-shot ticker after ticks_delay,
	 * this will let any radio role to gracefully abort and release the
	 * Radio h/w.
	 */
	ret = ticker_start(instance_index, /* Radio instance ticker */
			   1, /* user id for link layer ULL_HIGH */
			      /* (MAYFLY_CALL_ID_WORKER) */
			   (ticker_id + 1), /* ticker_id */
			   ticks_at_expire, /* current tick */
			   ticks_delay, /* one-shot delayed timeout */
			   0, /* periodic timeout  */
			   0, /* periodic remainder */
			   0, /* lazy, voluntary skips */
			   0,
			   callback, /* handler for executing radio abort or */
				     /* flash work */
			   context, /* the context for the flash operation */
			   NULL, /* no op callback */
			   NULL);

	if (ret != TICKER_STATUS_SUCCESS && ret != TICKER_STATUS_BUSY) {
		/* Failed to enqueue the ticker start operation request */
		_ticker_sync_context.result = 0;

		/* Abort flash prepare ticker, from ULL_HIGH context */
		ret = ticker_stop(instance_index, 1U, ticker_id,
				  ticker_stop_prepare_cb, NULL);
		__ASSERT((ret == TICKER_STATUS_SUCCESS ||
			  ret == TICKER_STATUS_BUSY),
			 "Failed to stop ticker.\n");
	}
}

static void time_slot_callback_abort(uint32_t ticks_at_expire,
				     uint32_t ticks_drift,
				     uint32_t remainder,
				     uint16_t lazy, uint8_t force,
				     void *context)
{
	ll_radio_state_abort();
	time_slot_delay(ticks_at_expire,
			HAL_TICKER_US_TO_TICKS(FLASH_RADIO_WORK_DELAY_US),
			time_slot_callback_work,
			context);
}

static void time_slot_callback_prepare(uint32_t ticks_at_expire,
				       uint32_t ticks_drift,
				       uint32_t remainder,
				       uint16_t lazy, uint8_t force,
				       void *context)
{
#if defined(CONFIG_BT_CTLR_LOW_LAT)
	time_slot_callback_abort(ticks_at_expire, ticks_drift, remainder, lazy,
				 force, context);
#else /* !CONFIG_BT_CTLR_LOW_LAT */
	time_slot_delay(ticks_at_expire,
			HAL_TICKER_US_TO_TICKS(FLASH_RADIO_ABORT_DELAY_US),
			time_slot_callback_abort,
			context);
#endif /* CONFIG_BT_CTLR_LOW_LAT */
}


int nrf_flash_sync_init(void)
{
	k_sem_init(&sem_sync, 0, 1);
	return 0;
}

void nrf_flash_sync_set_context(uint32_t duration)
{
	/* FLASH_SYNC_SWITCHING_TIME is delay which is always added by
	 * the slot calling mechanism
	 */
	_ticker_sync_context.interval = duration - FLASH_SYNC_SWITCHING_TIME;
	_ticker_sync_context.slot = duration;
}

int nrf_flash_sync_exe(struct flash_op_desc *op_desc)
{
	uint8_t instance_index;
	uint8_t ticker_id;
	uint32_t ret;
	int result;

	/* Get the ticker instance and ticker id for flash operations */
	ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

	/* Start periodic flash operation prepare time slots */
	ret = ticker_start(instance_index,
			   3, /* user id for thread mode */
			      /* (MAYFLY_CALL_ID_PROGRAM) */
			   ticker_id, /* flash ticker id */
			   ticker_ticks_now_get(), /* current tick */
			   0, /* first int. immediately */
			   /* period */
			   HAL_TICKER_US_TO_TICKS(
				_ticker_sync_context.interval),
			   /* period remainder */
			   HAL_TICKER_REMAINDER(_ticker_sync_context.interval),
			   0, /* lazy, voluntary skips */
			   HAL_TICKER_US_TO_TICKS(_ticker_sync_context.slot),
			   time_slot_callback_prepare,
			   op_desc,
			   NULL, /* no op callback */
			   NULL);

	if (ret != TICKER_STATUS_SUCCESS && ret != TICKER_STATUS_BUSY) {
		/* Failed to enqueue the ticker start operation request */
		result = -ECANCELED;
	} else if (k_sem_take(&sem_sync, K_MSEC(FLASH_TIMEOUT_MS)) != 0) {
		/* Stop any scheduled jobs, from thread context */
		ret = ticker_stop(instance_index, 3U, ticker_id, NULL, NULL);
		__ASSERT((ret == TICKER_STATUS_SUCCESS ||
			  ret == TICKER_STATUS_BUSY),
			 "Failed to stop ticker.\n");

		/* wait for operation's complete overrun*/
		result = -ETIMEDOUT;
	} else {
		result = _ticker_sync_context.result;
	}

	return result;
}

bool nrf_flash_sync_is_required(void)
{
	return ticker_is_initialized(0);
}

void nrf_flash_sync_get_timestamp_begin(void)
{
	_ticker_sync_context.ticks_begin = ticker_ticks_now_get();
}

bool nrf_flash_sync_check_time_limit(uint32_t iteration)
{
	uint32_t ticks_diff;

	ticks_diff = ticker_ticks_diff_get(ticker_ticks_now_get(),
					   _ticker_sync_context.ticks_begin);
	if (ticks_diff + ticks_diff/iteration >
	    HAL_TICKER_US_TO_TICKS(_ticker_sync_context.slot)) {
		return true;
	}

	return false;
}
