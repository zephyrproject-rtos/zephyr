/** @file
 * @brief Bluetooth Controller Ticker functions
 *
 */

/*
 * Copyright (c) 2017-2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include <shell/shell.h>

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

static void ticker_op_done(u32_t err, void *context)
{
	*((u32_t volatile *)context) = err;
}

int cmd_ticker_info(const struct shell *shell, size_t argc, char *argv[])
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
	ticks_to_expire = 0U;
	ticks_current = 0U;
	tickers_count = 0U;
	retry = 4U;
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
			shell_print(shell, "Query done (0x%02x, err= %u).",
				    ticker_id, err);

			break;
		}

		if (ticks_current != ticks_previous) {
			retry--;
			if (!retry) {
				shell_print(shell, "Retry again, tickers too "
					    "busy now.");

				return -EAGAIN;
			}

			if (tickers_count) {
				tickers_count = 0U;

				shell_print(shell, "Query reset, %u retries "
					    "remaining.", retry);
			}
		}

		tickers[tickers_count].id = ticker_id;
		tickers[tickers_count].ticks_to_expire = ticks_to_expire;
		tickers_count++;

	} while (tickers_count < TICKERS_MAX);

	shell_print(shell, "Tickers: %u.", tickers_count);
	shell_print(shell, "Tick: %u (%uus).", ticks_current,
	       HAL_TICKER_TICKS_TO_US(ticks_current));

	if (!tickers_count) {
		return 0;
	}

	shell_print(shell, "---------------------");
	shell_print(shell, " id   offset   offset");
	shell_print(shell, "      (tick)     (us)");
	shell_print(shell, "---------------------");
	for (i = 0U; i < tickers_count; i++) {
		shell_print(shell, "%03u %08u %08u", tickers[i].id,
		       tickers[i].ticks_to_expire,
		       HAL_TICKER_TICKS_TO_US(tickers[i].ticks_to_expire));
	}
	shell_print(shell, "---------------------");

	return 0;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(ticker_cmds,
	SHELL_CMD_ARG(info, NULL, HELP_NONE, cmd_ticker_info, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_ticker(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(shell, "%s:%s%s", argv[0], "unknown parameter: ", argv[1]);
	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(ticker, &ticker_cmds, "Bluetooth Ticker shell commands",
		       cmd_ticker, 1, 1);
