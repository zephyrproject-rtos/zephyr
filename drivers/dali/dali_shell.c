/*
 * Copyright (c) 2026 by Markus Becker <markushx@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/dali.h>
#include <zephyr/device.h>

struct private_data {
	struct k_sem semaphore;
	const struct shell *sh;
};

static void transmit_callback(const struct device *dev, int error, void *user_data)
{
	struct private_data *priv = (struct private_data *)user_data;
	const struct shell *sh = priv->sh;

	if (error == 0) {
		shell_print(sh, "%s: success on %s", __func__, dev->name);
	} else if (error == -ECOMM) {
		shell_warn(sh, "%s: failed on %s (Collision or Bus Fault)", __func__, dev->name);
	} else {
		shell_error(sh, "%s: failed on %s with error %d", __func__, dev->name, error);
	}
	k_sem_give(&(priv->semaphore));
}

/**
 * @brief Shell command handler for 'dali transmit'
 * * Syntax: dali transmit <device_name> <16bit_frame_hex>
 */
static int cmd_dali_transmit(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev;
	uint32_t frame_data_raw;
	int ret;
	struct private_data priv;

	struct dali_frame frame = {0};

	if (argc != 3) {
		shell_error(sh, "Usage: dali transmit <device_name> <16bit_frame_hex>");
		return -EINVAL;
	}

	const char *dev_name = argv[1];

	dev = shell_device_get_binding(dev_name);
	if (dev == NULL) {
		shell_error(sh, "DALI device %s not found.", dev_name);
		return -ENOENT;
	}

	if (!device_is_ready(dev)) {
		shell_error(sh, "DALI device %s is not ready.", dev_name);
		return -ENODEV;
	}

	frame_data_raw = shell_strtoul(argv[2], 16, &ret);
	if (ret < 0) {
		shell_error(sh, "Invalid hexadecimal value for frame data (err %d)", ret);
		return ret;
	}

	if (frame_data_raw > 0xFFFFU) {
		shell_error(sh, "Frame data must be a 16-bit hexadecimal value (0x0000..0xFFFF)");
		return -EINVAL;
	}

	frame.event_type = DALI_FRAME_GEAR;
	frame.data = frame_data_raw;

	k_sem_init(&(priv.semaphore), 0, 1);
	priv.sh = sh;

	ret = dali_transmit(dev, &frame, transmit_callback, (void *)&priv);

	if (ret < 0) {
		shell_error(sh, "dali send failed: %d", ret);
		return ret;
	}

	k_sem_take(&(priv.semaphore), K_FOREVER);

	return ret;
}

static void device_name_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dsub_device_name, device_name_get);

static bool device_is_dali(const struct device *dev)
{
	return DEVICE_API_IS(dali, dev);
}

static void device_name_get(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_filter(idx, device_is_dali);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dali,
			       SHELL_CMD_ARG(transmit, &dsub_device_name,
					     SHELL_HELP("Transmit a DALI 16-bit frame.",
							"<device_name> <16bit_frame_hex>"),
					     cmd_dali_transmit, 3, 0),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_ARG_REGISTER(dali, &sub_dali, "DALI commands", NULL, 2, 0);
