/*
 * Copyright (c) 2018 Prevas A/S
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <errno.h>
#include <zephyr/sys/slist.h>
#include <zephyr/drivers/smbus.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smbus_shell, CONFIG_LOG_DEFAULT_LEVEL);

/**
 * smbus_shell is a highly modified version from i2c_shell. Basically only scan
 * logic remains from i2c_shell
 */

/**
 * Simplify argument parsing, smbus arguments always go in this order:
 * smbus <shell command> <device> <peripheral address> <command byte>
 */
#define ARGV_DEV	1
#define ARGV_ADDR	2
#define ARGV_CMD	3

/**
 * This sends SMBUS messages without any data (i.e. stop condition after
 * sending just the address). If there is an ACK for the address, it
 * is assumed there is a device present.
 *
 * WARNING: As there is no standard SMBUS detection command, this code
 * uses arbitrary SMBus commands (namely SMBus quick write to probe for
 * devices.
 * This operation can confuse your SMBUS bus, cause data loss, and is
 * known to corrupt the Atmel AT24RF08 EEPROM found on many IBM
 * Thinkpad laptops.
 *
 * https://manpages.debian.org/buster/i2c-tools/i2cdetect.8.en.html
 */

/* smbus scan <device> */
static int cmd_smbus_scan(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t cnt = 0, first = 0x04, last = 0x77;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	shell_print(sh, "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
	for (uint8_t i = 0; i <= last; i += 16) {
		shell_fprintf(sh, SHELL_NORMAL, "%02x: ", i);

		for (uint8_t j = 0; j < 16; j++) {
			if (i + j < first || i + j > last) {
				shell_fprintf(sh, SHELL_NORMAL, "   ");
				continue;
			}

			if (smbus_quick(dev, i + j, SMBUS_MSG_WRITE) == 0) {
				shell_fprintf(sh, SHELL_NORMAL, "%02x ", i + j);
				++cnt;
			} else {
				shell_fprintf(sh, SHELL_NORMAL, "-- ");
			}
		}

		shell_print(sh, "");
	}

	shell_print(sh, "%u devices found on %s", cnt, argv[ARGV_DEV]);

	return 0;
}

/* smbus quick <device> <dev_addr> */
static int cmd_smbus_quick(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t addr;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	addr = strtol(argv[ARGV_ADDR], NULL, 16);

	ret = smbus_quick(dev, addr, SMBUS_MSG_WRITE);
	if (ret < 0) {
		shell_error(sh, "SMBus: Failed quick cmd, perip: 0x%02x", addr);
	}

	return ret;
}

/* smbus byte_read <device> <dev_addr> */
static int cmd_smbus_byte_read(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t addr;
	uint8_t out;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	addr = strtol(argv[ARGV_ADDR], NULL, 16);

	ret = smbus_byte_read(dev, addr, &out);
	if (ret < 0) {
		shell_error(sh, "SMBus: Failed to read from periph: 0x%02x",
			    addr);
		return -EIO;
	}

	shell_print(sh, "Output: 0x%x", out);

	return 0;
}

/* smbus byte_write <device> <dev_addr> <value> */
static int cmd_smbus_byte_write(const struct shell *sh,
				size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t addr;
	uint8_t value;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	addr = strtol(argv[ARGV_ADDR], NULL, 16);
	/* First byte is command */
	value = strtol(argv[ARGV_CMD], NULL, 16);

	ret = smbus_byte_write(dev, addr, value);
	if (ret < 0) {
		shell_error(sh, "SMBus: Failed to write to periph: 0x%02x",
			    addr);
		return -EIO;
	}

	return 0;
}

/* smbus byte_data_read <device> <dev_addr> <cmd> */
static int cmd_smbus_byte_data_read(const struct shell *sh,
				    size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t addr, command;
	uint8_t out;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	addr = strtol(argv[ARGV_ADDR], NULL, 16);
	command = strtol(argv[ARGV_CMD], NULL, 16);

	ret = smbus_byte_data_read(dev, addr, command, &out);
	if (ret < 0) {
		shell_error(sh, "SMBus: Failed to read from periph: 0x%02x",
			    addr);
		return -EIO;
	}

	shell_print(sh, "Output: 0x%x", out);

	return 0;
}

/* smbus byte_data_write <device> <dev_addr> <cmd> <value> */
static int cmd_smbus_byte_data_write(const struct shell *sh,
				     size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t addr, command;
	uint8_t value;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	addr = strtol(argv[ARGV_ADDR], NULL, 16);
	command = strtol(argv[ARGV_CMD], NULL, 16);
	value = strtol(argv[4], NULL, 16);

	ret = smbus_byte_data_write(dev, addr, command, value);
	if (ret < 0) {
		shell_error(sh, "SMBus: Failed to write to periph: 0x%02x",
			    addr);
		return -EIO;
	}

	return 0;
}

/* smbus word_data_read <device> <dev_addr> <cmd> */
static int cmd_smbus_word_data_read(const struct shell *sh,
				    size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t addr, command;
	uint16_t out;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	addr = strtol(argv[ARGV_ADDR], NULL, 16);
	command = strtol(argv[ARGV_CMD], NULL, 16);

	ret = smbus_word_data_read(dev, addr, command, &out);
	if (ret < 0) {
		shell_error(sh, "SMBus: Failed to read from periph: 0x%02x",
			    addr);
		return -EIO;
	}

	shell_print(sh, "Output: 0x%04x", out);

	return 0;
}

/* smbus word_data_write <device> <dev_addr> <cmd> <value> */
static int cmd_smbus_word_data_write(const struct shell *sh,
				     size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t addr, command;
	uint16_t value;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	addr = strtol(argv[ARGV_ADDR], NULL, 16);
	command = strtol(argv[ARGV_CMD], NULL, 16);
	value = strtol(argv[4], NULL, 16);

	ret = smbus_word_data_write(dev, addr, command, value);
	if (ret < 0) {
		shell_error(sh, "SMBus: Failed to write to periph: 0x%02x",
			    addr);
		return -EIO;
	}

	return 0;
}

/* smbus block_write <device> <dev_addr> <cmd> <bytes ... > */
static int cmd_smbus_block_write(const struct shell *sh,
				 size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t addr, command;
	uint8_t count = argc - 4;
	char **p = &argv[4]; /* start data bytes */
	uint8_t buf[32]; /* max block count */
	int ret;

	if (count == 0 || count > sizeof(buf)) {
		return -EINVAL;
	}

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	addr = strtol(argv[ARGV_ADDR], NULL, 16);
	command = strtol(argv[ARGV_CMD], NULL, 16);

	for (int i = 0; i < count; i++) {
		buf[i] = (uint8_t)strtoul(p[i], NULL, 16);
	}

	LOG_HEXDUMP_DBG(buf, count, "Constructed block buffer");

	ret = smbus_block_write(dev, addr, command, count, buf);
	if (ret < 0) {
		shell_error(sh, "Failed block write to periph: 0x%02x",
			    addr);
		return ret;
	}

	return 0;
}

/* smbus block_read <device> <dev_addr> <cmd> */
static int cmd_smbus_block_read(const struct shell *sh,
				size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t addr, command;
	uint8_t buf[32]; /* max block count */
	uint8_t count;
	int ret;

	dev = shell_device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(sh, "SMBus: Device %s not found", argv[ARGV_DEV]);
		return -ENODEV;
	}

	addr = strtol(argv[ARGV_ADDR], NULL, 16);
	command = strtol(argv[ARGV_CMD], NULL, 16);

	ret = smbus_block_read(dev, addr, command, &count, buf);
	if (ret < 0) {
		shell_error(sh, "Failed block read from periph: 0x%02x",
			    addr);
		return ret;
	}

	if (count == 0 || count > sizeof(buf)) {
		shell_error(sh, "Returned count %u", count);
		return -ENODATA;
	}

	shell_hexdump(sh, buf, count);

	return 0;
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, "smbus");

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_smbus_cmds,
	SHELL_CMD_ARG(quick, &dsub_device_name,
		      "SMBus Quick command\n"
		      "Usage: quick <device> <addr>",
		      cmd_smbus_quick, 3, 0),
	SHELL_CMD_ARG(scan, &dsub_device_name,
		      "Scan SMBus peripheral devices command\n"
		      "Usage: scan <device>",
		      cmd_smbus_scan, 2, 0),
	SHELL_CMD_ARG(byte_read, &dsub_device_name,
		      "SMBus: byte read command\n"
		      "Usage: byte_read <device> <addr>",
		      cmd_smbus_byte_read, 3, 0),
	SHELL_CMD_ARG(byte_write, &dsub_device_name,
		      "SMBus: byte write command\n"
		      "Usage: byte_write <device> <addr> <value>",
		      cmd_smbus_byte_write, 4, 0),
	SHELL_CMD_ARG(byte_data_read, &dsub_device_name,
		      "SMBus: byte data read command\n"
		      "Usage: byte_data_read <device> <addr> <cmd>",
		      cmd_smbus_byte_data_read, 4, 0),
	SHELL_CMD_ARG(byte_data_write, &dsub_device_name,
		      "SMBus: byte data write command\n"
		      "Usage: byte_data_write <device> <addr> <cmd> <value>",
		      cmd_smbus_byte_data_write, 5, 0),
	SHELL_CMD_ARG(word_data_read, &dsub_device_name,
		      "SMBus: word data read command\n"
		      "Usage: word_data_read <device> <addr> <cmd>",
		      cmd_smbus_word_data_read, 4, 0),
	SHELL_CMD_ARG(word_data_write, &dsub_device_name,
		      "SMBus: word data write command\n"
		      "Usage: word_data_write <device> <addr> <cmd> <value>",
		      cmd_smbus_word_data_write, 5, 0),
	SHELL_CMD_ARG(block_write, &dsub_device_name,
		      "SMBus: Block Write command\n"
		      "Usage: block_write <device> <addr> <cmd> [<byte1>, ...]",
		      cmd_smbus_block_write, 4, 32),
	SHELL_CMD_ARG(block_read, &dsub_device_name,
		      "SMBus: Block Read command\n"
		      "Usage: block_read <device> <addr> <cmd>",
		      cmd_smbus_block_read, 4, 0),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(smbus, &sub_smbus_cmds, "smbus commands", NULL);
