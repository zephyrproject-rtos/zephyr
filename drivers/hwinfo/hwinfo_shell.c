/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>

static int cmd_get_device_id(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t dev_id[16];
	ssize_t length;
	int i;

	length = hwinfo_get_device_id(dev_id, sizeof(dev_id));

	if (length == -ENOTSUP) {
		shell_error(sh, "Not supported by hardware");
		return -ENOTSUP;
	} else if (length < 0) {
		shell_error(sh, "Error: %zd", length);
		return length;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Length: %zd\n", length);
	shell_fprintf(sh, SHELL_NORMAL, "ID: 0x");

	for (i = 0 ; i < length ; i++) {
		shell_fprintf(sh, SHELL_NORMAL, "%02x", dev_id[i]);
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");

	return 0;
}

static inline const char *cause_to_string(uint32_t cause)
{
	switch (cause) {
	case RESET_PIN:
		return "pin";

	case RESET_SOFTWARE:
		return "software";

	case RESET_BROWNOUT:
		return "brownout";

	case RESET_POR:
		return "power-on reset";

	case RESET_WATCHDOG:
		return "watchdog";

	case RESET_DEBUG:
		return "debug";

	case RESET_SECURITY:
		return "security";

	case RESET_LOW_POWER_WAKE:
		return "low power wake-up";

	case RESET_CPU_LOCKUP:
		return "CPU lockup";

	case RESET_PARITY:
		return "parity error";

	case RESET_PLL:
		return "PLL error";

	case RESET_CLOCK:
		return "clock";

	case RESET_HARDWARE:
		return "hardware";

	case RESET_USER:
		return "user";

	case RESET_TEMPERATURE:
		return "temperature";

	default:
		return "unknown";
	}
}

static void print_all_reset_causes(const struct shell *sh, uint32_t cause)
{
	for (uint32_t cause_mask = 1; cause_mask; cause_mask <<= 1) {
		if (cause & cause_mask) {
			shell_print(sh, "- %s",
				    cause_to_string(cause & cause_mask));
		}
	}
}

static int cmd_show_reset_cause(const struct shell *sh, size_t argc,
				char **argv)
{
	int res;
	uint32_t cause;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	res = hwinfo_get_reset_cause(&cause);
	if (res == -ENOTSUP) {
		shell_error(sh, "Not supported by hardware");
		return res;
	} else if (res != 0) {
		shell_error(sh, "Error reading the cause [%d]", res);
		return res;
	}

	if (cause != 0) {
		shell_print(sh, "reset caused by:");
		print_all_reset_causes(sh, cause);
	} else {
		shell_print(sh, "No reset cause set");
	}

	return 0;
}

static int cmd_clear_reset_cause(const struct shell *sh, size_t argc,
				 char **argv)
{
	int res;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	res = hwinfo_clear_reset_cause();
	if (res == -ENOTSUP) {
		shell_error(sh, "Not supported by hardware");
	} else if (res != 0) {
		shell_error(sh, "Error clearing the reset causes [%d]", res);
		return res;
	}

	return 0;
}

static int cmd_supported_reset_cause(const struct shell *sh, size_t argc,
				     char **argv)
{
	uint32_t cause;
	int res;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	res = hwinfo_get_supported_reset_cause(&cause);
	if (res == -ENOTSUP) {
		shell_error(sh, "Not supported by hardware");
	} else if (res != 0) {
		shell_error(sh, "Could not get the supported reset causes [%d]", res);
		return res;
	}

	if (cause != 0) {
		shell_print(sh, "supported reset causes:");
		print_all_reset_causes(sh, cause);
	} else {
		shell_print(sh, "No causes supported");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_reset_cause,
	SHELL_CMD_ARG(show, NULL, "Show persistent reset causes",
		      cmd_show_reset_cause, 1, 0),
	SHELL_CMD_ARG(clear, NULL, "Clear all persistent reset causes",
		      cmd_clear_reset_cause, 1, 0),
	SHELL_CMD_ARG(supported, NULL,
		      "Get a list of all supported reset causes",
		      cmd_supported_reset_cause, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_hwinfo,
	SHELL_CMD_ARG(devid, NULL, "Show device id", cmd_get_device_id, 1, 0),
	SHELL_CMD_ARG(reset_cause, &sub_reset_cause, "Reset cause commands",
		      cmd_show_reset_cause, 1, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_ARG_REGISTER(hwinfo, &sub_hwinfo, "HWINFO commands", NULL, 2, 0);
