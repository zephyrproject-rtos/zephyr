/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/**
 * Should be included from sof/schedule/task.h
 * but triggers include chain issue
 * FIXME
 */
int sof_main(int argc, char *argv[]);

/**
 * TODO: Here comes SOF initialization
 */

void main(void)
{
	int ret;

	LOG_INF("SOF on %s", CONFIG_BOARD);

	/* sof_main is actually SOF initialization */
	ret = sof_main(0, NULL);
	if (ret) {
		LOG_ERR("SOF initialization failed");
	}

	LOG_INF("SOF initialized");
}
