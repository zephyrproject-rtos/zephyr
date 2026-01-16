/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/version.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/misc/esp_tool/esp_tool.h>
#include <ctype.h>

#ifdef CONFIG_NATIVE_LIBC
#include <unistd.h>
#else
#include <zephyr/posix/unistd.h>
#endif

LOG_MODULE_REGISTER(esp_shell);

#ifdef CONFIG_ESP_TOOL_FW_ARRAYS
extern const uint8_t app_bin[];
extern const uint32_t app_bin_size;
extern const uint8_t app_bin_md5[];
#endif

static const struct device *esp = DEVICE_DT_GET(DT_INST(0, espressif_esp_tool));

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

static int cmd_esp_connect(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1 && strcmp(argv[1], "stub") == 0) {
		if (esp_tool_connect_stub(esp, false)) {
			shell_print(sh,"Failed connecting stub");
			return -1;
		}
		shell_print(sh, "STUB connected at");

		return 0;
	}

	if (esp_tool_connect(esp, true)) {
		shell_print(sh, "Connection failed");
		return -1;
	}
	shell_print(sh, "ROM connected");

	return 0;
}

static int cmd_esp_reset(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (esp_tool_reset_target(esp)) {
		shell_print(sh, "Reset failed");
		return -1;
	}
	shell_print(sh, "ESP reset done");

	return 0;
}

static int cmd_esp_target_info(const struct shell *sh, size_t argc, char **argv)
{
	int id;
	uint32_t size, offset;
	char *name;

	if (esp_tool_get_target(esp, &id)) {
		shell_print(sh, "Failed to read ID");
		return -1;
	}
	if (esp_tool_get_target_name(esp, &name)) {
		shell_print(sh, "Failed to read name");
		return -1;
	}
	if (esp_tool_get_boot_offset(esp, &offset)) {
		shell_print(sh, "Failed to get boot offset");
		return -1;
	}
	if (esp_tool_flash_detect_size(esp, &size)) {
		shell_print(sh, "Failed to detect flash size");
		return -1;
	}

	if (argc > 1) {
		if (strcmp(argv[1], "chip") == 0) {
			shell_print(sh, "%s", name);
			return 0;
		}
		if (strcmp(argv[1], "flash") == 0) {
			shell_print(sh, "%d MB", size/1024/1024);
			return 0;
		}
		if (strcmp(argv[1], "boot") == 0) {
			shell_print(sh, "0x%x", offset);
			return 0;
		}
	}
	shell_print(sh, "%s, boot from 0x%x, flash size %d MB", name, offset,
		    size/1024/1024);

	return 0;
}

static int cmd_esp_resources(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 0;
}

