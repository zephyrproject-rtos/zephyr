/*
 * Copyright (c) 2026 Rodrigo Peixoto <rodrigopex@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#define DEV(nodelabel) DEVICE_DT_GET(DT_NODELABEL(nodelabel))

/* The power domain tree, drawn from pd_root down. Each branch string is padded
 * to a fixed width so the state column lines up; the box-drawing characters are
 * multi-byte, so the padding is measured in visible columns, not bytes.
 * pd_shared has two parents, hence the note.
 */
static const struct {
	const char *branch;
	const struct device *dev;
	const char *note;
} tree[] = {
	{ "pd_root           ", DEV(pd_root),   NULL },
	{ "├─ pd_a           ", DEV(pd_a),      NULL },
	{ "│  ├─ pd_a1       ", DEV(pd_a1),     NULL },
	{ "│  ├─ pd_a2       ", DEV(pd_a2),     NULL },
	{ "│  └─ pd_shared   ", DEV(pd_shared), "also powered by pd_b" },
	{ "│     └─ pd_deep  ", DEV(pd_deep),   NULL },
	{ "└─ pd_b           ", DEV(pd_b),      NULL },
	{ "   └─ pd_b1       ", DEV(pd_b1),     NULL },
};

static void print_tree(void)
{
	for (size_t i = 0U; i < ARRAY_SIZE(tree); i++) {
		enum pm_device_state state;

		(void)pm_device_state_get(tree[i].dev, &state);
		printk("%s(%s)", tree[i].branch, pm_device_state_str(state));
		if (tree[i].note != NULL) {
			printk("   [%s]", tree[i].note);
		}
		printk("\n");
	}
}

static void demo_get(const struct device *dev)
{
	printk("\n> get %s\n", dev->name);
	(void)pm_device_runtime_get(dev);
	print_tree();
}

static void demo_put(const struct device *dev)
{
	printk("\n> put %s\n", dev->name);
	(void)pm_device_runtime_put(dev);
	print_tree();
}

static void run_demo(void)
{
	printk("\n=== power domain tree demo ===\n");
	printk("initial state:\n");
	print_tree();

	/* Bring up dev_a: only its branch (pd_a1 -> pd_a -> pd_root) powers up. */
	demo_get(DEV(dev_a));

	/* dev_b needs two branches; pd_a and pd_root are already up. */
	demo_get(DEV(dev_b));

	/* dev_c reaches pd_root through the shared, two-parent pd_shared. */
	demo_get(DEV(dev_c));

	/* Releasing dev_a must not drop pd_a: dev_b still needs it. */
	demo_put(DEV(dev_a));

	/* Releasing dev_b keeps pd_a and pd_b up: pd_shared still needs both. */
	demo_put(DEV(dev_b));

	/* The last consumer goes away and the whole tree collapses. */
	demo_put(DEV(dev_c));

	printk("\n=== demo complete ===\n");
	printk("Use 'pd get <name>' / 'pd put <name>' / 'pd status' to explore.\n");
}

#ifdef CONFIG_SHELL
static int cmd_pd_get(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);

	if (dev == NULL) {
		shell_error(sh, "unknown device: %s", argv[1]);
		return -EINVAL;
	}

	(void)pm_device_runtime_get(dev);
	print_tree();

	return 0;
}

static int cmd_pd_put(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *dev = device_get_binding(argv[1]);

	if (dev == NULL) {
		shell_error(sh, "unknown device: %s", argv[1]);
		return -EINVAL;
	}

	(void)pm_device_runtime_put(dev);
	print_tree();

	return 0;
}

static int cmd_pd_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	print_tree();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(pd_cmds,
	SHELL_CMD_ARG(get, NULL, "Runtime-get a device: pd get <name>", cmd_pd_get, 2, 0),
	SHELL_CMD_ARG(put, NULL, "Runtime-put a device: pd put <name>", cmd_pd_put, 2, 0),
	SHELL_CMD(status, NULL, "Show the state of every power domain", cmd_pd_status),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(pd, &pd_cmds, "Power domain tree demo", NULL);
#endif /* CONFIG_SHELL */

int main(void)
{
	run_demo();

	return 0;
}
