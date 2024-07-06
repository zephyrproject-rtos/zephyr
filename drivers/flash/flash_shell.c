/*
 * Copyright (c) 2017-2023 Nordic Semiconductor ASA
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <stdlib.h>
#include <string.h>
#include <zephyr/drivers/flash.h>

/* Buffer is only needed for bytes that follow command and offset */
#define BUF_ARRAY_CNT (CONFIG_SHELL_ARGC_MAX - 2)

/* This only issues compilation error when it would not be possible
 * to extract at least one byte from command line arguments, yet
 * it does not warrant successful writes if BUF_ARRAY_CNT
 * is smaller than flash write alignment.
 */
BUILD_ASSERT(BUF_ARRAY_CNT >= 1);

static const struct device *const zephyr_flash_controller =
	DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));

static uint8_t __aligned(4) test_arr[CONFIG_FLASH_SHELL_BUFFER_SIZE];

static int parse_helper(const struct shell *sh, size_t *argc,
		char **argv[], const struct device * *flash_dev,
		uint32_t *addr)
{
	char *endptr;

	*addr = strtoul((*argv)[1], &endptr, 16);

	if (*endptr != '\0') {
		/* flash controller from user input */
		*flash_dev = device_get_binding((*argv)[1]);
		if (!*flash_dev) {
			shell_error(sh, "Given flash device was not found");
			return -ENODEV;
		}
	} else if (zephyr_flash_controller != NULL) {
		/* default to zephyr,flash-controller */
		if (!device_is_ready(zephyr_flash_controller)) {
			shell_error(sh, "Default flash driver not ready");
			return -ENODEV;
		}
		*flash_dev = zephyr_flash_controller;
	} else {
		/* no flash controller given, no default available */
		shell_error(sh, "No flash device specified (required)");
		return -ENODEV;
	}

	if (*endptr == '\0') {
		return 0;
	}
	if (*argc < 3) {
		shell_error(sh, "Missing address.");
		return -EINVAL;
	}
	*addr = strtoul((*argv)[2], &endptr, 16);
	(*argc)--;
	(*argv)++;
	return 0;
}

static int cmd_erase(const struct shell *sh, size_t argc, char *argv[])
{
	int result = -ENOTSUP;

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
	const struct device *flash_dev;
	uint32_t page_addr;
	uint32_t size;

	result = parse_helper(sh, &argc, &argv, &flash_dev, &page_addr);
	if (result) {
		return result;
	}
	if (argc > 2) {
		size = strtoul(argv[2], NULL, 16);
	} else {
		struct flash_pages_info info;

		result = flash_get_page_info_by_offs(flash_dev, page_addr,
						     &info);

		if (result != 0) {
			shell_error(sh, "Could not determine page size, "
				    "code %d.", result);
			return -EINVAL;
		}

		size = info.size;
	}

	result = flash_erase(flash_dev, page_addr, size);

	if (result) {
		shell_error(sh, "Erase Failed, code %d.", result);
	} else {
		shell_print(sh, "Erase success.");
	}
#endif

	return result;
}

static int cmd_write(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t __aligned(4) check_array[BUF_ARRAY_CNT];
	uint32_t __aligned(4) buf_array[BUF_ARRAY_CNT];
	const struct device *flash_dev;
	uint32_t w_addr;
	int ret;
	size_t op_size;

	ret = parse_helper(sh, &argc, &argv, &flash_dev, &w_addr);
	if (ret) {
		return ret;
	}

	if (argc <= 2) {
		shell_error(sh, "Missing data to be written.");
		return -EINVAL;
	}

	op_size = 0;

	for (int i = 2; i < argc; i++) {
		int j = i - 2;

		buf_array[j] = strtoul(argv[i], NULL, 16);
		check_array[j] = ~buf_array[j];

		op_size += sizeof(buf_array[0]);
	}

	if (flash_write(flash_dev, w_addr, buf_array, op_size) != 0) {
		shell_error(sh, "Write internal ERROR!");
		return -EIO;
	}

	shell_print(sh, "Write OK.");

	if (flash_read(flash_dev, w_addr, check_array, op_size) < 0) {
		shell_print(sh, "Verification read ERROR!");
		return -EIO;
	}

	if (memcmp(buf_array, check_array, op_size) == 0) {
		shell_print(sh, "Verified.");
	} else {
		shell_error(sh, "Verification ERROR!");
		return -EIO;
	}

	return 0;
}

