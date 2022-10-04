#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/usb/usb_dc.h>

#include "view.h"
#include "meas.h"
#include "model.h"

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   500

#if 0
static int led_wrapper(const struct shell *shell, size_t argc, char **argv, void* data) {
	enum led_t color = (enum led_t) data;

	set_led(color);
	return 0;
}

SHELL_SUBCMD_DICT_SET_CREATE(led_options, led_wrapper,
        (off, LED_OFF), (red, LED_RED), (green, LED_GREEN), (blue, LED_BLUE)
);

SHELL_CMD_REGISTER(led, &led_options, "Switch selected LED on or off", led_wrapper);
#endif

static int meas_wrapper(const struct shell *shell, size_t argc, char **argv) {
        int32_t vbus = 0;
        int32_t cbus = 0;
        int32_t vcc1 = 0;
        int32_t vcc2 = 0;
        int32_t ccon = 0;

	meas_vbus(&vbus);
        meas_cbus(&cbus);
        meas_vcc1(&vcc1);
        meas_vcc2(&vcc2);
        meas_ccon(&ccon);

        shell_print(shell, "vbus: %i, cbus: %i, vcc1: %i, vcc2: %i, ccon: %i", vbus, cbus, vcc1, vcc2, ccon);
	return 0;
}

SHELL_CMD_REGISTER(read, NULL, "Reads the current and voltage of the selected line", meas_wrapper);

static int cmd_meas_cc2(const struct shell *shell, size_t argc, char **argv) {
	int32_t out;

	if (argc == 2 && *argv[1] == 'c') {
		meas_ccon(&out);
		shell_print(shell, "current of cc2: %i", out);
	}
	else {
		meas_vcc2(&out);
		shell_print(shell, "voltage of cc2: %i", out);
	}

	return 0;
}

static int cmd_meas_vb(const struct shell *shell, size_t argc, char **argv) {
	int32_t out;

	if (argc == 2 && *argv[1] == 'c') {
		meas_cbus(&out);
		shell_print(shell, "current of vbus: %i", out);
	}
	else {
		meas_vbus(&out);
		shell_print(shell, "voltage of vbus: %i", out);
	}

	return 0;
}

static int cmd_meas_cc1(const struct shell *shell, size_t argc, char **argv) {
	int32_t out;

	meas_vcc1(&out);
	shell_print(shell, "voltage of cc1: %i\n", out);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_meas,
        SHELL_CMD(cc1, NULL, "Print cc1 voltage.", cmd_meas_cc1),
        SHELL_CMD(cc2, NULL, "Print cc2 voltage or current.", cmd_meas_cc2),
        SHELL_CMD(vb, NULL, "Print vbus voltage or current.", cmd_meas_vb),
        SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(meas, &sub_meas, "Reads current and voltage of all lines", NULL);

static int cmd_version(const struct shell *shell, size_t argc, char**argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Twinkie version 2.0.1");

	return 0;
}

SHELL_CMD_REGISTER(version, NULL, "Show kernel version", cmd_version);

static int cmd_reset(const struct shell *shell, size_t argc, char**argv) {
	reset_sniffer();
	usb_dc_reset();

	return 0;
}

SHELL_CMD_REGISTER(reset, NULL, "Resets the usb", cmd_reset);

static int cmd_snoop(const struct shell *shell, size_t argc, char**argv) {
	if (argc >= 2 && *argv[1] <= '3' && *argv[1] >= '0') {
		view_set_snoop((enum view_snoop_t) (*argv[1] - '0'));
//		set_readline(*argv[1] - '0');
	}

	return 0;
}

SHELL_CMD_REGISTER(snoop, NULL, "Sets the snoop CC line", cmd_snoop);

static int cmd_start(const struct shell *shell, size_t argc, char**argv)
{
	start_snooper(true);
	return 0;
}
SHELL_CMD_REGISTER(start, NULL, "Start snooper", cmd_start);

static int cmd_stop(const struct shell *shell, size_t argc, char**argv)
{
	start_snooper(false);
	return 0;
}
SHELL_CMD_REGISTER(stop, NULL, "Stop snooper", cmd_stop);



static int cmd_role_source(const struct shell *shell, size_t argc, char**argv) {
	uint8_t Rp = *argv[1] - '0';

	set_role(Rp * 2);

	return 0;
}

static int cmd_role_sink(const struct shell *shell, size_t argc, char**argv) {
	uint8_t Rp = *argv[1] - '0';

	set_role(Rp * 2 + 1);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_role,
        SHELL_CMD(source, NULL, "Sets role as source", cmd_role_source),
	SHELL_CMD(sink, NULL, "Sets role as sink", cmd_role_sink),
        SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(role, &sub_role, "Sets role as sink or source", NULL);

static int cmd_rpull(const struct shell *shell, size_t argc, char**argv, void* data) {
	uint8_t pull_mask = (uint32_t) data;

	select_pull_up(pull_mask);

	return 0;
}


SHELL_SUBCMD_DICT_SET_CREATE(rpull_options, cmd_rpull,
        (cc1_rd, 0), (cc1_ru, 2), (cc1_r1, 4), (cc1_r3, 6),
	(cc2_rd, 1), (cc2_ru, 3), (cc2_r1, 5), (cc2_r3, 7)

);

SHELL_CMD_REGISTER(rpull, &rpull_options, "Place pull resistor", cmd_rpull);
