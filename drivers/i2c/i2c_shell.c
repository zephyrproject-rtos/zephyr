/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_shell, CONFIG_LOG_DEFAULT_LEVEL);

#define MAX_BYTES_FOR_REGISTER_INDEX	4
#define ARGV_DEV	1
#define ARGV_ADDR	2
#define ARGV_REG	3

/* Maximum bytes we can write or read at once */
#define MAX_I2C_BYTES	16

static int get_bytes_count_for_hex(char *arg)
{
	int length = (strlen(arg) + 1) / 2;

	if (length > 1 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
		length -= 1;
	}

	return MIN(MAX_BYTES_FOR_REGISTER_INDEX, length);
}

/*
 * This sends I2C messages without any data (i.e. stop condition after
 * sending just the address). If there is an ACK for the address, it
 * is assumed there is a device present.
 *
 * WARNING: As there is no standard I2C detection command, this code
 * uses arbitrary SMBus commands (namely SMBus quick write and SMBus
 * receive byte) to probe for devices.  This operation can confuse
 * your I2C bus, cause data loss, and is known to corrupt the Atmel
 * AT24RF08 EEPROM found on many IBM Thinkpad laptops.
 *
 * https://manpages.debian.org/buster/i2c-tools/i2cdetect.8.en.html
 */
/* i2c scan <device> */
static int cmd_i2c_scan(const struct shell *shell_ctx,
			size_t argc, char **argv)
{
	const struct device *dev;
	uint8_t cnt = 0, first = 0x04, last = 0x77;

	dev = device_get_binding(argv[ARGV_DEV]);

	if (!dev) {
		shell_error(shell_ctx, "I2C: Device driver %s not found.",
			    argv[ARGV_DEV]);
		return -ENODEV;
	}

	shell_print(shell_ctx,
		    "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
	for (uint8_t i = 0; i <= last; i += 16) {
		shell_fprintf(shell_ctx, SHELL_NORMAL, "%02x: ", i);
		for (uint8_t j = 0; j < 16; j++) {
			if (i + j < first || i + j > last) {
				shell_fprintf(shell_ctx, SHELL_NORMAL, "   ");
				continue;
			}

			struct i2c_msg msgs[1];
			uint8_t dst;

			/* Send the address to read from */
			msgs[0].buf = &dst;
			msgs[0].len = 0U;
			msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;
			if (i2c_transfer(dev, &msgs[0], 1, i + j) == 0) {
				shell_fprintf(shell_ctx, SHELL_NORMAL,
					      "%02x ", i + j);
				++cnt;
			} else {
				shell_fprintf(shell_ctx, SHELL_NORMAL, "-- ");
			}
		}
		shell_print(shell_ctx, "");
	}

	shell_print(shell_ctx, "%u devices found on %s",
		    cnt, argv[ARGV_DEV]);

	return 0;
}

/* i2c recover <device> */
static int cmd_i2c_recover(const struct shell *shell_ctx,
			   size_t argc, char **argv)
{
	const struct device *dev;
	int err;

	dev = device_get_binding(argv[ARGV_DEV]);
	if (!dev) {
		shell_error(shell_ctx, "I2C: Device driver %s not found.",
			    argv[1]);
		return -ENODEV;
	}

	err = i2c_recover_bus(dev);
	if (err) {
		shell_error(shell_ctx, "I2C: Bus recovery failed (err %d)",
			    err);
		return err;
	}

	return 0;
}

static int i2c_write_from_buffer(const struct shell *shell_ctx,
		char *s_dev_name, char *s_dev_addr, char *s_reg_addr,
		char **data, uint8_t data_length)
{
	/* This buffer must preserve 4 bytes for register address, as it is
	 * filled using put_be32 function and we don't want to lower available
	 * space when using 1 byte address.
	 */
	uint8_t buf[MAX_I2C_BYTES + MAX_BYTES_FOR_REGISTER_INDEX - 1];
	const struct device *dev;
	int reg_addr_bytes;
	int reg_addr;
	int dev_addr;
	int ret;
	int i;

	dev = device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(shell_ctx, "I2C: Device driver %s not found.",
			    s_dev_name);
		return -ENODEV;
	}

	dev_addr = strtol(s_dev_addr, NULL, 16);
	reg_addr = strtol(s_reg_addr, NULL, 16);

	reg_addr_bytes = get_bytes_count_for_hex(s_reg_addr);
	sys_put_be32(reg_addr, buf);

	if (data_length + reg_addr_bytes > MAX_I2C_BYTES) {
		data_length = MAX_I2C_BYTES - reg_addr_bytes;
		shell_info(shell_ctx, "Too many bytes provided, limit is %d",
			   MAX_I2C_BYTES - reg_addr_bytes);
	}

	for (i = 0; i < data_length; i++) {
		buf[MAX_BYTES_FOR_REGISTER_INDEX + i] =
			(uint8_t)strtol(data[i], NULL, 16);
	}

	ret = i2c_write(dev,
			buf + MAX_BYTES_FOR_REGISTER_INDEX - reg_addr_bytes,
			reg_addr_bytes + data_length, dev_addr);
	if (ret < 0) {
		shell_error(shell_ctx, "Failed to read from device: %s",
			    s_dev_addr);
		return -EIO;
	}

	return 0;
}

/* i2c write <device> <dev_addr> <reg_addr> [<byte1>, ...] */
static int cmd_i2c_write(const struct shell *shell_ctx,
			 size_t argc, char **argv)
{
	return i2c_write_from_buffer(shell_ctx, argv[ARGV_DEV],
				     argv[ARGV_ADDR], argv[ARGV_REG],
				     &argv[4], argc - 4);
}