static int cmd_read(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	uint32_t addr;
	int todo;
	int upto;
	int cnt;
	int ret;

	ret = parse_helper(sh, &argc, &argv, &flash_dev, &addr);
	if (ret) {
		return ret;
	}

	if (argc > 2) {
		cnt = strtoul(argv[2], NULL, 16);
	} else {
		cnt = 1;
	}

	for (upto = 0; upto < cnt; upto += todo) {
		uint8_t data[SHELL_HEXDUMP_BYTES_IN_LINE];

		todo = MIN(cnt - upto, SHELL_HEXDUMP_BYTES_IN_LINE);
		ret = flash_read(flash_dev, addr, data, todo);
		if (ret != 0) {
			shell_error(sh, "Read ERROR!");
			return -EIO;
		}
		shell_hexdump_line(sh, addr, data, todo);
		addr += todo;
	}

	shell_print(sh, "");

	return 0;
}

static int cmd_test(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	uint32_t repeat;
	int result;
	uint32_t addr;
	uint32_t size;

	static uint8_t __aligned(4) check_arr[CONFIG_FLASH_SHELL_BUFFER_SIZE];

	result = parse_helper(sh, &argc, &argv, &flash_dev, &addr);
	if (result) {
		return result;
	}

	size = strtoul(argv[2], NULL, 16);
	repeat = strtoul(argv[3], NULL, 16);
	if (size > CONFIG_FLASH_SHELL_BUFFER_SIZE) {
		shell_error(sh, "<size> must be at most 0x%x.",
			    CONFIG_FLASH_SHELL_BUFFER_SIZE);
		return -EINVAL;
	}

	if (repeat == 0) {
		repeat = 1;
	}

	for (uint32_t i = 0; i < size; i++) {
		test_arr[i] = (uint8_t)i;
	}

	result = 0;

	while (repeat--) {
		result = flash_erase(flash_dev, addr, size);

		if (result) {
			shell_error(sh, "Erase Failed, code %d.", result);
			break;
		}

		shell_print(sh, "Erase OK.");

		result = flash_write(flash_dev, addr, test_arr, size);

		if (result) {
			shell_error(sh, "Write failed, code %d", result);
			break;
		}

		shell_print(sh, "Write OK.");

		result = flash_read(flash_dev, addr, check_arr, size);

		if (result < 0) {
			shell_print(sh, "Verification read failed, code: %d", result);
			break;
		}

		if (memcmp(test_arr, check_arr, size) != 0) {
			shell_error(sh, "Verification ERROR!");
			break;
		}

		shell_print(sh, "Verified OK.");
	}

	if (result == 0) {
		shell_print(sh, "Erase-Write-Verify test done.");
	}

	return result;
}

#ifdef CONFIG_FLASH_SHELL_TEST_COMMANDS
const static uint8_t speed_types[][4] = { "B", "KiB", "MiB", "GiB" };
const static uint32_t speed_divisor = 1024;

static int read_write_erase_validate(const struct shell *sh, size_t argc, char *argv[],
				     uint32_t *size, uint32_t *repeat)
{
	if (argc < 4) {
		shell_error(sh, "Missing parameters: <device> <offset> <size> <repeat>");
		return -EINVAL;
	}

	*size = strtoul(argv[2], NULL, 0);
	*repeat = strtoul(argv[3], NULL, 0);

	if (*size == 0 || *size > CONFIG_FLASH_SHELL_BUFFER_SIZE) {
		shell_error(sh, "<size> must be between 0x1 and 0x%x.",
			    CONFIG_FLASH_SHELL_BUFFER_SIZE);
		return -EINVAL;
	}

	if (*repeat == 0 || *repeat > 10) {
		shell_error(sh, "<repeat> must be between 1 and 10.");
		return -EINVAL;
	}

	return 0;
}

