/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <version.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <ctype.h>

#ifdef CONFIG_ARCH_POSIX
#include <unistd.h>
#else
#include <zephyr/posix/unistd.h>
#endif

LOG_MODULE_REGISTER(app);

extern void foo(void);

void timer_expired_handler(struct k_timer *timer)
{
	LOG_INF("Timer expired.");

	/* Call another module to present logging from multiple sources. */
	foo();
}

K_TIMER_DEFINE(log_timer, timer_expired_handler, NULL);

static int cmd_log_test_start(const struct shell *sh, size_t argc,
			      char **argv, uint32_t period)
{
	ARG_UNUSED(argv);

	k_timer_start(&log_timer, K_MSEC(period), K_MSEC(period));
	shell_print(sh, "Log test started\n");

	return 0;
}

static int cmd_log_test_start_demo(const struct shell *sh, size_t argc,
				   char **argv)
{
	return cmd_log_test_start(sh, argc, argv, 200);
}

static int cmd_log_test_start_flood(const struct shell *sh, size_t argc,
				    char **argv)
{
	return cmd_log_test_start(sh, argc, argv, 10);
}

static int cmd_log_test_stop(const struct shell *sh, size_t argc,
			     char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	k_timer_stop(&log_timer);
	shell_print(sh, "Log test stopped");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_log_test_start,
	SHELL_CMD_ARG(demo, NULL,
		  "Start log timer which generates log message every 200ms.",
		  cmd_log_test_start_demo, 1, 0),
	SHELL_CMD_ARG(flood, NULL,
		  "Start log timer which generates log message every 10ms.",
		  cmd_log_test_start_flood, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_STATIC_SUBCMD_SET_CREATE(sub_log_test,
	SHELL_CMD_ARG(start, &sub_log_test_start, "Start log test", NULL, 2, 0),
	SHELL_CMD_ARG(stop, NULL, "Stop log test.", cmd_log_test_stop, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(log_test, &sub_log_test, "Log test", NULL);

static int cmd_demo_ping(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "pong");

	return 0;
}

static int cmd_demo_board(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, CONFIG_BOARD);

	return 0;
}

#if defined CONFIG_SHELL_GETOPT
/* Thread save usage */
static int cmd_demo_getopt_ts(const struct shell *sh, size_t argc,
			      char **argv)
{
	struct getopt_state *state;
	char *cvalue = NULL;
	int aflag = 0;
	int bflag = 0;
	int c;

	while ((c = getopt(argc, argv, "abhc:")) != -1) {
		state = getopt_state_get();
		switch (c) {
		case 'a':
			aflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'c':
			cvalue = state->optarg;
			break;
		case 'h':
			/* When getopt is active shell is not parsing
			 * command handler to print help message. It must
			 * be done explicitly.
			 */
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if (state->optopt == 'c') {
				shell_print(sh,
					"Option -%c requires an argument.",
					state->optopt);
			} else if (isprint(state->optopt) != 0) {
				shell_print(sh,
					"Unknown option `-%c'.",
					state->optopt);
			} else {
				shell_print(sh,
					"Unknown option character `\\x%x'.",
					state->optopt);
			}
			return 1;
		default:
			break;
		}
	}

	shell_print(sh, "aflag = %d, bflag = %d", aflag, bflag);
	return 0;
}

static int cmd_demo_getopt(const struct shell *sh, size_t argc,
			      char **argv)
{
	char *cvalue = NULL;
	int aflag = 0;
	int bflag = 0;
	int c;

	while ((c = getopt(argc, argv, "abhc:")) != -1) {
		switch (c) {
		case 'a':
			aflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'c':
			cvalue = optarg;
			break;
		case 'h':
			/* When getopt is active shell is not parsing
			 * command handler to print help message. It must
			 * be done explicitly.
			 */
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if (optopt == 'c') {
				shell_print(sh,
					"Option -%c requires an argument.",
					optopt);
			} else if (isprint(optopt) != 0) {
				shell_print(sh, "Unknown option `-%c'.",
					optopt);
			} else {
				shell_print(sh,
					"Unknown option character `\\x%x'.",
					optopt);
			}
			return 1;
		default:
			break;
		}
	}

	shell_print(sh, "aflag = %d, bflag = %d", aflag, bflag);
	return 0;
}
#endif

static int cmd_demo_params(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "argc = %zd", argc);
	for (size_t cnt = 0; cnt < argc; cnt++) {
		shell_print(sh, "  argv[%zd] = %s", cnt, argv[cnt]);
	}

	return 0;
}

static int cmd_demo_hexdump(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "argc = %zd", argc);
	for (size_t cnt = 0; cnt < argc; cnt++) {
		shell_print(sh, "argv[%zd]", cnt);
		shell_hexdump(sh, argv[cnt], strlen(argv[cnt]));
	}

	return 0;
}

static int cmd_version(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Zephyr version %s", KERNEL_VERSION_STRING);

	return 0;
}

#define DEFAULT_PASSWORD "zephyr"

static void login_init(void)
{
	printk("Shell Login Demo\nHint: password = %s\n", DEFAULT_PASSWORD);
	if (!CONFIG_SHELL_CMD_ROOT[0]) {
		shell_set_root_cmd("login");
	}
}

static int check_passwd(char *passwd)
{
	/* example only -- not recommended for production use */
	return strcmp(passwd, DEFAULT_PASSWORD);
}

