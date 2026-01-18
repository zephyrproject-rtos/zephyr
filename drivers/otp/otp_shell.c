/*
 * Copyright (c) 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/otp.h>

struct args_index {
	uint8_t device;
	uint8_t offset;
	uint8_t length;
	uint8_t data;
};

static const struct args_index args_indx = {
	.device = 1,
	.offset = 2,
	.length = 3,
	.data = 3,
};

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t data[SHELL_HEXDUMP_BYTES_IN_LINE];
	const struct device *dev;
	off_t offset;
	size_t len;
	size_t pending;
	int err;

	offset = shell_strtoul(argv[args_indx.offset], 0, &err);
	if (err < 0) {
		shell_error(sh, "Offset parsing error (err %d)", err);
		return err;
	}
	len = shell_strtoul(argv[args_indx.length], 0, &err);
	if (err < 0) {
		shell_error(sh, "Length parsing error (err %d)", err);
		return err;
	}

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (dev == NULL) {
		shell_error(sh, "OTP device not found");
		return -EINVAL;
	}

	shell_print(sh, "Reading %zu bytes from OTP, offset %zu...", len, offset);

	for (size_t i = 0; i < len; i += pending) {
		pending = MIN(len - i, SHELL_HEXDUMP_BYTES_IN_LINE);

		err = otp_read(dev, offset, data, pending);
		if (err < 0) {
			shell_error(sh, "OTP read failed (err %d)", err);
			return err;
		}

		shell_hexdump_line(sh, offset, data, pending);
		offset += pending;
	}

	shell_print(sh, "");
	return 0;
}

static int cmd_program(const struct shell *sh, size_t argc, char **argv)
{
#if defined(CONFIG_OTP_PROGRAM)
	uint8_t wr_buf[CONFIG_OTP_SHELL_BUFFER_SIZE];
	uint8_t rd_buf[CONFIG_OTP_SHELL_BUFFER_SIZE];
	const struct device *dev;
	unsigned long byte;
	off_t offset;
	size_t len;
	int err;

	offset = shell_strtoul(argv[args_indx.offset], 0, &err);
	if (err < 0) {
		shell_error(sh, "Offset parsing error (err %d)", err);
		return err;
	}
	len = argc - args_indx.data;

	if (len > sizeof(wr_buf)) {
		shell_error(sh, "Write buffer size (%zu bytes) exceeded", sizeof(wr_buf));
		return -EINVAL;
	}

	for (size_t i = 0; i < len; i++) {
		byte = shell_strtoul(argv[args_indx.data + i], 0, &err);
		if (byte > UINT8_MAX || err < 0) {
			shell_error(sh, "Error parsing data byte %d (err %d)", i, err);
			return -EINVAL;
		}
		wr_buf[i] = byte;
	}

	dev = shell_device_get_binding(argv[args_indx.device]);
	if (dev == NULL) {
		shell_error(sh, "OTP device not found");
		return -EINVAL;
	}

	shell_print(sh, "Programming %zu bytes onto OTP, offset %d...", len, offset);

	err = otp_program(dev, offset, wr_buf, len);
	if (err < 0) {
		shell_error(sh, "OTP program failed (err %d)", err);
		return err;
	}

	shell_print(sh, "Verifying...");

	err = otp_read(dev, offset, rd_buf, len);
	if (err < 0) {
		shell_error(sh, "OTP read failed (err %d)", err);
		return err;
	}

	if (memcmp(wr_buf, rd_buf, len) != 0) {
		shell_error(sh, "Verify failed");
		return -EIO;
	}

	shell_print(sh, "Verify OK");

	return 0;
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_error(sh, "OTP programming disabled");

	return -ENOTSUP;
#endif
}

static bool device_is_otp(const struct device *dev)
{
	return DEVICE_API_IS(otp, dev);
}

/* Device name autocompletion support */
static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_otp);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

SHELL_STATIC_SUBCMD_SET_CREATE(
	otp_cmds,
	SHELL_CMD_ARG(read, &dsub_device_name,
		      SHELL_HELP("Read data from OTP device", "<device> <offset> <length>"),
		      cmd_read, 4, 0),
	SHELL_CMD_ARG(program, &dsub_device_name,
		      SHELL_HELP("Program data onto OTP device\n"
				 "WARNING: Irreversible action!",
				 "<device> <offset> <byte0> [byte1] .. [byteN]"),
		      cmd_program, 4, CONFIG_OTP_SHELL_BUFFER_SIZE - 1),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(otp, &otp_cmds, "OTP shell commands", NULL);