static void speed_output(const struct shell *sh, uint64_t total_time, double loops, double size)
{
	double time_per_loop = (double)total_time / loops;
	double throughput = size;
	uint8_t speed_index = 0;

	if (time_per_loop > 0) {
		throughput /= (time_per_loop / 1000.0);
	}

	while (throughput >= (double)speed_divisor && speed_index < ARRAY_SIZE(speed_types)) {
		throughput /= (double)speed_divisor;
		++speed_index;
	}

	shell_print(sh, "Total: %llums, Per loop: ~%.0fms, Speed: ~%.1f%sps",
		    total_time, time_per_loop, throughput, speed_types[speed_index]);
}

static int cmd_read_test(const struct shell *sh, size_t argc, char *argv[])
{

	const struct device *flash_dev;
	uint32_t repeat;
	int result;
	uint32_t addr;
	uint32_t size;
	uint64_t start_time;
	uint64_t loop_time;
	uint64_t total_time = 0;
	uint32_t loops = 0;

	result = parse_helper(sh, &argc, &argv, &flash_dev, &addr);
	if (result) {
		return result;
	}

	result = read_write_erase_validate(sh, argc, argv, &size, &repeat);
	if (result) {
		return result;
	}

	while (repeat--) {
		start_time = k_uptime_get();
		result = flash_read(flash_dev, addr, test_arr, size);
		loop_time = k_uptime_delta(&start_time);

		if (result) {
			shell_error(sh, "Read failed: %d", result);
			break;
		}

		++loops;
		total_time += loop_time;
		shell_print(sh, "Loop #%u done in %llums.", loops, loop_time);
	}

	if (result == 0) {
		speed_output(sh, total_time, (double)loops, (double)size);
	}

	return result;
}

static int cmd_write_test(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	uint32_t repeat;
	int result;
	uint32_t addr;
	uint32_t size;
	uint64_t start_time;
	uint64_t loop_time;
	uint64_t total_time = 0;
	uint32_t loops = 0;

	result = parse_helper(sh, &argc, &argv, &flash_dev, &addr);
	if (result) {
		return result;
	}

	result = read_write_erase_validate(sh, argc, argv, &size, &repeat);
	if (result) {
		return result;
	}

	for (uint32_t i = 0; i < size; i++) {
		test_arr[i] = (uint8_t)i;
	}

	while (repeat--) {
		start_time = k_uptime_get();
		result = flash_write(flash_dev, addr, test_arr, size);
		loop_time = k_uptime_delta(&start_time);

		if (result) {
			shell_error(sh, "Write failed: %d", result);
			break;
		}

		++loops;
		total_time += loop_time;
		shell_print(sh, "Loop #%u done in %llu ticks.", loops, loop_time);
	}

	if (result == 0) {
		speed_output(sh, total_time, (double)loops, (double)size);
	}

	return result;
}

static int cmd_erase_test(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	uint32_t repeat;
	int result;
	uint32_t addr;
	uint32_t size;
	uint64_t start_time;
	uint64_t loop_time;
	uint64_t total_time = 0;
	uint32_t loops = 0;

	result = parse_helper(sh, &argc, &argv, &flash_dev, &addr);
	if (result) {
		return result;
	}

	result = read_write_erase_validate(sh, argc, argv, &size, &repeat);
	if (result) {
		return result;
	}

	for (uint32_t i = 0; i < size; i++) {
		test_arr[i] = (uint8_t)i;
	}

	while (repeat--) {
		start_time = k_uptime_get();
		result = flash_erase(flash_dev, addr, size);
		loop_time = k_uptime_delta(&start_time);

		if (result) {
			shell_error(sh, "Erase failed: %d", result);
			break;
		}

		++loops;
		total_time += loop_time;
		shell_print(sh, "Loop #%u done in %llums.", loops, loop_time);
	}

	if (result == 0) {
		speed_output(sh, total_time, (double)loops, (double)size);
	}

	return result;
}

