/*
 * Copyright (c) 2026 James Walmsley <james@fullfat-fs.co.uk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <stdlib.h>
#include <string.h>

#define OTP_DEFAULT_NODE DT_ALIAS(otp0)

#if defined(DT_N_NODELABEL_otp_lock) && DT_NODE_HAS_STATUS(DT_NODELABEL(otp_lock), okay)
#define OTP_LOCK_AVAILABLE 1
#define OTP_LOCK_NODE      DT_NODELABEL(otp_lock)
#define OTP_LOCK_OFFSET    DT_REG_ADDR(OTP_LOCK_NODE)
#define OTP_LOCK_LEN       DT_REG_SIZE(OTP_LOCK_NODE)
#else
#define OTP_LOCK_AVAILABLE 0
#endif

struct otp_shell_dev_info {
	const struct device *dev;
	size_t size;
};

#define OTP_DEV_ENTRY(node_id) { DEVICE_DT_GET(node_id), DT_REG_SIZE(node_id) },

static const struct otp_shell_dev_info otp_devs[] = {
	/* zephyr-keep-sorted-start */
	DT_FOREACH_STATUS_OKAY(nxp_ocotp, OTP_DEV_ENTRY)
	DT_FOREACH_STATUS_OKAY(sifli_sf32lb_efuse, OTP_DEV_ENTRY)
	DT_FOREACH_STATUS_OKAY(st_stm32_bsec, OTP_DEV_ENTRY)
	DT_FOREACH_STATUS_OKAY(st_stm32_otp, OTP_DEV_ENTRY)
	DT_FOREACH_STATUS_OKAY(zephyr_otp_emul, OTP_DEV_ENTRY)
	/* zephyr-keep-sorted-stop */
};

static const struct device *otp_default_dev = DEVICE_DT_GET_OR_NULL(OTP_DEFAULT_NODE);
static const struct device *otp_dev = DEVICE_DT_GET_OR_NULL(OTP_DEFAULT_NODE);

static const struct device *otp_get_device_by_name(const char *name)
{
	const struct device *dev = device_get_binding(name);

#ifdef CONFIG_DEVICE_DT_METADATA
	if (dev == NULL) {
		dev = device_get_by_dt_nodelabel(name);
	}
#endif

	return dev;
}

static int otp_parse_ulong(const char *str, unsigned long *value)
{
	char *endptr;

	if (str == NULL || str[0] == '-') {
		return -EINVAL;
	}

	errno = 0;
	*value = strtoul(str, &endptr, 0);
	if (errno == ERANGE || endptr == str || *endptr != '\0') {
		return -EINVAL;
	}

	return 0;
}

static bool otp_get_size(const struct device *dev, size_t *size)
{
	for (size_t i = 0; i < ARRAY_SIZE(otp_devs); i++) {
		if (otp_devs[i].dev == dev) {
			*size = otp_devs[i].size;
			return true;
		}
	}

	return false;
}

static int otp_check_ready(const struct shell *sh)
{
	if (otp_dev == NULL) {
		shell_error(sh, "OTP device not selected");
		return -ENODEV;
	}

	if (!device_is_ready(otp_dev)) {
		shell_error(sh, "OTP device %s is not ready", otp_dev->name);
		return -ENODEV;
	}

	return 0;
}

static int cmd_otp_device(const struct shell *sh, size_t argc, char **argv)
{
	if (argc < 2) {
		if (otp_dev == NULL) {
			shell_print(sh, "No OTP device selected");
			return 0;
		}

		if (!device_is_ready(otp_dev)) {
			shell_error(sh, "OTP device %s is not ready", otp_dev->name);
			return -ENODEV;
		}

		shell_print(sh, "Current OTP device: %s", otp_dev->name);
#ifdef CONFIG_DEVICE_DT_METADATA
		const struct device_dt_nodelabels *nl = device_get_dt_nodelabels(otp_dev);

		if (nl != NULL && nl->num_nodelabels > 0) {
			shell_print(sh, "Node labels:");
			for (size_t i = 0; i < nl->num_nodelabels; i++) {
				shell_print(sh, "  %s", nl->nodelabels[i]);
			}
		}
#endif
		return 0;
	}

	const struct device *dev = otp_get_device_by_name(argv[1]);

	if (dev == NULL) {
		shell_error(sh, "Unknown device or nodelabel: %s", argv[1]);
		return -ENODEV;
	}

	if (!device_is_ready(dev)) {
		shell_error(sh, "Device %s is not ready", dev->name);
		return -ENODEV;
	}

	if (!DEVICE_API_IS(otp, dev)) {
		shell_error(sh, "Device %s is not an OTP device", dev->name);
		return -EINVAL;
	}

	otp_dev = dev;
	shell_print(sh, "Selected OTP device: %s", otp_dev->name);
	if (otp_default_dev != NULL && otp_dev != otp_default_dev) {
		shell_warn(sh, "Non-default device selected");
	}

	return 0;
}