#if defined CONFIG_SHELL_GETOPT
/* Thread save usage */
static int cmd_demo_getopt_ts(const struct shell *sh, size_t argc,
			      char **argv)
{
	struct sys_getopt_state *state;
	char *cvalue = NULL;
	int aflag = 0;
	int bflag = 0;
	int c;

	while ((c = sys_getopt(argc, argv, "abhc:")) != -1) {
		state = sys_getopt_state_get();
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

	while ((c = sys_getopt(argc, argv, "abhc:")) != -1) {
		switch (c) {
		case 'a':
			aflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		case 'c':
			cvalue = sys_getopt_optarg;
			break;
		case 'h':
			/* When getopt is active shell is not parsing
			 * command handler to print help message. It must
			 * be done explicitly.
			 */
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		case '?':
			if (sys_getopt_optopt == 'c') {
				shell_print(sh,
					"Option -%c requires an argument.",
					sys_getopt_optopt);
			} else if (isprint(sys_getopt_optopt) != 0) {
				shell_print(sh, "Unknown option `-%c'.",
					    sys_getopt_optopt);
			} else {
				shell_print(sh,
					"Unknown option character `\\x%x'.",
					sys_getopt_optopt);
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

static int cmd_esp_flash_read(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint8_t buf[128];

	if (esp_tool_flash_read(esp, 0x20000, buf, sizeof(buf))) {
		shell_print(sh, "Failed to read flash");
		return -1;
	}
	shell_hexdump(sh, buf, sizeof(buf));

	return 0;
}

/* local */
#include "hexdump.h"
static int cmd_esp_flash_app(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	static uint8_t payload[1024];
	const uint8_t *bin_ptr = app_bin;
	size_t size = app_bin_size;
	uint32_t offset;

	if (!esp_tool_is_connected(esp)) {
		shell_print(sh, "not connected");
		return -1;
	}

	esp_tool_get_boot_offset(esp, &offset);

	if (esp_tool_flash_start(esp, offset, size, sizeof(payload))) {
		shell_print(sh, "Failed to read flash");
		return -1;
	}

	shell_print(sh, "Start programming");

	size_t written = 0;

	while (size > 0) {
		size_t to_write = MIN(size, sizeof(payload));
		memcpy(payload, bin_ptr, to_write);

		if (esp_tool_flash_write(esp, payload, to_write)) {
			shell_print(sh, "Packet could not be written");
			return -1;
		}
hexdump("pay", payload, 32);
		size -= to_write;
		bin_ptr += to_write;
		written += to_write;

		int progress = (int)(((float)written / app_bin_size) * 100);
		shell_print(sh, "\rProgress: %d %%", progress);
	};

//	esp_tool_flash_finish(esp, true);

	return 0;
}

static int cmd_esp_flash_erase(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (esp_tool_flash_erase(esp)) {
		shell_print(sh, "Failed to read flash");
		return -1;
	}
	shell_print(sh, "Flash is erased");

	return 0;
}

static int cmd_esp_mac_read(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint8_t mac[8];
	esp_tool_mac_read(esp, mac);

	shell_print(sh, "MAC  %d:%d:%d:%d:%d:%d", mac[0], mac[1], mac[2], mac[3],
	     mac[4], mac[5]);

	return 0;
}

static int cmd_esp_reg_read(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint32_t reg;
	esp_tool_register_read(esp, 0x60002000, &reg);

	shell_print(sh, "Reg val:  0x%x", reg);

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

	shell_set_bypass(sh, bypass, NULL);

	return 0;
}

#define CHAR_1 0x18
#define CHAR_2 0x11

static void bypass_cb(const struct shell *sh, uint8_t *data, size_t len, void *user_data)
{
	static uint8_t tail;
	bool escape = false;

	ARG_UNUSED(user_data);

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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_esp,
	SHELL_CMD(hexdump, NULL, "Hexdump params command.", cmd_demo_hexdump),
	SHELL_CMD(connect, NULL, "Connect target.", cmd_esp_connect),
	SHELL_CMD(reset, NULL, "Reset target.", cmd_esp_reset),
	SHELL_CMD(info, NULL, "Reset target.", cmd_esp_target_info),
	SHELL_CMD(resources, NULL, "ESP flash size.", cmd_esp_resources),
	SHELL_CMD(flash_read, NULL, "ESP flash size.", cmd_esp_flash_read),
	SHELL_CMD(flash_erase, NULL, "ESP flash size.", cmd_esp_flash_erase),
	SHELL_CMD(flash_app, NULL, "ESP flash size.", cmd_esp_flash_app),
	SHELL_CMD(mac_read, NULL, "ESP flash size.", cmd_esp_mac_read),
	SHELL_CMD(reg_read, NULL, "ESP flash size.", cmd_esp_reg_read),
//	SHELL_CMD(mem_read, NULL, "ESP flash size.", cmd_esp_mem_read),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(esp, &sub_esp, "ESP(tool) commands", NULL);

SHELL_CMD_ARG_REGISTER(version, NULL, "Show kernel version", cmd_version, 1, 0);

SHELL_CMD_ARG_REGISTER(bypass, NULL, "Bypass shell", cmd_bypass, 1, 0);