static int cmd_erase_write_test(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	uint32_t repeat;
	int result_erase = 0;
	int result_write = 0;
	uint32_t addr;
	uint32_t size;
	uint64_t start_time;
	uint64_t loop_time;
	uint64_t total_time = 0;
	uint32_t loops = 0;

	result_erase = parse_helper(sh, &argc, &argv, &flash_dev, &addr);
	if (result_erase) {
		return result_erase;
	}

	result_erase = read_write_erase_validate(sh, argc, argv, &size, &repeat);
	if (result_erase) {
		return result_erase;
	}

	for (uint32_t i = 0; i < size; i++) {
		test_arr[i] = (uint8_t)i;
	}

	while (repeat--) {
		start_time = k_uptime_get();
		result_erase = flash_erase(flash_dev, addr, size);
		result_write = flash_write(flash_dev, addr, test_arr, size);
		loop_time = k_uptime_delta(&start_time);

		if (result_erase) {
			shell_error(sh, "Erase failed: %d", result_erase);
			break;
		}

		if (result_write) {
			shell_error(sh, "Write failed: %d", result_write);
			break;
		}

		++loops;
		total_time += loop_time;
		shell_print(sh, "Loop #%u done in %llums.", loops, loop_time);
	}

	if (result_erase == 0 && result_write == 0) {
		speed_output(sh, total_time, (double)loops, (double)size);
	}

	return (result_erase != 0 ? result_erase : result_write);
}
#endif

static int set_bypass(const struct shell *sh, shell_bypass_cb_t bypass)
{
	static bool in_use;

	if (bypass && in_use) {
		shell_error(sh, "flash load supports setting bypass on a single instance.");

		return -EBUSY;
	}

	/* Mark that we have set or unset the bypass function */
	in_use = bypass != NULL;

	if (in_use) {
		shell_print(sh, "Loading...");
	}

	shell_set_bypass(sh, bypass);

	return 0;
}

#define FLASH_LOAD_BUF_MAX 256

static const struct device *flash_load_dev;
static uint32_t flash_load_buf_size;
static uint32_t flash_load_addr;
static uint32_t flash_load_total;
static uint32_t flash_load_written;
static uint32_t flash_load_chunk;

static uint32_t flash_load_boff;
static uint8_t flash_load_buf[FLASH_LOAD_BUF_MAX];

static void bypass_cb(const struct shell *sh, uint8_t *recv, size_t len)
{
	uint32_t left_to_read = flash_load_total - flash_load_written - flash_load_boff;
	uint32_t to_copy = MIN(len, left_to_read);
	uint32_t copied = 0;

	while (copied < to_copy) {

		uint32_t buf_copy = MIN(to_copy, flash_load_buf_size - flash_load_boff);

		memcpy(flash_load_buf + flash_load_boff, recv + copied, buf_copy);

		flash_load_boff += buf_copy;
		copied += buf_copy;

		/* Buffer is full. Write data to memory. */
		if (flash_load_boff == flash_load_buf_size) {
			uint32_t addr = flash_load_addr + flash_load_written;
			int rc = flash_write(flash_load_dev, addr, flash_load_buf,
					flash_load_buf_size);

			if (rc != 0) {
				shell_error(sh, "Write to addr %x on dev %p ERROR!",
						addr, flash_load_dev);
			}

			shell_print(sh, "Written chunk %d", flash_load_chunk);

			flash_load_written += flash_load_buf_size;
			flash_load_chunk++;
			flash_load_boff = 0;
		}
	}

	/* When data is not aligned to flash_load_buf_size there may be partial write
	 * at the end.
	 */
	if (flash_load_written < flash_load_total &&
			flash_load_written + flash_load_boff >= flash_load_total) {

		uint32_t addr = flash_load_addr + flash_load_written;
		int rc = flash_write(flash_load_dev, addr, flash_load_buf, flash_load_boff);

		if (rc != 0) {
			set_bypass(sh, NULL);
			shell_error(sh, "Write to addr %x on dev %p ERROR!",
					addr, flash_load_dev);
			return;
		}

		shell_print(sh, "Written chunk %d", flash_load_chunk);
		flash_load_written += flash_load_boff;
		flash_load_chunk++;
	}

	if (flash_load_written >= flash_load_total) {
		set_bypass(sh, NULL);
		shell_print(sh, "Read all");
	}
}

