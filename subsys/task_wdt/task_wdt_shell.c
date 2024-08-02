/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/task_wdt/task_wdt.h>

static int cmd_init(const struct shell *sh, size_t argc, char *argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#ifdef CONFIG_TASK_WDT_HW_FALLBACK
	const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
#else
	const struct device *const wdt = NULL;
#endif

	shell_fprintf(sh, SHELL_INFO, "Init task watchdog ...\n");

	int ret = task_wdt_init(wdt);

	if (ret < 0) {
		shell_fprintf(sh, SHELL_ERROR, "Failed to init task watchdog: %d\n", ret);
		return ret;
	}

	return 0;
}

static int cmd_add(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_fprintf(sh, SHELL_ERROR, "Invalid number of arguments\n");
		return -EINVAL;
	}

	shell_fprintf(sh, SHELL_INFO, "Add task watchdog channel\n");

	uint32_t period = atoi(argv[1]) * MSEC_PER_SEC;

	int ret = task_wdt_add(period, NULL, NULL);

	if (ret < 0) {
		shell_fprintf(sh, SHELL_ERROR, "Failed to add task watchdog channel: %d\n", ret);
		return ret;
	}

	shell_fprintf(sh, SHELL_INFO, "Task watchdog channel: %d\n", ret);

	shell_fprintf(sh, SHELL_NORMAL,
		      "Use \"task_wdt feed %d\" to feed this channel\n"
		      "and \"task_wdt del %d\" to delete this channel\n",
		      ret, ret);

	return 0;
}

static int cmd_feed(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_fprintf(sh, SHELL_ERROR, "Invalid number of arguments\n");
		return -EINVAL;
	}

	shell_fprintf(sh, SHELL_INFO, "Feed task watchdog channel %s\n", argv[1]);

	int ret = task_wdt_feed(atoi(argv[1]));

	if (ret < 0) {
		shell_fprintf(sh, SHELL_ERROR, "Failed to add task watchdog channel: %d\n", ret);
		return ret;
	}

	return 0;
}

static int cmd_del(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_fprintf(sh, SHELL_ERROR, "Invalid number of arguments\n");
		return -EINVAL;
	}

	shell_fprintf(sh, SHELL_INFO, "Delete task watchdog channel %s\n", argv[1]);

	int ret = task_wdt_delete(atoi(argv[1]));

	if (ret < 0) {
		shell_fprintf(sh, SHELL_ERROR, "Failed to delete task watchdog channel: %d\n", ret);
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_task_wdt,
	SHELL_CMD(init, NULL, "Initialize task watchdog", cmd_init),
	SHELL_CMD(add, NULL, "Install new timeout (time in seconds)", cmd_add),
	SHELL_CMD(feed, NULL, "Feed specified watchdog channel", cmd_feed),
	SHELL_CMD(del, NULL, "Delete task watchdog channel", cmd_del),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(task_wdt, &sub_task_wdt, "Task watchdog commands", NULL);
