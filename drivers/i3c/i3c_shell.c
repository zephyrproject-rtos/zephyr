/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i3c_shell, CONFIG_LOG_DEFAULT_LEVEL);

#define MAX_BYTES_FOR_REGISTER_INDEX	4
#define ARGV_DEV	1
#define ARGV_PID_H	2
#define ARGV_PID_L	3

#define ARGV_ADDR	2
#define ARGV_REG	3

/* Maximum bytes we can write or read at once */
#define MAX_BYTES	16

static int get_bytes_count_for_hex(char *arg)
{
	int length = (strlen(arg) + 1) / 2;

	if (length > 1 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
		length -= 1;
	}

	return MIN(MAX_BYTES_FOR_REGISTER_INDEX, length);
}

static int i3c_write_from_buffer(const struct shell *shell_ctx,
		char *s_dev_name, char *s_dev_pid_high, char *s_dev_pid_low,
		char **data, uint8_t data_length)
{
	const struct device *dev;
	int pid_H;
	int pid_L;
	int ret;
	int i;
	int err = 0;

	dev = device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.",
			    s_dev_name);
		return -ENODEV;
	}

	pid_H = shell_strtoul(s_dev_pid_high, 16, &err);
	if (err != 0) {
		shell_error(shell_ctx, "invalid parameter: <PID_High[47:32]>");
		return err;
	}

	pid_L = shell_strtoul(s_dev_pid_low, 16, &err);
	if (err != 0) {
		shell_error(shell_ctx, "invalid parameter: <PID_Low[31:0]>");
		return err;
	}

	uint64_t pid = (uint64_t)pid_H << 32;

	pid |= pid_L;

	const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);
	struct i3c_device_desc *target = i3c_device_find(dev, &i3c_id);

	uint8_t buf[] = {0};

	if (target != NULL) {
		for (i = 0; i < data_length; i++) {
			buf[i] = (uint8_t)shell_strtoul(data[i], 16, &err);
			if (err != 0) {
				shell_error(shell_ctx, "invalid input bytes parameter");
				return err;
			}
			shell_print(shell_ctx, "To be Written data[%d]: %x", i, buf[i]);
		}

		ret = i3c_write(target, buf, data_length);
		if (ret) {
			shell_error(shell_ctx, "Error writing: error code (%d)", ret);
			return ret;
		}
		shell_print(shell_ctx, "Wrote %d bytes: Device Dynamic address (%d)",
			data_length, target->dynamic_addr);

		return 0;
	}
	shell_error(shell_ctx, "Error finding device with PID 0x%012llx", pid);
	return -EIO;
}

/* i3c write <device> <PID_High[47:32]> <PID_Low[31:0]> [<byte1>, ...] */
static int cmd_i3c_write(const struct shell *shell_ctx,
			      size_t argc, char **argv)
{
	return i3c_write_from_buffer(shell_ctx, argv[ARGV_DEV],
				     argv[ARGV_PID_H], argv[ARGV_PID_L],
				     &argv[4], argc - 4);
}


static int i3c_read_to_buffer(const struct shell *shell_ctx,
			      char *s_dev_name,
			      char *s_dev_pid_high, char *s_dev_pid_low,
			      uint8_t *buf, uint8_t buf_length)
{
	const struct device *dev;
	int pid_H;
	int pid_L;
	int ret;
	int err = 0;

	dev = device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(shell_ctx, "I3C: Device driver %s not found.",
			    s_dev_name);
		return -ENODEV;
	}

	pid_H = shell_strtoul(s_dev_pid_high, 16, &err);
	if (err != 0) {
		shell_error(shell_ctx, "invalid parameter: <PID_High[47:32]>");
		return err;
	}

	pid_L = shell_strtoul(s_dev_pid_low, 16, &err);
	if (err != 0) {
		shell_error(shell_ctx, "invalid parameter: <PID_Low[31:0]>");
		return err;
	}

	uint64_t pid = (uint64_t)pid_H << 32;

	pid |= pid_L;

	const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);
	struct i3c_device_desc *target = i3c_device_find(dev, &i3c_id);

	if (target != NULL) {
		ret =  i3c_read(target, buf, buf_length);
		if (ret) {
			shell_error(shell_ctx, "Error reading: error code (%d)\n", ret);
			return ret;
		}
		shell_print(shell_ctx, "\nRead %d bytes from i3c device with address (%d)\n",
				buf_length, target->dynamic_addr);

		return 0;
	}
	shell_error(shell_ctx, "Error finding device with PID 0x%012llx", pid);
	return -EIO;
}