static int cmd_load(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	int result;
	uint32_t addr;
	uint32_t size;
	k_ssize_t write_block_size;

	result = parse_helper(sh, &argc, &argv, &flash_dev, &addr);
	if (result) {
		return result;
	}

	size = strtoul(argv[2], NULL, 0);

	write_block_size = flash_get_write_block_size(flash_dev);

	/* Check if size is aligned */
	if (size % write_block_size != 0) {
		shell_error(sh, "Size must be %zu bytes aligned", write_block_size);
		return -EIO;
	}

	/* Align buffer size to write_block_size */
	flash_load_buf_size = FLASH_LOAD_BUF_MAX;

	if (flash_load_buf_size < write_block_size) {
		shell_error(sh, "Size of buffer is too small to be aligned to %zu.",
				write_block_size);
		return -ENOSPC;
	}

	/* If buffer size is not aligned then change its size. */
	if (flash_load_buf_size % write_block_size != 0) {
		flash_load_buf_size -= flash_load_buf_size % write_block_size;

		shell_warn(sh, "Load buffer was not aligned to %zu.", write_block_size);
		shell_warn(sh, "Effective load buffer size was set from %d to %d",
				FLASH_LOAD_BUF_MAX, flash_load_buf_size);
	}

	/* Prepare data for callback. */
	flash_load_dev = flash_dev;
	flash_load_addr = addr;
	flash_load_total = size;
	flash_load_written = 0;
	flash_load_boff = 0;
	flash_load_chunk = 0;

	shell_print(sh, "Send %d bytes to complete flash load command", size);

	set_bypass(sh, bypass_cb);
	return 0;
}

static int cmd_page_info(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *flash_dev;
	struct flash_pages_info info;
	int result;
	uint32_t addr;

	result = parse_helper(sh, &argc, &argv, &flash_dev, &addr);
	if (result) {
		return result;
	}

	result = flash_get_page_info_by_offs(flash_dev, addr, &info);

	if (result != 0) {
		shell_error(sh, "Could not determine page size, error code %d.", result);
		return -EINVAL;
	}

	shell_print(sh, "Page for address 0x%x:\nstart offset: 0x%lx\nsize: %zu\nindex: %d", addr,
		    (long)info.start_offset, info.size, info.index);
	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help  = NULL;
	entry->subcmd = &dsub_device_name;
}

SHELL_STATIC_SUBCMD_SET_CREATE(flash_cmds,
	SHELL_CMD_ARG(erase, &dsub_device_name,
		"[<device>] <page address> [<size>]",
		cmd_erase, 2, 2),
	SHELL_CMD_ARG(read, &dsub_device_name,
		"[<device>] <address> [<Dword count>]",
		cmd_read, 2, 2),
	SHELL_CMD_ARG(test, &dsub_device_name,
		"[<device>] <address> <size> <repeat count>",
		cmd_test, 4, 1),
	SHELL_CMD_ARG(write, &dsub_device_name,
		"[<device>] <address> <dword> [<dword>...]",
		cmd_write, 3, BUF_ARRAY_CNT),
	SHELL_CMD_ARG(load, &dsub_device_name,
		"[<device>] <address> <size>",
		cmd_load, 3, 1),
	SHELL_CMD_ARG(page_info, &dsub_device_name,
		"[<device>] <address>",
		cmd_page_info, 2, 1),

#ifdef CONFIG_FLASH_SHELL_TEST_COMMANDS
	SHELL_CMD_ARG(read_test, &dsub_device_name,
		"[<device>] <address> <size> <repeat count>",
		cmd_read_test, 4, 1),
	SHELL_CMD_ARG(write_test, &dsub_device_name,
		"[<device>] <address> <size> <repeat count>",
		cmd_write_test, 4, 1),
	SHELL_CMD_ARG(erase_test, &dsub_device_name,
		"[<device>] <address> <size> <repeat count>",
		cmd_erase_test, 4, 1),
	SHELL_CMD_ARG(erase_write_test, &dsub_device_name,
		"[<device>] <address> <size> <repeat count>",
		cmd_erase_write_test, 4, 1),
#endif

	SHELL_SUBCMD_SET_END
);

static int cmd_flash(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s:unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(flash, &flash_cmds, "Flash shell commands",
		       cmd_flash, 2, 0);
