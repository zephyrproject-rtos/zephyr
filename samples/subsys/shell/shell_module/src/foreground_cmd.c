/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <zephyr.h>

#define FOREGROUND_TIMER_HELP \
	"Demo command showing printing to shell from various contexts"

#define FOREGROUND_TIMER_START_HELP \
	"Start periodic timer. Timer handler prints to shell. CTRL+C" \
	" terminates printing. Timer keeps running in background."

#define FOREGROUND_TIMER_STOP_HELP \
	"Stop timer."

#define FOREGROUND_TIMER_SINGLE_SHOT_HELP \
	"Start single shot timer."

struct timer_work_duo {
	struct k_timer timer;
	struct k_work work;
	const struct shell *shell;
	bool single_shot;
	bool running;
};

static struct timer_work_duo data;

static void timer_expired_work(struct k_work *work)
{
	struct timer_work_duo *data =
			CONTAINER_OF(work, struct timer_work_duo, work);

	shell_print(data->shell, "Timer expired.");

	if (data->single_shot) {
		shell_print(data->shell, "Single shot timer command exits.");
		shell_command_exit(data->shell);
		data->running = false;
	}
}

static void timer_handler(struct k_timer *timer)
{
	struct timer_work_duo *data =
			CONTAINER_OF(timer, struct timer_work_duo, timer);

	k_work_submit(&data->work);
}

static int start_timer(const struct shell *shell, bool single_shot)
{

	if (data.running) {
		shell_error(shell, "Timer is already started.");
		return -ENOEXEC;
	}

	k_timer_init(&data.timer, timer_handler, NULL);
	k_work_init(&data.work, timer_expired_work);
	data.shell = shell;
	data.single_shot = single_shot;
	data.running = true;

	k_timer_start(&data.timer, 1000, single_shot ? 0 : 1000);

	shell_command_enter(shell);

	return 0;

}
static int cmd_fg_timer_start(const struct shell *shell,
			      size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err;

	err = start_timer(shell, false);

	if (err == 0) {
		shell_print(shell, "Periodic timer started.");
	}

	return err;
}

static int cmd_fg_timer_single_shot(const struct shell *shell,
				    size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err;

	err = start_timer(shell, true);

	if (err == 0) {
		shell_print(shell, "Single shot started.");
	}

	return err;
}

static int cmd_fg_timer_stop(const struct shell *shell,
			      size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	k_timer_stop(&data.timer);
	data.running = false;

	shell_print(shell, "Timer stopped.");

	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(sub_foreground_timer)
{
	/* Alphabetically sorted. */
	SHELL_CMD_ARG(start, NULL, FOREGROUND_TIMER_START_HELP,
			cmd_fg_timer_start, 1, 0),
	SHELL_CMD_ARG(stop, NULL, FOREGROUND_TIMER_STOP_HELP,
			cmd_fg_timer_stop, 1, 0),
	SHELL_CMD_ARG(single_shot, NULL, FOREGROUND_TIMER_SINGLE_SHOT_HELP,
			cmd_fg_timer_single_shot, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
};

SHELL_CMD_ARG_REGISTER(foreground_timer, &sub_foreground_timer,
		   FOREGROUND_TIMER_HELP, NULL, 1, 1);
