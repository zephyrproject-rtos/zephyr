/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

static int cmd_net_suspend(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_POWER_MANAGEMENT)
	if (argv[1]) {
		struct net_if *iface = NULL;
		const struct device *dev;
		int idx;
		int ret;

		idx = get_iface_idx(sh, argv[1]);
		if (idx < 0) {
			return -ENOEXEC;
		}

		iface = net_if_get_by_index(idx);
		if (!iface) {
			PR_WARNING("No such interface in index %d\n", idx);
			return -ENOEXEC;
		}

		dev = net_if_get_device(iface);

		ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
		if (ret != 0) {
			PR_INFO("Iface could not be suspended: ");

			if (ret == -EBUSY) {
				PR_INFO("device is busy\n");
			} else if (ret == -EALREADY) {
				PR_INFO("dehive is already suspended\n");
			}
		}
	} else {
		PR("Usage:\n");
		PR("\tsuspend <iface index>\n");
	}
#else
	PR_INFO("You need a network driver supporting Power Management.\n");
#endif /* CONFIG_NET_POWER_MANAGEMENT */

	return 0;
}

SHELL_SUBCMD_ADD((net), suspend, NULL,
		 "Suspend a network interface",
		 cmd_net_suspend, 1, 0);
