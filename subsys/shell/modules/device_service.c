/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <init.h>
#include <string.h>
#include <device.h>

extern struct device __device_init_start[];
extern struct device __device_PRE_KERNEL_1_start[];
extern struct device __device_PRE_KERNEL_2_start[];
extern struct device __device_POST_KERNEL_start[];
extern struct device __device_APPLICATION_start[];
extern struct device __device_init_end[];

static const char *device_get_config_level(struct device *dev)
{
	const char *level = NULL;

	__ASSERT_NO_MSG(dev >= __device_init_start && dev <  __device_init_end);

	if (dev >= __device_PRE_KERNEL_1_start) {
		level = "PRE_KERNEL_1";
	}

	if (dev >= __device_PRE_KERNEL_2_start) {
		level = "PRE_KERNEL_2";
	}

	if (dev >= __device_POST_KERNEL_start) {
		level = "POST_KERNEL";
	}

	if (dev >= __device_APPLICATION_start) {
		level = "APPLICATION";
	}

	return level;
}

static int cmd_device_list(const struct shell *shell, size_t argc, char **argv)
{
	struct device *dev;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL, "  Device\t Init Level\t Name\n"
			     "------------------------------------------\n");

	for (dev = __device_init_start; dev != __device_init_end; dev++) {
		if (dev->driver_api == NULL) {
			continue;
		}

		shell_fprintf(shell, SHELL_NORMAL, "0x%08X\t%s\t%s\n",
			      dev, device_get_config_level(dev),
			      dev->config->name);
	}

	return 0;
}

#if CONFIG_DEVICE_HIERARCHY
static void device_print_subtree(const struct shell *shell,
			       const char *name,
			       unsigned int level,
			       unsigned int last)
{
	struct device *dev, *curr, *next;
	int i;

	for (i = level - 1; i >= 0; i--) {
		char *prefix;

		if (i) {
			prefix = (last & (1 << i)) ? "    " : "|   ";
		} else {
			prefix = "+-- ";
		}

		shell_fprintf(shell, SHELL_NORMAL, prefix);
	}

	shell_fprintf(shell, SHELL_NORMAL, "%-48.48s\n", name);

	for (dev = __device_init_start, curr = NULL, next = NULL;
					dev != __device_init_end; dev++) {
		if (strcmp(name, dev->config->parent_name) != 0) {
			continue;
		}

		curr = next;
		next = dev;

		if (!curr) {
			continue;
		}

		device_print_subtree(shell, curr->config->name,
				     level + 1, (last << 1) | 0);
	}

	if (next) {
		device_print_subtree(shell, next->config->name,
				     level + 1, (last << 1) | 1);
	}
}

static int cmd_device_tree(const struct shell *shell, size_t argc, char **argv)
{
	device_print_subtree(shell, "SOC", 0, 1);
	return 0;
}
#endif /* CONFIG_DEVICE_HIERARCHY */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_device,
	/* Alphabetically sorted. */
	SHELL_CMD(list, NULL, "List configured devices", cmd_device_list),
#if CONFIG_DEVICE_HIERARCHY
	SHELL_CMD(tree, NULL, "Print device tree",	 cmd_device_tree),
#endif
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(device, &sub_device, "Device commands", NULL);
