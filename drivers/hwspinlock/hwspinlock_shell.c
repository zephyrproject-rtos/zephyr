/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/device.h>
#include <zephyr/drivers/hwspinlock.h>
#include <zephyr/sys/util.h>

#include <errno.h>

struct hwspinlock_shell_state {
	const struct device *dev;
	uint32_t id;
	bool holding;
};

static struct hwspinlock_shell_state st;

static inline const struct hwspinlock_driver_api *hwspinlock_api(const struct device *dev)
{
	return DEVICE_API_GET(hwspinlock, dev);
}

static int cmd_hwspinlock_select(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	const struct device *dev = device_get_binding(argv[1]);
	uint32_t id;
	int err = 0;

	if (dev == NULL) {
		shell_error(sh, "device not found: %s", argv[1]);
		return -ENODEV;
	}

	uint32_t max = hw_spinlock_get_max_id(dev);
	unsigned long id_ul = shell_strtoul(argv[2], 0, &err);

	if (err != 0) {
		shell_error(sh, "invalid id: %s (%d)", argv[2], err);
		return -EINVAL;
	}

	if (id_ul > UINT32_MAX) {
		shell_error(sh, "id too large: %s", argv[2]);
		return -ERANGE;
	}

	id = (uint32_t)id_ul;

	if (id > max) {
		shell_error(sh, "id out of range (max=%u)", max);
		return -ERANGE;
	}

	if (st.holding) {
		shell_warn(sh, "was holding %s id=%u; unlocking",
				st.dev ? st.dev->name : "(null)", st.id);
		hwspinlock_api(st.dev)->unlock(st.dev, st.id);
		st.holding = false;
	}

	st.dev = dev;
	st.id = id;

	shell_print(sh, "selected %s id=%u", st.dev->name, st.id);
	shell_print(sh, "tip: use 'hwspinlock status' or 'hwspinlock trylock'");

	return 0;
}

static int cmd_hwspinlock_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (st.dev == NULL) {
		shell_error(sh, "no selection; use: hwspinlock select <dev> <id>");
		return -EINVAL;
	}

	int ret = hwspinlock_api(st.dev)->trylock(st.dev, st.id);

	if (st.holding) {
		shell_print(sh, "status: locked (held locally)");
		return 0;
	}

	if (ret == 0) {
		shell_print(sh, "status: unlocked");
		/* release immediately */
		hwspinlock_api(st.dev)->unlock(st.dev, st.id);
	} else if (ret == -EBUSY) {
		shell_print(sh, "status: locked by remote");
	} else {
		shell_print(sh, "status: error (%d)", ret);
	}

	return 0;
}

static int cmd_hwspinlock_trylock(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (st.dev == NULL) {
		shell_error(sh, "no selection; use: hwspinlock select <dev> <id>");
		return -EINVAL;
	}

	int ret = hwspinlock_api(st.dev)->trylock(st.dev, st.id);

	if (st.holding) {
		shell_warn(sh, "already holding");
		return 0;
	}

	if (ret == 0) {
		st.holding = true;
		shell_print(sh, "trylock ok");
	} else {
		shell_print(sh, "trylock ret=%d", ret);
	}

	return 0;
}

static int cmd_hwspinlock_lock(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret;
	bool locked_by_remote = false;

	if (st.dev == NULL) {
		shell_error(sh, "no selection; use: hwspinlock select <dev> <id>");
		return -EINVAL;
	}

	if (st.holding) {
		shell_warn(sh, "already holding by local");
		return 0;
	}

	ret = hwspinlock_api(st.dev)->trylock(st.dev, st.id);
	if (ret < 0) {
		locked_by_remote = true;
		shell_print(sh, "status: locked by remote");
	}

	shell_print(sh, "locking...");
	hwspinlock_api(st.dev)->lock(st.dev, st.id);

	if (locked_by_remote) {
		/* If remote have already hold spinlock before local lock it,
		 * local is blocking until unlocked by remote.
		 * Added local holding judgement in there.
		 */
		st.holding = false;
	} else {
		st.holding = true;
	}

	shell_print(sh, "locked");

	return 0;
}

static int cmd_hwspinlock_unlock(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (st.dev == NULL) {
		shell_error(sh, "no selection; use: hwspinlock select <dev> <id>");
		return -EINVAL;
	}

	if (!st.holding) {
		shell_warn(sh, "not holding");
		return 0;
	}

	hwspinlock_api(st.dev)->unlock(st.dev, st.id);
	st.holding = false;
	shell_print(sh, "unlocked");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_hwspinlock,
	SHELL_CMD_ARG(select, NULL, "Select controller and id: select <dev> <id>",
		      cmd_hwspinlock_select, 3, 0),
	SHELL_CMD(status, NULL, "Show lock status (local/remote)",
		cmd_hwspinlock_status),
	SHELL_CMD(trylock, NULL, "Try to acquire selected lock (non-blocking)",
		cmd_hwspinlock_trylock),
	SHELL_CMD(lock, NULL, "Acquire selected lock (blocking)",
		cmd_hwspinlock_lock),
	SHELL_CMD(unlock, NULL, "Release selected lock",
		cmd_hwspinlock_unlock),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(hwspinlock, &sub_hwspinlock, "HW spinlock commands", NULL);