static int cmd_otp_dump(const struct shell *sh, size_t argc, char **argv)
{
	size_t otp_size = 0;
	unsigned long dump_len = 0;
	uint8_t buf[16];
	size_t offset = 0;
	int ret;

	ret = otp_check_ready(sh);
	if (ret != 0) {
		return ret;
	}

	if (!otp_get_size(otp_dev, &otp_size)) {
		if (argc >= 2) {
			ret = otp_parse_ulong(argv[1], &dump_len);
			if (ret != 0 || dump_len == 0) {
				shell_error(sh, "Invalid length");
				return -EINVAL;
			}
			otp_size = (size_t)dump_len;
		} else {
			shell_error(sh, "Size unknown for selected device; use: otp dump <len>");
			return -EINVAL;
		}
	}

	while (offset < otp_size) {
		size_t len = MIN(sizeof(buf), otp_size - offset);

		ret = otp_read(otp_dev, (off_t)offset, buf, len);
		if (ret < 0) {
			shell_error(sh, "OTP read failed at 0x%x: %d", (unsigned int)offset,
				    ret);
			return ret;
		}

		shell_hexdump_line(sh, (unsigned int)offset, buf, len);
		offset += len;
	}

	return 0;
}

static int cmd_otp_program(const struct shell *sh, size_t argc, char **argv)
{
	unsigned long offset;
	int ret;
	size_t otp_size = 0;
	bool check_size;

	ret = otp_check_ready(sh);
	if (ret != 0) {
		return ret;
	}

	if (argc < 3) {
		shell_error(sh, "Usage: otp program <offset> <byte> [byte ...]");
		return -EINVAL;
	}

	check_size = otp_get_size(otp_dev, &otp_size);
	offset = strtoul(argv[1], NULL, 0);
	if (check_size && offset >= otp_size) {
		shell_error(sh, "Offset out of range");
		return -EINVAL;
	}

	shell_info(sh, "write: %08lx, %d bytes", offset, 1);

	for (size_t i = 2; i < argc; i++) {
		shell_info(sh, "i=%d, argc=%d", i, argc);
		uint8_t value = (uint8_t)strtoul(argv[i], NULL, 0);
		size_t write_offset = offset + (i - 2);

		if (check_size && write_offset >= otp_size) {
			shell_error(sh, "Write exceeds OTP size");
			return -EINVAL;
		}

		if (check_size && otp_dev == otp_default_dev && OTP_LOCK_AVAILABLE) {
			if ((write_offset >= OTP_LOCK_OFFSET) &&
			    (write_offset < (OTP_LOCK_OFFSET + OTP_LOCK_LEN))) {
				shell_error(sh, "Refusing to write OTP lock bytes; use 'otp lock'");
				return -EPERM;
			}
		}

		ret = otp_program(otp_dev, (off_t)write_offset, &value, 1);
		if (ret < 0) {
			shell_error(sh, "OTP program failed at 0x%lx: %d",
				    (unsigned long)write_offset, ret);
			return ret;
		}
	}

	shell_info(sh, "OTP program OK (%u byte(s))", (unsigned int)(argc - 2));
	return 0;
}

static int cmd_otp_lock(const struct shell *sh, size_t argc, char **argv)
{
#if !OTP_LOCK_AVAILABLE
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_error(sh, "OTP lock bytes not defined in devicetree");
	return -ENOTSUP;
#else
	unsigned long index;
	uint8_t lock_byte = 0x00;
	int ret;

	ret = otp_check_ready(sh);
	if (ret != 0) {
		return ret;
	}

	if (otp_default_dev == NULL || otp_dev != otp_default_dev) {
		shell_error(sh, "OTP lock bytes only supported on the default device");
		return -ENOTSUP;
	}

	if (argc < 2) {
		shell_error(sh, "Usage: otp lock <index 0-15> [value]");
		return -EINVAL;
	}

	index = strtoul(argv[1], NULL, 0);
	if (index >= OTP_LOCK_LEN) {
		shell_error(sh, "Index out of range (0-%u)", (unsigned int)(OTP_LOCK_LEN - 1));
		return -EINVAL;
	}

	if (argc >= 3) {
		lock_byte = (uint8_t)strtoul(argv[2], NULL, 0);
	}

	ret = otp_program(otp_dev, (off_t)(OTP_LOCK_OFFSET + index), &lock_byte, 1);
	if (ret < 0) {
		shell_error(sh, "OTP lock write failed: %d", ret);
		return ret;
	}

	shell_info(sh, "OTP lock byte %lu programmed to 0x%02x", index, lock_byte);
	return 0;
#endif
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	otp_cmds,
	SHELL_CMD(device, NULL, "Get or set OTP device", cmd_otp_device),
	SHELL_CMD(dump, NULL, "Dump OTP contents in hex [len]", cmd_otp_dump),
	SHELL_CMD(program, NULL, "Program raw byte(s): otp program <offset> <byte> [byte ...]",
		  cmd_otp_program),
	SHELL_CMD(lock, NULL, "Program one OTP lock byte: otp lock <index> [value]", cmd_otp_lock),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(otp, &otp_cmds, "OTP commands", NULL);