static int cmd_login(const struct shell *sh, size_t argc, char **argv)
{
	static uint32_t attempts;

	if (check_passwd(argv[1]) != 0) {
		shell_error(sh, "Incorrect password!");
		attempts++;
		if (attempts > 3) {
			k_sleep(K_SECONDS(attempts));
		}
		return -EINVAL;
	}

	/* clear history so password not visible there */
	z_shell_history_purge(sh->history);
	shell_obscure_set(sh, false);
	shell_set_root_cmd(NULL);
	shell_prompt_change(sh, "uart:~$ ");
	shell_print(sh, "Shell Login Demo\n");
	shell_print(sh, "Hit tab for help.\n");
	attempts = 0;
	return 0;
}

static int cmd_logout(const struct shell *sh, size_t argc, char **argv)
{
	shell_set_root_cmd("login");
	shell_obscure_set(sh, true);
	shell_prompt_change(sh, "login: ");
	shell_print(sh, "\n");
	return 0;
}

static int set_bypass(const struct shell *sh, shell_bypass_cb_t bypass)
{
	static bool in_use;

	if (bypass && in_use) {
		shell_error(sh, "Sample supports setting bypass on single instance.");

		return -EBUSY;
	}

	in_use = !in_use;
	if (in_use) {
		shell_print(sh, "Bypass started, press ctrl-x ctrl-q to escape");
		in_use = true;
	}

	shell_set_bypass(sh, bypass);

	return 0;
}

#define CHAR_1 0x18
#define CHAR_2 0x11

static void bypass_cb(const struct shell *sh, uint8_t *data, size_t len)
{
	static uint8_t tail;
	bool escape = false;

	/* Check if escape criteria is met. */
	if (tail == CHAR_1 && data[0] == CHAR_2) {
		escape = true;
	} else {
		for (int i = 0; i < (len - 1); i++) {
			if (data[i] == CHAR_1 && data[i + 1] == CHAR_2) {
				escape = true;
				break;
			}
		}
	}

	if (escape) {
		shell_print(sh, "Exit bypass");
		set_bypass(sh, NULL);
		tail = 0;
		return;
	}

	/* Store last byte for escape sequence detection */
	tail = data[len - 1];

	/* Do the data processing. */
	for (int i = 0; i < len; i++) {
		shell_fprintf(sh, SHELL_INFO, "%02x ", data[i]);
	}
	shell_fprintf(sh, SHELL_INFO, "| ");

	for (int i = 0; i < len; i++) {
		shell_fprintf(sh, SHELL_INFO, "%c", data[i]);
	}
	shell_fprintf(sh, SHELL_INFO, "\n");

}

static int cmd_bypass(const struct shell *sh, size_t argc, char **argv)
{
	return set_bypass(sh, bypass_cb);
}

static int cmd_dict(const struct shell *sh, size_t argc, char **argv,
		    void *data)
{
	int val = (intptr_t)data;

	shell_print(sh, "(syntax, value) : (%s, %d)", argv[0], val);

	return 0;
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_dict_cmds, cmd_dict,
	(value_0, 0, "value 0"), (value_1, 1, "value 1"),
	(value_2, 2, "value 2"), (value_3, 3, "value 3")
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_demo,
	SHELL_CMD(dictionary, &sub_dict_cmds, "Dictionary commands", NULL),
	SHELL_CMD(hexdump, NULL, "Hexdump params command.", cmd_demo_hexdump),
	SHELL_CMD(params, NULL, "Print params command.", cmd_demo_params),
	SHELL_CMD(ping, NULL, "Ping command.", cmd_demo_ping),
	SHELL_CMD(board, NULL, "Show board name command.", cmd_demo_board),
#if defined CONFIG_SHELL_GETOPT
	SHELL_CMD(getopt_thread_safe, NULL,
		  "Cammand using getopt in thread safe way"
		  " looking for: \"abhc:\".",
		  cmd_demo_getopt_ts),
	SHELL_CMD(getopt, NULL, "Cammand using getopt in non thread safe way"
		  " looking for: \"abhc:\".\n", cmd_demo_getopt),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(demo, &sub_demo, "Demo commands", NULL);

SHELL_CMD_ARG_REGISTER(version, NULL, "Show kernel version", cmd_version, 1, 0);

SHELL_CMD_ARG_REGISTER(bypass, NULL, "Bypass shell", cmd_bypass, 1, 0);

SHELL_COND_CMD_ARG_REGISTER(CONFIG_SHELL_START_OBSCURED, login, NULL,
			    "<password>", cmd_login, 2, 0);

SHELL_COND_CMD_REGISTER(CONFIG_SHELL_START_OBSCURED, logout, NULL,
			"Log out.", cmd_logout);


/* Create a set of commands. Commands to this set are added using @ref SHELL_SUBCMD_ADD
 * and @ref SHELL_SUBCMD_COND_ADD.
 */
SHELL_SUBCMD_SET_CREATE(sub_section_cmd, (section_cmd));

static int cmd1_handler(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "cmd1 executed");

	return 0;
}

/* Create a set of subcommands for "section_cmd cm1". */
SHELL_SUBCMD_SET_CREATE(sub_section_cmd1, (section_cmd, cmd1));

/* Add command to the set. Subcommand set is identify by parent shell command. */
SHELL_SUBCMD_ADD((section_cmd), cmd1, &sub_section_cmd1, "help for cmd1", cmd1_handler, 1, 0);

SHELL_CMD_REGISTER(section_cmd, &sub_section_cmd,
		   "Demo command using section for subcommand registration", NULL);

int main(void)
{
	if (IS_ENABLED(CONFIG_SHELL_START_OBSCURED)) {
		login_init();
	}

#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	const struct device *dev;
	uint32_t dtr = 0;

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (!device_is_ready(dev) || usb_enable(NULL)) {
		return 0;
	}

	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
#endif
	return 0;
}
