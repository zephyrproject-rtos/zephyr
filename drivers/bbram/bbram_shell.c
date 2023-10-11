/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

/* Buffer is only needed for bytes that follow command, device and address. */
#define BUF_ARRAY_CNT (CONFIG_SHELL_ARGC_MAX - 3)

static inline int parse_ul(const char *str, unsigned long *result)
{
	char *end;
	unsigned long val;

	val = strtoul(str, &end, 0);

	if (*str == '\0' || *end != '\0') {
		return -EINVAL;
	}

	*result = val;
	return 0;
}

static inline int parse_u32(const char *str, uint32_t *result)
{
	unsigned long val;

	if (parse_ul(str, &val) || val > 0xffffffff) {
		return -EINVAL;
	}
	*result = (uint32_t)val;
	return 0;
}

static inline int parse_u8(const char *str, uint8_t *result)
{
	unsigned long val;

	if (parse_ul(str, &val) || val > 0xff) {
		return -EINVAL;
	}
	*result = (uint8_t)val;
	return 0;
}

static inline int parse_device(const struct shell *sh, size_t argc, char *argv[],
			       const struct device **bbram_dev)
{
	if (argc < 2) {
		shell_error(sh, "Missing BBRAM device");
		return -EINVAL;
	}

	*bbram_dev = device_get_binding(argv[1]);
	if (!*bbram_dev) {
		shell_error(sh, "Given BBRAM device was not found");
		return -ENODEV;
	}

	return 0;
}

static int cmd_read(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *bbram_dev;
	uint32_t addr;
	size_t size;
	int part_size, ret;

	ret = parse_device(sh, argc, argv, &bbram_dev);
	if (ret) {
		return ret;
	}

	if (argc < 3) {
		/* Dump whole BBRAM if address not provided. */
		addr = 0;
		ret = bbram_get_size(bbram_dev, &size);
		if (ret < 0) {
			shell_error(sh, "Can't get BBRAM size: %d", ret);
			return -EIO;
		}
	} else {
		/* Parse address if provided. */
		ret = parse_u32(argv[2], &addr);
		if (ret) {
			return ret;
		}

		/* If size not provided read one byte. */
		size = 1;

		if (argc >= 4) {
			/* Parse size if provided. */
			ret = parse_u32(argv[3], &size);
			if (ret) {
				return ret;
			}
		}
	}

	for (int cnt = 0; cnt < size; cnt += part_size) {
		uint8_t data[SHELL_HEXDUMP_BYTES_IN_LINE];

		part_size = MIN(size - cnt, SHELL_HEXDUMP_BYTES_IN_LINE);
		ret = bbram_read(bbram_dev, addr, part_size, data);
		if (ret != 0) {
			shell_error(sh, "BBRAM read error: %d", ret);
			return -EIO;
		}
		shell_hexdump_line(sh, addr, data, part_size);
		addr += part_size;
	}

	shell_print(sh, "");
	return 0;
}

static int cmd_write(const struct shell *sh, size_t argc, char *argv[])
{
	const struct device *bbram_dev;
	uint8_t buf[BUF_ARRAY_CNT];
	uint32_t addr;
	size_t size = 0;
	int ret;

	ret = parse_device(sh, argc, argv, &bbram_dev);
	if (ret) {
		return ret;
	}

	if (argc < 3) {
		shell_error(sh, "Missing address");
		return -EINVAL;
	}

	/* Parse address. */
	ret = parse_u32(argv[2], &addr);
	if (ret) {
		return ret;
	}

	/* Parse bytes and place them in the buffer. */
	for (int i = 3; i < argc; i++) {
		ret = parse_u8(argv[i], &buf[i - 3]);
		if (ret) {
			return ret;
		}
		size++;
	}

	if (size == 0) {
		shell_error(sh, "Missing data");
		return -EINVAL;
	}

	ret = bbram_write(bbram_dev, addr, size, buf);
	if (ret < 0) {
		shell_error(sh, "BBRAM write error: %d", ret);
		return -EIO;
	}

	return 0;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(bbram_cmds,
			       SHELL_CMD_ARG(read, &dsub_device_name,
					     "<device> [<address>] [<count>]", cmd_read, 2, 2),
			       SHELL_CMD_ARG(write, &dsub_device_name,
					     "<device> <address> <byte> [<byte>...]", cmd_write, 4,
					     BUF_ARRAY_CNT),
			       SHELL_SUBCMD_SET_END);

static int cmd_bbram(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s: unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(bbram, &bbram_cmds, "Battery-backed RAM shell commands", cmd_bbram, 2, 0);
