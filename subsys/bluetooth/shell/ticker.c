/** @file
 * @brief Bluetooth Controller Ticker functions
 *
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <shell/shell.h>
#include <misc/printk.h>

#include "../controller/util/mayfly.h"
#include "../controller/ticker/ticker.h"

#define TICKER_SHELL_MODULE "ticker"

#if defined(CONFIG_BLUETOOTH_MAX_CONN)
#define TICKERS_MAX (CONFIG_BLUETOOTH_MAX_CONN + 2)
#else
#define TICKERS_MAX 2
#endif

static void ticker_op_done(u32_t err, void *context)
{
	*((u32_t volatile *)context) = err;
}

int cmd_ticker_info(int argc, char *argv[])
{
	struct {
		u8_t id;
		u32_t ticks_to_expire;
	} tickers[TICKERS_MAX];
	u32_t ticks_to_expire;
	u32_t ticks_current;
	u8_t tickers_count;
	u8_t ticker_id;
	u8_t retry;
	u8_t i;

	ticker_id = TICKER_NULL;
	ticks_to_expire = 0;
	ticks_current = 0;
	tickers_count = 0;
	retry = 4;
	do {
		u32_t volatile err_cb = TICKER_STATUS_BUSY;
		u32_t ticks_previous;
		u32_t err;

		ticks_previous = ticks_current;

		err = ticker_next_slot_get(0, MAYFLY_CALL_ID_PROGRAM,
					   &ticker_id, &ticks_current,
					   &ticks_to_expire,
					   ticker_op_done, (void *)&err_cb);
		if (err == TICKER_STATUS_BUSY) {
			while (err_cb == TICKER_STATUS_BUSY) {
				ticker_job_sched(0, MAYFLY_CALL_ID_PROGRAM);
			}
		}

		if ((err_cb != TICKER_STATUS_SUCCESS) ||
		    (ticker_id == TICKER_NULL)) {
			printk("Query done (0x%02x, err= %u).\n", ticker_id,
			       err);

			break;
		}

		if (ticks_current != ticks_previous) {
			retry--;
			if (!retry) {
				printk("Retry again, tickers too busy now.\n");

				return -EAGAIN;
			}

			if (tickers_count) {
				tickers_count = 0;

				printk("Query reset, %u retries remaining.\n",
				       retry);
			}
		}

		tickers[tickers_count].id = ticker_id;
		tickers[tickers_count].ticks_to_expire = ticks_to_expire;
		tickers_count++;

	} while (tickers_count < TICKERS_MAX);

	printk("Tickers: %u.\n", tickers_count);
	printk("Tick: %u (%uus).\n", ticks_current,
	       TICKER_TICKS_TO_US(ticks_current));

	if (!tickers_count) {
		return 0;
	}

	printk("---------------------\n");
	printk(" id   offset   offset\n");
	printk("      (tick)     (us)\n");
	printk("---------------------\n");
	for (i = 0; i < tickers_count; i++) {
		printk("%03u %08u %08u\n", tickers[i].id,
		       tickers[i].ticks_to_expire,
		       TICKER_TICKS_TO_US(tickers[i].ticks_to_expire));
	}
	printk("---------------------\n");

	return 0;
}

static const struct shell_cmd ticker_commands[] = {
	{ "info", cmd_ticker_info, "Enumerate active ticker details."},
	{ NULL, NULL, NULL}
};

SHELL_REGISTER(TICKER_SHELL_MODULE, ticker_commands);
