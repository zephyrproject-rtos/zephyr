/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <shell/shell.h>

#include <stdlib.h>
#include <string.h>
#include <drivers/flash.h>
#include <soc.h>

#define FLASH_SHELL_MODULE "flash"
#define BUF_ARRAY_CNT 16
#define TEST_ARR_SIZE 0x1000

static u8_t test_arr[TEST_ARR_SIZE];

static int cmd_erase(const struct shell *shell, size_t argc, char *argv[])
{
	struct device *flash_dev;
	u32_t page_addr;
	int result;
	u32_t size;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (!flash_dev) {
		shell_error(shell, "Flash driver was not found!");
		return -ENODEV;
	}

	if (argc < 2) {
		shell_error(shell, "Missing page address.");
		return -EINVAL;
	}

	page_addr = strtoul(argv[1], NULL, 16);

	if (argc > 2) {
		size = strtoul(argv[2], NULL, 16);
	} else {
		struct flash_pages_info info;

		result = flash_get_page_info_by_offs(flash_dev, page_addr,
						     &info);

		if (result != 0) {
			shell_error(shell, "Could not determine page size, "
				    "code %d.", result);
			return -EINVAL;
		}

		size = info.size;
	}

	flash_write_protection_set(flash_dev, 0);

	result = flash_erase(flash_dev, page_addr, size);

	if (result) {
		shell_error(shell, "Erase Failed, code %d.", result);
	} else {
		shell_print(shell, "Erase success.");
	}

	return result;
}

static int cmd_write(const struct shell *shell, size_t argc, char *argv[])
{
	u32_t check_array[BUF_ARRAY_CNT];
	u32_t buf_array[BUF_ARRAY_CNT];
	struct device *flash_dev;
	u32_t w_addr;
	int j = 0;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (!flash_dev) {
		shell_error(shell, "Flash driver was not found!");
		return -ENODEV;
	}

	if (argc < 2) {
		shell_error(shell, "Missing address.");
		return -EINVAL;
	}

	if (argc <= 2) {
		shell_error(shell, "Type data to be written.");
		return -EINVAL;
	}

	for (int i = 2; i < argc && i < BUF_ARRAY_CNT; i++) {
		buf_array[j] = strtoul(argv[i], NULL, 16);
		check_array[j] = ~buf_array[j];
		j++;
	}

	flash_write_protection_set(flash_dev, 0);

	w_addr = strtoul(argv[1], NULL, 16);

	if (flash_write(flash_dev, w_addr, buf_array,
			sizeof(buf_array[0]) * j) != 0) {
		shell_error(shell, "Write internal ERROR!");
		return -EIO;
	}

	shell_print(shell, "Write OK.");

	flash_read(flash_dev, w_addr, check_array, sizeof(buf_array[0]) * j);

	if (memcmp(buf_array, check_array, sizeof(buf_array[0]) * j) == 0) {
		shell_print(shell, "Verified.");
	} else {
		shell_error(shell, "Verification ERROR!");
		return -EIO;
	}

	return 0;
}

static int cmd_read(const struct shell *shell, size_t argc, char *argv[])
{
	struct device *flash_dev;
	u32_t addr;
	int cnt;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (!flash_dev) {
		shell_error(shell, "Flash driver was not found!");
		return -ENODEV;
	}

	if (argc < 2) {
		shell_error(shell, "Missing address.");
		return -EINVAL;
	}

	addr = strtoul(argv[1], NULL, 16);

	if (argc > 2) {
		cnt = strtoul(argv[2], NULL, 16);
	} else {
		cnt = 1;
	}

	while (cnt--) {
		u32_t data;

		flash_read(flash_dev, addr, &data, sizeof(data));
		shell_print(shell, "0x%08x ", data);
		addr += sizeof(data);
	}

	shell_print(shell, "");

	return 0;
}

static int cmd_test(const struct shell *shell, size_t argc, char *argv[])
{
	struct device *flash_dev;
	u32_t repeat;
	int result;
	u32_t addr;
	u32_t size;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (!flash_dev) {
		shell_error(shell, "Flash driver was not found!");
		return -ENODEV;
	}

	if (argc != 4) {
		shell_error(shell, "3 parameters reqired.");
		return -EINVAL;
	}

	addr = strtoul(argv[1], NULL, 16);
	size = strtoul(argv[2], NULL, 16);
	repeat = strtoul(argv[3], NULL, 16);

	if (size > TEST_ARR_SIZE) {
		shell_error(shell, "<size> must be at most 0x%x.",
			    TEST_ARR_SIZE);
		return -EINVAL;
	}

	flash_write_protection_set(flash_dev, 0);

	for (u32_t i = 0; i < size; i++) {
		test_arr[i] = (u8_t)i;
	}

	while (repeat--) {
		result = flash_erase(flash_dev, addr, size);
		if (result) {
			shell_error(shell, "Erase Failed, code %d.", result);
			return -EIO;
		}

		shell_print(shell, "Erase OK.");

		if (flash_write(flash_dev, addr, test_arr, size) != 0) {
			shell_error(shell, "Write internal ERROR!");
			return -EIO;
		}

		shell_print(shell, "Write OK.");
	}

	shell_print(shell, "Erase-Write test done.");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(flash_cmds,
	SHELL_CMD(erase, NULL, "<page address> <size>", cmd_erase),
	SHELL_CMD(read, NULL, "<address> <Dword count>", cmd_read),
	SHELL_CMD(test, NULL, "<address> <size> <repeat count>", cmd_test),
	SHELL_CMD(write, NULL, "<address> <dword> <dword>...", cmd_write),
	SHELL_SUBCMD_SET_END
);

static int cmd_flash(const struct shell *shell, size_t argc, char **argv)
{
	shell_error(shell, "%s:unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(flash, &flash_cmds, "Flash shell commands",
		       cmd_flash, 2, 0);