/* i3c read <device> <PID_High[47:32]> <PID_Low[31:0]> [<numbytes>] */
static int cmd_i3c_read(const struct shell *shell_ctx, size_t argc, char **argv)
{
	uint8_t buf[] = {0};
	int num_bytes;
	int ret;
	int err = 0;

	num_bytes = shell_strtoul(argv[4], 16, &err);
	if (err != 0) {
		shell_error(shell_ctx, "invalid <num_bytes> parameter");
		return err;
	}

	ret = i3c_read_to_buffer(shell_ctx, argv[ARGV_DEV],
				 argv[ARGV_PID_H], argv[ARGV_PID_L],
				 buf, num_bytes);
	if (ret == 0) {
		shell_hexdump(shell_ctx, buf, num_bytes);
	}

	return ret;
}
/*****************************************************************/
static int i2c_write_from_buffer(const struct shell *shell_ctx,
		char *s_dev_name, char *s_dev_addr, char *s_reg_addr,
		char **data, uint8_t data_length)
{
	/* This buffer must preserve 4 bytes for register address, as it is
	 * filled using put_be32 function and we don't want to lower available
	 * space when using 1 byte address.
	 */
	uint8_t buf[MAX_BYTES + MAX_BYTES_FOR_REGISTER_INDEX - 1];
	const struct device *dev;
	int reg_addr_bytes;
	int reg_addr;
	int dev_addr;
	int ret;
	int i;
	int err = 0;

	dev = device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(shell_ctx, "I2C: Device driver %s not found.",
			    s_dev_name);
		return -ENODEV;
	}

	dev_addr = shell_strtoul(s_dev_addr, 16, &err);
	if (err != 0) {
		shell_error(shell_ctx, "invalid parameter : <dev_addr>");
		return err;
	}

	reg_addr = shell_strtoul(s_reg_addr, 16, &err);
	if (err != 0) {
		shell_error(shell_ctx, "invalid parameter : <reg_addr>");
		return err;
	}

	reg_addr_bytes = get_bytes_count_for_hex(s_reg_addr);
	sys_put_be32(reg_addr, buf);

	if (data_length + reg_addr_bytes > MAX_BYTES) {
		data_length = MAX_BYTES - reg_addr_bytes;
		shell_info(shell_ctx, "Too many bytes provided, limit is %d",
			   MAX_BYTES - reg_addr_bytes);
	}

	for (i = 0; i < data_length; i++) {
		buf[MAX_BYTES_FOR_REGISTER_INDEX + i] =
			(uint8_t)shell_strtoul(data[i], 16, &err);
		if (err != 0) {
			shell_error(shell_ctx, "invalid input bytes parameter");
			return err;
		}
	}

	ret = i2c_write(dev,
			buf + MAX_BYTES_FOR_REGISTER_INDEX - reg_addr_bytes,
			reg_addr_bytes + data_length, dev_addr);
	if (ret < 0) {
		shell_error(shell_ctx, "Failed to write to device: %s",
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
	int err = 0;

	dev = device_get_binding(s_dev_name);
	if (!dev) {
		shell_error(shell_ctx, "I2C: Device driver %s not found.",
			    s_dev_name);
		return -ENODEV;
	}

	dev_addr = shell_strtoul(s_dev_addr, 16, &err);
	if (err != 0) {
		shell_error(shell_ctx, "invalid parameter : <dev_addr>");
		return err;
	}

	reg_addr = shell_strtoul(s_reg_addr, 16, &err);
	if (err != 0) {
		shell_error(shell_ctx, "invalid parameter : <reg_addr>");
		return err;
	}

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

/* i2c read <device> <dev_addr> <reg_addr> [<numbytes>] */
static int cmd_i2c_read(const struct shell *shell_ctx, size_t argc, char **argv)
{
	uint8_t buf[MAX_BYTES];
	int num_bytes;
	int ret;
	int err = 0;

	if (argc > 4) {
		num_bytes = shell_strtoul(argv[4], 16, &err);

		if (err != 0) {
			shell_error(shell_ctx, "invalid <num_bytes> parameter");
			return err;
		}

		if (num_bytes > MAX_BYTES) {
			num_bytes = MAX_BYTES;
		}
	} else {
		num_bytes = MAX_BYTES;
	}

	ret = i2c_read_to_buffer(shell_ctx, argv[ARGV_DEV],
				 argv[ARGV_ADDR], argv[ARGV_REG],
				 buf, num_bytes);
	if (ret == 0) {
		shell_hexdump(shell_ctx, buf, num_bytes);
	}

	return ret;
}
/**************************************************************/


static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_i3c_cmds,
	SHELL_CMD_ARG(read, &dsub_device_name,
		      "Read bytes from an I3C device\n"
		      "Usage: read <device> <pid_high[47:32]> <pid_low[0:31]> <num_bytes>",
		      cmd_i3c_read, 5, 0),
	SHELL_CMD_ARG(write, &dsub_device_name,
		      "Write bytes to an I3C device\n"
		      "Usage: write <device> <pid_high[47:32]> <pid_low[0:31]> [<byte1>, ...]",
		      cmd_i3c_write, 5, MAX_BYTES),
	SHELL_CMD_ARG(i2c_read, &dsub_device_name,
		      "Read bytes from an I2C device\n"
		      "Usage: i2c_read <device> <addr> <reg> [<num_bytes>]",
		      cmd_i2c_read, 4, 1),
	SHELL_CMD_ARG(i2c_write, &dsub_device_name,
		      "Write bytes to an I2C device\n"
		      "Usage: i2c_write <device> <addr> <reg> [<byte1>, ...]",
		      cmd_i2c_write, 4, MAX_BYTES),
	SHELL_SUBCMD_SET_END     /* Array terminated. */
);

SHELL_CMD_REGISTER(i3c, &sub_i3c_cmds, "I3C commands", NULL);
