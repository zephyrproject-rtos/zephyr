/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lis2dh

#include <zephyr/shell/shell.h>

#include "lis2dh.h"

static const struct device *lis2dh_shell_device(const char *name)
{
	if (name != NULL) {
		return device_get_binding(name);
	}

	return DEVICE_DT_INST_GET(0);
}

static int lis2dh_shell_read(const struct device *dev, uint8_t reg, uint8_t *value)
{
	struct lis2dh_data *data = dev->data;

	return data->hw_tf->read_reg(dev, reg, value);
}

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = lis2dh_shell_device(argc > 1 ? argv[1] : NULL);
	struct lis2dh_data *data;
	uint8_t ctrl1, ctrl5, fifo_ctrl, fifo_src;
	int status;

	if (dev == NULL) {
		shell_error(sh, "device not found");
		return -ENODEV;
	}
	data = dev->data;

	lis2dh_lock(dev);
	status = lis2dh_shell_read(dev, LIS2DH_REG_CTRL1, &ctrl1);
	if (status == 0) {
		status = lis2dh_shell_read(dev, LIS2DH_REG_CTRL5, &ctrl5);
	}
	if (status == 0) {
		status = lis2dh_shell_read(dev, LIS2DH_REG_FIFO_CTRL, &fifo_ctrl);
	}
	if (status == 0) {
		status = lis2dh_shell_read(dev, LIS2DH_REG_FIFO_SRC, &fifo_src);
	}
	if (status < 0) {
		lis2dh_unlock(dev);
		shell_error(sh, "bus error: %d", status);
		return status;
	}

	shell_print(sh, "device   : %s", dev->name);
	shell_print(sh, "state    : %s", lis2dh_fifo_is_active(dev) ? "streaming" : "idle");
	shell_print(sh, "odr      : 0x%x (%s)",
		    (unsigned int)((ctrl1 & LIS2DH_ODR_MASK) >> LIS2DH_ODR_SHIFT),
		    (ctrl1 & LIS2DH_LP_EN_BIT_MASK) != 0U ? "low-power" : "normal/high-res");
	shell_print(sh, "fifo_en  : %u", (ctrl5 & LIS2DH_EN_FIFO) != 0U);
	shell_print(sh, "fifo_ctrl: mode=0x%x fth=%u", (unsigned int)(fifo_ctrl >> 6),
		    (unsigned int)((fifo_ctrl & LIS2DH_FIFO_FTH_MASK) + 1U));
	shell_print(sh, "fifo_src : wtm=%u ovrn=%u empty=%u fss=%u",
		    (fifo_src & LIS2DH_FIFO_WTM) != 0U, (fifo_src & LIS2DH_FIFO_OVRN) != 0U,
		    (fifo_src & LIS2DH_FIFO_EMPTY) != 0U,
		    (unsigned int)(fifo_src & LIS2DH_FIFO_FSS_MASK));
	shell_print(sh, "counters : batches=%u frames=%u overruns=%u errors=%u",
		    data->fifo_stats.batches, data->fifo_stats.frames,
		    data->fifo_stats.overruns, data->fifo_stats.errors);
	lis2dh_unlock(dev);

	return 0;
}

static int cmd_regs(const struct shell *sh, size_t argc, char **argv)
{
	static const struct {
		uint8_t reg;
		const char *name;
	} regs[] = {
		{LIS2DH_REG_CTRL1, "CTRL1"},	 {LIS2DH_REG_CTRL2, "CTRL2"},
		{LIS2DH_REG_CTRL3, "CTRL3"},	 {LIS2DH_REG_CTRL4, "CTRL4"},
		{LIS2DH_REG_CTRL5, "CTRL5"},	 {LIS2DH_REG_FIFO_CTRL, "FIFO_CTRL"},
		{LIS2DH_REG_FIFO_SRC, "FIFO_SRC"}, {LIS2DH_REG_INT1_SRC, "INT1_SRC"},
		{LIS2DH_REG_WAI, "WHO_AM_I"},
	};
	const struct device *dev = lis2dh_shell_device(argc > 1 ? argv[1] : NULL);
	int status = 0;

	if (dev == NULL) {
		shell_error(sh, "device not found");
		return -ENODEV;
	}

	lis2dh_lock(dev);
	for (size_t i = 0U; i < ARRAY_SIZE(regs); i++) {
		uint8_t value;

		status = lis2dh_shell_read(dev, regs[i].reg, &value);
		if (status < 0) {
			break;
		}
		shell_print(sh, "%s (0x%02x) = 0x%02x", regs[i].name, regs[i].reg, value);
	}
	lis2dh_unlock(dev);

	if (status < 0) {
		shell_error(sh, "bus error: %d", status);
	}

	return status;
}

static int cmd_counters(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = lis2dh_shell_device(argc > 1 ? argv[1] : NULL);
	struct lis2dh_data *data;

	if (dev == NULL) {
		shell_error(sh, "device not found");
		return -ENODEV;
	}
	data = dev->data;

	lis2dh_lock(dev);
	shell_print(sh, "batches  : %u", data->fifo_stats.batches);
	shell_print(sh, "frames   : %u", data->fifo_stats.frames);
	shell_print(sh, "overruns : %u", data->fifo_stats.overruns);
	shell_print(sh, "errors   : %u", data->fifo_stats.errors);
	lis2dh_unlock(dev);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	lis2dh_cmds, SHELL_CMD_ARG(status, NULL, "FIFO state, ODR and counters [device]",
				   cmd_status, 1, 1),
	SHELL_CMD_ARG(regs, NULL, "Dump control, FIFO and interrupt registers [device]",
		      cmd_regs, 1, 1),
	SHELL_CMD_ARG(counters, NULL, "Show FIFO diagnostic counters [device]", cmd_counters, 1, 1));

SHELL_CMD_REGISTER(lis2dh, &lis2dh_cmds, "LIS2DH diagnostics", NULL);
