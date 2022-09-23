/** @file
 * @brief Bluetooth Controller Ticker functions
 *
 */

/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/shell/shell.h>

#include "../controller/util/memq.h"
#include "../controller/util/mayfly.h"
#include "../controller/hal/ticker.h"
#include "../controller/ticker/ticker.h"

#if defined(CONFIG_BT_MAX_CONN)
#define TICKERS_MAX (CONFIG_BT_MAX_CONN + 2)
#else
#define TICKERS_MAX 2
#endif

#include "bt.h"

static void ticker_op_done(uint32_t err, void *context)
{
	*((uint32_t volatile *)context) = err;
}

int cmd_ticker_info(const struct shell *sh, size_t argc, char *argv[])
{
	struct {
		uint8_t id;
		uint32_t ticks_to_expire;
	} tickers[TICKERS_MAX];
	uint32_t ticks_to_expire;
	uint32_t ticks_current;
	uint8_t tickers_count;
	uint8_t ticker_id;
	uint8_t retry;
	uint8_t i;

	ticker_id = TICKER_NULL;
	ticks_to_expire = 0U;
	ticks_current = 0U;
	tickers_count = 0U;
	retry = 4U;
	do {
		uint32_t volatile err_cb = TICKER_STATUS_BUSY;
		uint32_t ticks_previous;
		uint32_t err;

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
			shell_print(sh, "Query done (0x%02x, err= %u).",
				    ticker_id, err);

			break;
		}

		if (ticks_current != ticks_previous) {
			retry--;
			if (!retry) {
				shell_print(sh, "Retry again, tickers too "
					    "busy now.");

				return -EAGAIN;
			}

			if (tickers_count) {
				tickers_count = 0U;

				shell_print(sh, "Query reset, %u retries "
					    "remaining.", retry);
			}
		}

		tickers[tickers_count].id = ticker_id;
		tickers[tickers_count].ticks_to_expire = ticks_to_expire;
		tickers_count++;

	} while (tickers_count < TICKERS_MAX);

	shell_print(sh, "Tickers: %u.", tickers_count);
	shell_print(sh, "Tick: %u (%uus).", ticks_current,
	       HAL_TICKER_TICKS_TO_US(ticks_current));

	if (!tickers_count) {
		return 0;
	}

	shell_print(sh, "---------------------");
	shell_print(sh, " id   offset   offset");
	shell_print(sh, "      (tick)     (us)");
	shell_print(sh, "---------------------");
	for (i = 0U; i < tickers_count; i++) {
		shell_print(sh, "%03u %08u %08u", tickers[i].id,
		       tickers[i].ticks_to_expire,
		       HAL_TICKER_TICKS_TO_US(tickers[i].ticks_to_expire));
	}
	shell_print(sh, "---------------------");

	return 0;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(ticker_cmds,
	SHELL_CMD_ARG(info, NULL, HELP_NONE, cmd_ticker_info, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_ticker(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s:%s%s", argv[0], "unknown parameter: ", argv[1]);
	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(ticker, &ticker_cmds, "Bluetooth Ticker shell commands",
		       cmd_ticker, 1, 1);
