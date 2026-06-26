/*
 * Copyright (c) 2026 Rithic Chellaram Hariharan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/printk.h>

#ifdef CONFIG_APP_SIM_PM_BACKEND
/* Simulated PM backend for native_sim (real SoCs provide these). */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	printk("  sim: SoC entering %s\n", pm_state_to_str(state));
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	irq_unlock(0);
}

/* Only enter explicitly requested states. */
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	ARG_UNUSED(cpu);
	ARG_UNUSED(ticks);

	return NULL;
}
#endif /* CONFIG_APP_SIM_PM_BACKEND */

static void try_light_sleep(void)
{
	int ret;

	printk("Requesting light sleep for 500 ms...\n");

	ret = pm_light_sleep(K_MSEC(500));
	switch (ret) {
	case 0:
		printk("Resumed from light sleep\n");
		break;
	case -ENOTSUP:
		printk("Light sleep not supported on this platform\n");
		break;
	default:
		printk("Light sleep failed (%d)\n", ret);
		break;
	}
}

static void try_deep_sleep(void)
{
	int ret;

	printk("Requesting deep sleep for 1 s...\n");

	ret = pm_deep_sleep(K_SECONDS(1));
	switch (ret) {
	case 0:
		printk("Resumed from deep sleep\n");
		break;
	case -ENOTSUP:
		printk("Deep sleep not supported on this platform\n");
		break;
	default:
		printk("Deep sleep failed (%d)\n", ret);
		break;
	}
}

static void try_soft_off(void)
{
	int ret;

	printk("Requesting soft-off (context not preserved)...\n");

	/* On real hardware soft-off resets on wake-up and never returns; the
	 * simulated native_sim backend returns here.
	 */
	ret = pm_soft_off(K_SECONDS(1));
	if (ret == -ENOTSUP) {
		printk("Soft-off not supported on this platform\n");
	} else {
		printk("Resumed from soft-off (simulated)\n");
	}
}

int main(void)
{
	printk("PM unified sleep sample\n");

	try_light_sleep();
	try_deep_sleep();
	try_soft_off();

	printk("PM unified sleep sample complete\n");

	return 0;
}