/* i2c write_byte <device> <dev_addr> <reg_addr> <value> */
static int cmd_i2c_write_byte(const struct shell *shell_ctx,
			      size_t argc, char **argv)
{
	return i2c_write_from_buffer(shell_ctx, argv[ARGV_DEV],
				     argv[ARGV_ADDR], argv[ARGV_REG],
				     &argv[4], 1);
}

static int i2c_read_to_buffer(const struct shell *shell_ctx,
			      char *s_dev_name,
			      char *s_dev_addr, char *s_reg_addr,
			      uint8_t *buf, uint8_t buf_length)
{
	const struct device *dev;
	uint8_t reg_addr_buf[MAX_BYTES_FOR_REGISTER_INDEX];
	int reg_addr_bytes;
	int reg_addr;
	int dev_addr;
	int ret;

	dev = device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(shell_ctx, "I2C: Device driver %s not found.",
			    s_dev_name);
		return -ENODEV;
	}

	dev_addr = strtol(s_dev_addr, NULL, 16);
	reg_addr = strtol(s_reg_addr, NULL, 16);

	reg_addr_bytes = get_bytes_count_for_hex(s_reg_addr);
	sys_put_be32(reg_addr, reg_addr_buf);

	ret = i2c_write_read(dev, dev_addr,
			     reg_addr_buf +
			       MAX_BYTES_FOR_REGISTER_INDEX - reg_addr_bytes,
			     reg_addr_bytes, buf, buf_length);
	if (ret < 0) {
		shell_error(shell_ctx, "Failed to read from device: %s",
			    s_dev_addr);
		return -EIO;
	}

	return 0;
}

/* i2c read_byte <device> <dev_addr> <reg_addr> */
static int cmd_i2c_read_byte(const struct shell *shell_ctx,
			     size_t argc, char **argv)
{
	uint8_t out;
	int ret;


	ret = i2c_read_to_buffer(shell_ctx, argv[ARGV_DEV],
				 argv[ARGV_ADDR], argv[ARGV_REG], &out, 1);
	if (ret == 0) {
		shell_print(shell_ctx, "Output: 0x%x", out);
	}

	return ret;
}

/* i2c read <device> <dev_addr> <reg_addr> [<numbytes>] */
static int cmd_i2c_read(const struct shell *shell_ctx, size_t argc, char **argv)
{
	uint8_t buf[MAX_I2C_BYTES];
	int num_bytes;
	int ret;

	if (argc > 4) {
		num_bytes = strtol(argv[4], NULL, 16);
		if (num_bytes > MAX_I2C_BYTES) {
			num_bytes = MAX_I2C_BYTES;
		}
	} else {
		num_bytes = MAX_I2C_BYTES;
	}

	ret = i2c_read_to_buffer(shell_ctx, argv[ARGV_DEV],
				 argv[ARGV_ADDR], argv[ARGV_REG],
				 buf, num_bytes);
	if (ret == 0) {
		shell_hexdump(shell_ctx, buf, num_bytes);
	}

	return ret;
}

/* i2c speed <device> <speed>
 * For: speed see constants like I2C_SPEED_STANDARD
 */
static int cmd_i2c_speed(const struct shell *shell_ctx, size_t argc, char **argv)
{
	char *s_dev_name = argv[ARGV_DEV];
	const struct device *dev;
	uint32_t dev_config = 0;
	uint32_t speed;
	int ret;

	dev = device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(shell_ctx, "I2C: Device driver %s not found.",
			    s_dev_name);
		return -ENODEV;
	}

	speed = strtol(argv[ARGV_DEV + 1], NULL, 10);
	ret = i2c_get_config(dev, &dev_config);
	if (ret == 0) {
		dev_config &= ~I2C_SPEED_MASK;
		dev_config |= I2C_SPEED_SET(speed);
	} else {
		/* Can't get current config. Fallback to something reasonable */
		dev_config = I2C_MODE_CONTROLLER | I2C_SPEED_SET(speed);
	}

	ret = i2c_configure(dev, dev_config);
	if (ret < 0) {
		shell_error(shell_ctx, "I2C: Failed to configure device: %s",
			    s_dev_name);
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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_i2c_cmds,
	SHELL_CMD_ARG(scan, &dsub_device_name,
		      "Scan I2C devices\n"
		      "Usage: scan <device>",
		      cmd_i2c_scan, 2, 0),
	SHELL_CMD_ARG(recover, &dsub_device_name,
		      "Recover I2C bus\n"
		      "Usage: recover <device>",
		      cmd_i2c_recover, 2, 0),
	SHELL_CMD_ARG(read, &dsub_device_name,
		      "Read bytes from an I2C device\n"
		      "Usage: read <device> <addr> <reg> [<bytes>]",
		      cmd_i2c_read, 4, 1),
	SHELL_CMD_ARG(read_byte, &dsub_device_name,
		      "Read a byte from an I2C device\n"
		      "Usage: read_byte <device> <addr> <reg>",
		      cmd_i2c_read_byte, 4, 0),
	SHELL_CMD_ARG(write, &dsub_device_name,
		      "Write bytes to an I2C device\n"
		      "Usage: write <device> <addr> <reg> [<byte1>, ...]",
		      cmd_i2c_write, 4, MAX_I2C_BYTES),
	SHELL_CMD_ARG(write_byte, &dsub_device_name,
		      "Write a byte to an I2C device\n"
		      "Usage: write_byte <device> <addr> <reg> <value>",
		      cmd_i2c_write_byte, 5, 0),
	SHELL_CMD_ARG(speed, &dsub_device_name,
		      "Configure I2C bus speed\n"
		      "Usage: speed <device> <speed>",
		      cmd_i2c_speed, 3, 0),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(i2c, &sub_i2c_cmds, "I2C commands", NULL);
