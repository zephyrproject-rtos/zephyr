/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/state.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>

#include <fsl_mu.h>

#define RT700_PM_MU_REG kMU_MsgReg0

#define RT700_PM_MSG_CMD_ACTIVE      0x43504d00U
#define RT700_PM_MSG_CMD_SLEEP       0x43504d01U
#define RT700_PM_MSG_CMD_DEEP_SLEEP  0x43504d02U
#define RT700_PM_MSG_CMD_GO          0x43504d10U
#define RT700_PM_MSG_ACK_ACTIVE      0x41504d00U
#define RT700_PM_MSG_ACK_SLEEP       0x41504d01U
#define RT700_PM_MSG_ACK_DEEP_SLEEP  0x41504d02U
#define RT700_PM_MSG_DONE_ACTIVE     0x44504d00U
#define RT700_PM_MSG_DONE_SLEEP      0x44504d01U
#define RT700_PM_MSG_DONE_DEEP_SLEEP 0x44504d02U

enum rt700_pm_mode {
	RT700_PM_MODE_ACTIVE,
	RT700_PM_MODE_SLEEP,
	RT700_PM_MODE_DEEP_SLEEP,
};

static atomic_t state_entry_count[PM_STATE_COUNT];
static atomic_t state_exit_count[PM_STATE_COUNT];

static void pm_state_entry(enum pm_state state)
{
	if (state < PM_STATE_COUNT) {
		atomic_inc(&state_entry_count[state]);
	}
}

static void pm_state_exit(enum pm_state state)
{
	if (state < PM_STATE_COUNT) {
		atomic_inc(&state_exit_count[state]);
	}
}

static struct pm_notifier pm_events = {
	.state_entry = pm_state_entry,
	.state_exit = pm_state_exit,
};

static void print_pm_counts(void)
{
	printk("CPU1 PM counts: runtime-idle=%ld/%ld suspend-to-idle=%ld/%ld\n",
	       atomic_get(&state_entry_count[PM_STATE_RUNTIME_IDLE]),
	       atomic_get(&state_exit_count[PM_STATE_RUNTIME_IDLE]),
	       atomic_get(&state_entry_count[PM_STATE_SUSPEND_TO_IDLE]),
	       atomic_get(&state_exit_count[PM_STATE_SUSPEND_TO_IDLE]));
}

static enum rt700_pm_mode cmd_to_mode(uint32_t cmd)
{
	switch (cmd) {
	case RT700_PM_MSG_CMD_ACTIVE:
		return RT700_PM_MODE_ACTIVE;
	case RT700_PM_MSG_CMD_SLEEP:
		return RT700_PM_MODE_SLEEP;
	case RT700_PM_MSG_CMD_DEEP_SLEEP:
		return RT700_PM_MODE_DEEP_SLEEP;
	default:
		return RT700_PM_MODE_ACTIVE;
	}
}

static uint32_t mode_to_ack(enum rt700_pm_mode mode)
{
	switch (mode) {
	case RT700_PM_MODE_ACTIVE:
		return RT700_PM_MSG_ACK_ACTIVE;
	case RT700_PM_MODE_SLEEP:
		return RT700_PM_MSG_ACK_SLEEP;
	case RT700_PM_MODE_DEEP_SLEEP:
		return RT700_PM_MSG_ACK_DEEP_SLEEP;
	default:
		return RT700_PM_MSG_ACK_ACTIVE;
	}
}

static uint32_t mode_to_done(enum rt700_pm_mode mode)
{
	switch (mode) {
	case RT700_PM_MODE_ACTIVE:
		return RT700_PM_MSG_DONE_ACTIVE;
	case RT700_PM_MODE_SLEEP:
		return RT700_PM_MSG_DONE_SLEEP;
	case RT700_PM_MODE_DEEP_SLEEP:
		return RT700_PM_MSG_DONE_DEEP_SLEEP;
	default:
		return RT700_PM_MSG_DONE_ACTIVE;
	}
}

static const char *mode_to_str(enum rt700_pm_mode mode)
{
	switch (mode) {
	case RT700_PM_MODE_ACTIVE:
		return "active";
	case RT700_PM_MODE_SLEEP:
		return "sleep";
	case RT700_PM_MODE_DEEP_SLEEP:
		return "deep-sleep";
	default:
		return "unknown";
	}
}

static uint32_t receive_cpu0_msg(void)
{
	while ((MU_GetStatusFlags(MU1_MUB) & kMU_Rx0FullFlag) == 0U) {
		k_yield();
	}

	return MU_ReceiveMsgNonBlocking(MU1_MUB, RT700_PM_MU_REG);
}

static void send_cpu0_msg(uint32_t msg)
{
	(void)MU_SendMsg(MU1_MUB, RT700_PM_MU_REG, msg);
}

static void run_window(enum rt700_pm_mode mode)
{
	switch (mode) {
	case RT700_PM_MODE_ACTIVE:
		/* Busy-wait the whole window so the core stays fully active. */
		for (int i = 0; i < CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SLEEP_SECONDS; i++) {
			k_busy_wait(USEC_PER_SEC);
		}
		break;
	case RT700_PM_MODE_SLEEP:
		/*
		 * Suspend-to-idle is locked out at startup, so the idle thread
		 * stays in runtime-idle for the whole window instead of letting
		 * the default policy slide into the deeper state.
		 */
		k_sleep(K_SECONDS(CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SLEEP_SECONDS));
		break;
	case RT700_PM_MODE_DEEP_SLEEP:
		/* Temporarily allow suspend-to-idle for this window only. */
		pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		k_sleep(K_SECONDS(CONFIG_SAMPLE_RT700_DUAL_CORE_PM_SLEEP_SECONDS));
		pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
		break;
	default:
		break;
	}
}

int main(void)
{
	printk("RT700 dual-core PM sample: CPU1 started on %s\n", CONFIG_BOARD_TARGET);
	printk("CPU1: waiting for CPU0 PM test commands\n");

	pm_notifier_register(&pm_events);

	/* Lock suspend-to-idle by default so sleep windows stay in runtime-idle.
	 * Deep-sleep windows unlock it explicitly.
	 */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);

	while (true) {
		uint32_t cmd = receive_cpu0_msg();
		enum rt700_pm_mode mode = cmd_to_mode(cmd);

		printk("CPU1: prepared for %s window\n", mode_to_str(mode));
		send_cpu0_msg(mode_to_ack(mode));

		cmd = receive_cpu0_msg();
		if (cmd != RT700_PM_MSG_CMD_GO) {
			printk("CPU1: unexpected GO command 0x%08x\n", cmd);
			continue;
		}

		printk("CPU1: entering %s window\n", mode_to_str(mode));
		run_window(mode);
		printk("CPU1: %s window complete\n", mode_to_str(mode));
		print_pm_counts();
		send_cpu0_msg(mode_to_done(mode));
	}
}
