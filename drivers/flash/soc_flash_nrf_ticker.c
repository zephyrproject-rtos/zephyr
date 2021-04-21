/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

#include "soc_flash_nrf.h"

#include <sys/__assert.h>
#include <bluetooth/hci.h>
#include "controller/hal/ticker.h"
#include "controller/ticker/ticker.h"
#include "controller/include/ll.h"

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

static inline int _ticker_stop(uint8_t inst_idx, uint8_t u_id, uint8_t tic_id)
{
	int ret = ticker_stop(inst_idx, u_id, tic_id, NULL, NULL);

	if (ret != TICKER_STATUS_SUCCESS &&
	    ret != TICKER_STATUS_BUSY) {
		__ASSERT(0, "Failed to stop ticker.\n");
	}

	return ret;
}

static void time_slot_callback_work(uint32_t ticks_at_expire,
				    uint32_t remainder,
				    uint16_t lazy, uint8_t force,
				    void *context)
{
	struct flash_op_desc *op_desc;
	uint8_t instance_index;
	uint8_t ticker_id;
	int rc;

	__ASSERT(ll_radio_state_is_idle(),
		 "Radio is on during flash operation.\n");

	op_desc = context;
	rc = op_desc->handler(op_desc->context);
	if (rc != FLASH_OP_ONGOING) {
		ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

		/* Stop the time slot ticker */
		_ticker_stop(instance_index, 0, ticker_id);

		_ticker_sync_context.result = (rc == FLASH_OP_DONE) ? 0 : rc;

		/* notify thread that data is available */
		k_sem_give(&sem_sync);
	}
}

static void time_slot_delay(uint32_t ticks_at_expire, uint32_t ticks_delay,
			    ticker_timeout_func callback, void *context)
{
	uint8_t instance_index;
	uint8_t ticker_id;
	int err;

	ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

	/* start a secondary one-shot ticker after ticks_delay,
	 * this will let any radio role to gracefully abort and release the
	 * Radio h/w.
	 */
	err = ticker_start(instance_index, /* Radio instance ticker */
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

	if (err != TICKER_STATUS_SUCCESS && err != TICKER_STATUS_BUSY) {
		_ticker_sync_context.result = 0;

		/* abort flash timeslots */
		_ticker_stop(instance_index, 0, ticker_id);

		/* notify thread that data is available */
		k_sem_give(&sem_sync);
	}
}

static void time_slot_callback_abort(uint32_t ticks_at_expire,
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
				       uint32_t remainder,
				       uint16_t lazy, uint8_t force,
				       void *context)
{
#if defined(CONFIG_BT_CTLR_LOW_LAT)
	time_slot_callback_abort(ticks_at_expire, remainder, lazy, force,
				 context);
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

	/* FLASH_SYNC_SWITCHING_TIME is delay which is allways added by
	 * the slot calling mechanism
	 */
	_ticker_sync_context.interval = duration - FLASH_SYNC_SWITCHING_TIME;
	_ticker_sync_context.slot = duration;
}

int nrf_flash_sync_exe(struct flash_op_desc *op_desc)
{
	uint8_t instance_index;
	uint8_t ticker_id;
	int result;
	uint32_t err;

	ll_timeslice_ticker_id_get(&instance_index, &ticker_id);

	err = ticker_start(instance_index,
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

	if (err != TICKER_STATUS_SUCCESS && err != TICKER_STATUS_BUSY) {
		result = -ECANCELED;
	} else if (k_sem_take(&sem_sync, K_MSEC(FLASH_TIMEOUT_MS)) != 0) {
		/* Stop any scheduled jobs */
		_ticker_stop(instance_index, 3, ticker_id);

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
