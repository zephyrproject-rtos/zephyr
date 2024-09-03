/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/shell/shell.h>

#define LVL_INTR_OFFSET(lvl, idx) CONCAT(CONFIG_, CONCAT(Z_STR_L, lvl), _LVL_INTR_0, idx, _OFFSET)
#define PRINT_LVL_INTR_OFFSET(idx, lvl)                                                            \
	shell_print(sh, "L%d aggregator %d IRQ: %d", lvl, idx, LVL_INTR_OFFSET(lvl, idx))

static int cmd_irq_info(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argv);
	ARG_UNUSED(argc);

	shell_print(sh, "# of IRQs: %d\n", CONFIG_NUM_IRQS);

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
		shell_print(sh, "Interrupt level: %d",
			    IS_ENABLED(CONFIG_3RD_LEVEL_INTERRUPTS) ? 3 : 2);
		shell_print(sh, "Max IRQs per aggregator: %d", CONFIG_MAX_IRQ_PER_AGGREGATOR);
		shell_print(sh, "\n# of L%d aggregator(s): %d", 2, CONFIG_NUM_2ND_LEVEL_AGGREGATORS);
		LISTIFY(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, PRINT_LVL_INTR_OFFSET, (;), 2);

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
		shell_print(sh, "\n# of L%d aggregator(s): %d", 3, CONFIG_NUM_3RD_LEVEL_AGGREGATORS);
		LISTIFY(CONFIG_NUM_3RD_LEVEL_AGGREGATORS, PRINT_LVL_INTR_OFFSET, (;), 3);
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

		shell_print(sh, "");
#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */

	return 0;
}

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
static int cmd_irq_encode(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	uint32_t level = argc - 1, l1_irq, l2_irq = 0, l3_irq = 0, irq;

	shell_print(sh, "Encoding a level %d IRQ", level);

	l1_irq = shell_strtoul(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse %s %s (err %d)", "L1 IRQ", argv[1], err);
		return -EINVAL;
	}

	shell_print(sh, "Level 1: %d", l1_irq);

	if (level >= 2) {
		l2_irq = shell_strtoul(argv[2], 10, &err);
		if (err != 0) {
			shell_error(sh, "Unable to parse %s %s (err %d)", "L2 IRQ", argv[2], err);
			return -EINVAL;
		}
		shell_print(sh, "Level 2: %d", l2_irq);
		l2_irq = irq_to_level_2(l2_irq);
	}

	if (level == 3) {
		l3_irq = shell_strtoul(argv[3], 10, &err);
		if (err != 0) {
			shell_error(sh, "Unable to parse %s %s (err %d)", "L3 IRQ", argv[3], err);
			return -EINVAL;
		}
		shell_print(sh, "Level 3: %d", l3_irq);
		l3_irq = irq_to_level_3(l3_irq);
	}

	irq = l3_irq | l2_irq | l1_irq;

	shell_print(sh, "Encoded IRQ: %d (0x%X)", irq, irq);

	return 0;
}
#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */

static int cmd_irq_enable(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	int err = 0;
	uint32_t irq;

	irq = shell_strtoul(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse %s %s (err %d)", "irq", argv[1], err);
		return -EINVAL;
	}

	irq_enable(irq);

	return 0;
}

static int cmd_irq_disable(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	int err = 0;
	uint32_t irq;

	irq = shell_strtoul(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse %s %s (err %d)", "irq", argv[1], err);
		return -EINVAL;
	}

	irq_disable(irq);

	return 0;
}

static int cmd_irq_is_enabled(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	int err = 0;
	uint32_t irq;

	irq = shell_strtoul(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse %s %s (err %d)", "irq", argv[1], err);
		return -EINVAL;
	}

	err = irq_is_enabled(irq);

	shell_print(sh, "IRQ %d is %s", irq, err == 0 ? "disabled" : "enabled");

	return 0;
}

static int cmd_irq_set_affinity(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	int err = 0;
	uint32_t irq, mask;

	irq = shell_strtoul(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse %s %s (err %d)", "irq", argv[1], err);
		return -EINVAL;
	}

	mask = shell_strtoul(argv[2], 16, &err);
	if (err != 0) {
		shell_error(sh, "Unable to parse %s %s (err %d)", "mask", argv[2], err);
		return -EINVAL;
	}

	irq_set_affinity(irq, mask);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(irq_sub_cmds,
	SHELL_CMD(info, NULL, "IRQ info", cmd_irq_info),
#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS
	SHELL_CMD_ARG(encode, NULL,
		      "Encode an multilevel IRQ\n"
		      "Usage: irq encode <level 1 IRQ> [level 2 IRQ] [level 3 IRQ]",
		      cmd_irq_encode, 2, 2),
#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
	SHELL_CMD_ARG(enable, NULL, "Enable an IRQ", cmd_irq_enable, 2, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable an IRQ", cmd_irq_disable, 2, 0),
	SHELL_CMD_ARG(is_enabled, NULL, "Check if an IRQ is enabled", cmd_irq_is_enabled, 2, 0),
	SHELL_CMD_ARG(set_affinity, NULL, "Configure the affinity of an IRQ", cmd_irq_set_affinity,
		      3, 0),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(irq, &irq_sub_cmds, "IRQ shell commands", NULL);
