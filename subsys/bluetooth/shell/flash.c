/** @file
 * @brief Bluetooth Controller and flash co-operation
 *
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <shell/shell.h>
#include <misc/printk.h>

#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include <soc.h>

#define FLASH_SHELL_MODULE "flash"
#define BUF_ARRAY_CNT 16
#define TEST_ARR_SIZE 0x1000

static u8_t test_arr[TEST_ARR_SIZE];

static int cmd_erase(int argc, char *argv[])
{
	struct device *flash_dev;
	u32_t page_addr;
	int result;
	u32_t size;

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);
	if (!flash_dev) {
		printk("Nordic nRF5 flash driver was not found!\n");
		return -ENODEV;
	}

	if (argc < 2) {
		printk("Missing page address.\n");
		return -EINVAL;
	}

	page_addr = strtoul(argv[1], NULL, 16);

	if (argc > 2) {
		size = strtoul(argv[2], NULL, 16);
	} else {
		size = NRF_FICR->CODEPAGESIZE;
	}

	flash_write_protection_set(flash_dev, 0);

	result = flash_erase(flash_dev, page_addr, size);

	if (result) {
		printk("Erase Failed, code %d.\n", result);
	} else {
		printk("Erase success.\n");
	}

	return result;
}

static int cmd_flash(int argc, char *argv[])
{
	u32_t check_array[BUF_ARRAY_CNT];
	u32_t buf_array[BUF_ARRAY_CNT];
	struct device *flash_dev;
	u32_t w_addr;
	int j = 0;

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);
	if (!flash_dev) {
		printk("Nordic nRF5 flash driver was not found!\n");
		return -ENODEV;
	}

	if (argc < 2) {
		printk("Missing address.\n");
		return -EINVAL;
	}

	if (argc <= 2) {
		printk("Type data to be written.\n");
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
		printk("Write internal ERROR!\n");
		return -EIO;
	}

	printk("Write OK.\n");

	flash_read(flash_dev, w_addr, check_array, sizeof(buf_array[0]) * j);

	if (memcmp(buf_array, check_array, sizeof(buf_array[0]) * j) == 0) {
		printk("Verified.\n");
	} else {
		printk("Verification ERROR!\n");
		return -EIO;
	}

	return 0;
}

static int cmd_read(int argc, char *argv[])
{
	struct device *flash_dev;
	u32_t addr;
	int cnt;

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);
	if (!flash_dev) {
		printk("Nordic nRF5 flash driver was not found!\n");
		return -ENODEV;
	}

	if (argc < 2) {
		printk("Missing address.\n");
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
		printk("0x%08x ", data);
		addr += sizeof(data);
	}

	printk("\n");

	return 0;
}

static int cmd_test(int argc, char *argv[])
{
	struct device *flash_dev;
	u32_t repeat;
	int result;
	u32_t addr;
	u32_t size;

	flash_dev = device_get_binding(CONFIG_SOC_FLASH_NRF5_DEV_NAME);
	if (!flash_dev) {
		printk("Nordic nRF5 flash driver was not found!\n");
		return -ENODEV;
	}

	if (argc != 4) {
		printk("3 parameters reqired.\n");
		return -EINVAL;
	}

	addr = strtoul(argv[1], NULL, 16);
	size = strtoul(argv[2], NULL, 16);
	repeat = strtoul(argv[3], NULL, 16);

	if (size > TEST_ARR_SIZE) {
		printk("<size> must be at most 0x%x.", TEST_ARR_SIZE);
		return -EINVAL;
	}

	flash_write_protection_set(flash_dev, 0);

	for (u32_t i = 0; i < size; i++) {
		test_arr[i] = (u8_t)i;
	}

	while (repeat--) {
		result = flash_erase(flash_dev, addr, size);
		if (result) {
			printk("Erase Failed, code %d.\n", result);
			return -EIO;
		}

		printk("Erase OK.\n");

		if (flash_write(flash_dev, addr, test_arr, size) != 0) {
			printk("Write internal ERROR!\n");
			return -EIO;
		}

		printk("Write OK.\n");
	}

	printk("Erase-Write test done.\n");

	return 0;
}

static const struct shell_cmd flash_commands[] = {
	{ "flash-write", cmd_flash, "<address> <Dword> <Dword>..."},
	{ "flash-erase", cmd_erase, "<page address> <size>"},
	{ "flash-read", cmd_read, "<address> <Dword count>"},
	{ "flash-test", cmd_test, "<address> <size> <repeat count>"},
	{ NULL, NULL, NULL}
};

SHELL_REGISTER(FLASH_SHELL_MODULE, flash_commands);
