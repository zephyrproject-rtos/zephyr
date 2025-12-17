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

static int cmd_esp_connect_rom(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (esp_tool_connect(esp, true)) {
		shell_print(sh, "Failed to connect ROM");
		return -1;
	}
	shell_print(sh, "ROM connected at %d bps",
		    esp_tool_get_current_baudrate(esp));

	return 0;
}

static int cmd_esp_connect_stub(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (esp_tool_connect_stub(esp, true)) {
		shell_print(sh,"Failed to connect STUB");
		return -1;
	}
	shell_print(sh, "STUB connected at %d",
		    esp_tool_get_current_baudrate(esp));

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_esp_connect,
	SHELL_CMD_ARG(rom, NULL, "Connect using ROM functions",
		      cmd_esp_connect_rom, 1, 0),
	SHELL_CMD_ARG(stub, NULL, "Connect using STUB functions",
		      cmd_esp_connect_stub, 1, 0),
	SHELL_SUBCMD_SET_END
);

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
//	if (esp_tool_flash_detect_size(esp, &size)) {
//		shell_print(sh, "Failed to detect flash size");
//		return -1;
//	}

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
	shell_print(sh, "Current baudrate %d", esp_tool_get_current_baudrate(esp));
	shell_print(sh, "%s, boot from 0x%x, flash size %d MB", name, offset,
		    size/1024/1024);

	return 0;
}

static int cmd_esp_flash_detect_size(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint32_t size;

	if (esp_tool_flash_detect_size(esp, &size)) {
		shell_print(sh, "Failed to detect flash size");
		return -1;
	}
	shell_print(sh, "%dMB", size/1024/1024);

	return 0;
}

static int cmd_esp_flash_read(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint32_t addr;
	uint32_t size;
	uint8_t buf[128];

	addr = strtoul((*argv)[1], NULL, 16);
	if (argc > 2) {
		size = strtoul(argv[2], NULL, 16);
	} else {
		size = 1;
	}

	uint32_t to_read;
	do {
		to_read = MIN(size, sizeof(buf));

		if (esp_tool_flash_read(esp, addr, buf, to_read)) {
			shell_print(sh, "Failed to read flash");
			return -1;
		}
		shell_hexdump(sh, buf, to_read);
		size -= to_read;
	} while (size);

	return 0;
}

static int cmd_esp_flash_app(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret;
	ret = esp_tool_flash_binary(esp, app_bin, app_bin_size, 0x0);
	printk("return code %d", ret);

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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_esp_flash,
	SHELL_CMD_ARG(read, NULL, "<address> [<Dword count>]",
		      cmd_esp_flash_read, 2, 2),
	SHELL_CMD_ARG(size, NULL, "Detect flash size",
		      cmd_esp_flash_detect_size, 1, 0),
	SHELL_CMD_ARG(default-fw, NULL, "Write default fw to flash",
		      cmd_esp_flash_app, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_esp_register_read(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "TBD: %s", __func__);
//	uint32_t reg;
//	esp_tool_register_read(esp, 0x60002000, &reg);
//	shell_print(sh, "Reg val:  0x%x", reg);

	return 0;
}

static int cmd_esp_register_write(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "TBD: %s", __func__);
//	uint32_t reg, value;
//	esp_tool_register_write(esp, 0x60002000, &reg);
//	shell_print(sh, "Reg val:  0x%x", reg);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_esp_register,
	SHELL_CMD_ARG(read, NULL, "Read flash from offset",
		      cmd_esp_register_read, 1, 0),
	SHELL_CMD_ARG(write, NULL, "Detect flash size",
		      cmd_esp_flash_detect_size, 1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_esp_baudrate(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (argc > 1) {
		if (strcmp(argv[1], "high") == 0) {
			shell_print(sh, "high speed");
			return 0;
		}
		if (strcmp(argv[1], "default") == 0) {
			shell_print(sh, "default speed");
			return 0;
		}
	}
	shell_print(sh, "Current baudrate %d", esp_tool_get_current_baudrate(esp));

	return 0;
}


static int cmd_esp_mac_read(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	uint8_t mac[10];
	esp_tool_mac_read(esp, mac);

	shell_print(sh, "MAC  %x:%x:%x:%x:%x:%x", mac[0], mac[1], mac[2], mac[3],
	     mac[4], mac[5]);

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

static int cmd_esp_resources(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 0;
}

static int cmd_demo_params(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "argc = %zd", argc);
	for (size_t cnt = 0; cnt < argc; cnt++) {
		shell_print(sh, "  argv[%zd] = %s", cnt, argv[cnt]);
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


SHELL_STATIC_SUBCMD_SET_CREATE(sub_esp,
	SHELL_CMD(connect, &sub_esp_connect, "Connect target", NULL),
	SHELL_CMD(reset, NULL, "Reset target", cmd_esp_reset),
	SHELL_CMD(info, NULL, "Reset target", cmd_esp_target_info),
	SHELL_CMD(mac, NULL, "Read target MAC", cmd_esp_mac_read),
	SHELL_CMD(resources, NULL, "ESP flash size", cmd_esp_resources),
	SHELL_CMD(flash, &sub_esp_flash, "ESP flash operations", NULL),
	SHELL_CMD(baudrate, NULL, "ESP baudrate setup", cmd_esp_baudrate),
	SHELL_CMD(register, &sub_esp_register, "ESP register operations", NULL),

//	SHELL_CMD(flash-read, NULL, "ESP flash size.", cmd_esp_flash_read),
//	SHELL_CMD(flash-erase, NULL, "ESP flash size.", cmd_esp_flash_erase),
	SHELL_CMD(flash-app, NULL, "ESP flash size.", cmd_esp_flash_app),
//	SHELL_CMD(reg_read, NULL, "ESP flash size.", cmd_esp_reg_read),

//	SHELL_CMD(register, &sub_register, "ESP flash size.", NULL),

	SHELL_CMD(params, NULL, "Print params command.", cmd_demo_params),

	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(esp, &sub_esp, "ESP(tool) commands", NULL);

/******************************************************************************/
SHELL_CMD_ARG_REGISTER(version, NULL, "Show kernel version", cmd_version, 1, 0);
SHELL_CMD_ARG_REGISTER(bypass, NULL, "Bypass shell", cmd_bypass, 1, 0);
