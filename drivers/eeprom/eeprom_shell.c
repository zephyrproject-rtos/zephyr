/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2021 Lemonbeat GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EEPROM shell commands.
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/eeprom.h>
#include <stdlib.h>

struct args_index {
	uint8_t device;
	uint8_t offset;
	uint8_t length;
	uint8_t data;
	uint8_t pattern;
};

static const struct args_index args_indx = {
	.device = 1,
	.offset = 2,
	.length = 3,
	.data = 3,
	.pattern = 4,
};

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *eeprom;
	size_t addr;
	size_t len;
	size_t pending;
	size_t upto;
	int err;

	addr = strtoul(argv[args_indx.offset], NULL, 0);
	len = strtoul(argv[args_indx.length], NULL, 0);

	eeprom = device_get_binding(argv[args_indx.device]);
	if (!eeprom) {
		shell_error(sh, "EEPROM device not found");
		return -EINVAL;
	}

	shell_print(sh, "Reading %zu bytes from EEPROM, offset %zu...", len,
		    addr);

	for (upto = 0; upto < len; upto += pending) {
		uint8_t data[SHELL_HEXDUMP_BYTES_IN_LINE];

		pending = MIN(len - upto, SHELL_HEXDUMP_BYTES_IN_LINE);
		err = eeprom_read(eeprom, addr, data, pending);
		if (err) {
			shell_error(sh, "EEPROM read failed (err %d)", err);
			return err;
		}

		shell_hexdump_line(sh, addr, data, pending);
		addr += pending;
	}

	shell_print(sh, "");
	return 0;
}

static int cmd_write(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t wr_buf[CONFIG_EEPROM_SHELL_BUFFER_SIZE];
	uint8_t rd_buf[CONFIG_EEPROM_SHELL_BUFFER_SIZE];
	const struct device *eeprom;
	unsigned long byte;
	off_t offset;
	size_t len;
	int err;
	int i;

	offset = strtoul(argv[args_indx.offset], NULL, 0);
	len = argc - args_indx.data;

	if (len > sizeof(wr_buf)) {
		shell_error(sh, "Write buffer size (%zu bytes) exceeded",
			    sizeof(wr_buf));
		return -EINVAL;
	}

	for (i = 0; i < len; i++) {
		byte = strtoul(argv[args_indx.data + i], NULL, 0);
		if (byte > UINT8_MAX) {
			shell_error(sh, "Error parsing data byte %d", i);
			return -EINVAL;
		}
		wr_buf[i] = byte;
	}

	eeprom = device_get_binding(argv[args_indx.device]);
	if (!eeprom) {
		shell_error(sh, "EEPROM device not found");
		return -EINVAL;
	}

	shell_print(sh, "Writing %zu bytes to EEPROM...", len);

	err = eeprom_write(eeprom, offset, wr_buf, len);
	if (err) {
		shell_error(sh, "EEPROM write failed (err %d)", err);
		return err;
	}

	shell_print(sh, "Verifying...");

	err = eeprom_read(eeprom, offset, rd_buf, len);
	if (err) {
		shell_error(sh, "EEPROM read failed (err %d)", err);
		return err;
	}

	if (memcmp(wr_buf, rd_buf, len) != 0) {
		shell_error(sh, "Verify failed");
		return -EIO;
	}

	shell_print(sh, "Verify OK");

	return 0;
}

static int cmd_size(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *eeprom;

	eeprom = device_get_binding(argv[args_indx.device]);
	if (!eeprom) {
		shell_error(sh, "EEPROM device not found");
		return -EINVAL;
	}

	shell_print(sh, "%zu bytes", eeprom_get_size(eeprom));
	return 0;
}

static int cmd_fill(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t wr_buf[CONFIG_EEPROM_SHELL_BUFFER_SIZE];
	uint8_t rd_buf[CONFIG_EEPROM_SHELL_BUFFER_SIZE];
	const struct device *eeprom;
	unsigned long pattern;
	size_t addr;
	size_t initial_offset;
	size_t len;
	size_t pending;
	size_t upto;
	int err;

	initial_offset = strtoul(argv[args_indx.offset], NULL, 0);
	len = strtoul(argv[args_indx.length], NULL, 0);

	pattern = strtoul(argv[args_indx.pattern], NULL, 0);
	if (pattern > UINT8_MAX) {
		shell_error(sh, "Error parsing pattern byte");
		return -EINVAL;
	}
	memset(wr_buf, pattern, MIN(len, CONFIG_EEPROM_SHELL_BUFFER_SIZE));

	eeprom = device_get_binding(argv[args_indx.device]);
	if (!eeprom) {
		shell_error(sh, "EEPROM device not found");
		return -EINVAL;
	}

	shell_print(sh, "Writing %zu bytes of 0x%02lx to EEPROM...", len,
		    pattern);

	addr = initial_offset;

	for (upto = 0; upto < len; upto += pending) {
		pending = MIN(len - upto, CONFIG_EEPROM_SHELL_BUFFER_SIZE);
		err = eeprom_write(eeprom, addr, wr_buf, pending);
		if (err) {
			shell_error(sh, "EEPROM write failed (err %d)", err);
			return err;
		}
		addr += pending;
	}

	addr = initial_offset;

	shell_print(sh, "Verifying...");

	for (upto = 0; upto < len; upto += pending) {
		pending = MIN(len - upto, CONFIG_EEPROM_SHELL_BUFFER_SIZE);
		err = eeprom_read(eeprom, addr, rd_buf, pending);
		if (err) {
			shell_error(sh, "EEPROM read failed (err %d)", err);
			return err;
		}

		if (memcmp(wr_buf, rd_buf, pending) != 0) {
			shell_error(sh, "Verify failed");
			return -EIO;
		}

		addr += pending;
	}

	shell_print(sh, "Verify OK");

	return 0;
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(eeprom_cmds,
	SHELL_CMD_ARG(read, &dsub_device_name,
		      "<device> <offset> <length>", cmd_read, 4, 0),
	SHELL_CMD_ARG(write, &dsub_device_name,
		      "<device> <offset> [byte0] <byte1> .. <byteN>", cmd_write,
		      4, CONFIG_EEPROM_SHELL_BUFFER_SIZE - 1),
	SHELL_CMD_ARG(size, &dsub_device_name, "<device>", cmd_size, 2, 0),
	SHELL_CMD_ARG(fill, &dsub_device_name,
		      "<device> <offset> <length> <pattern>", cmd_fill, 5, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(eeprom, &eeprom_cmds, "EEPROM shell commands", NULL);
