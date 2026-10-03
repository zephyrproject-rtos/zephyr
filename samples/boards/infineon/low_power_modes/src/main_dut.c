/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if defined(CONFIG_APP_ROLE_DUT)

#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include <soc.h>

#define MAIN_SLEEP_TIME_MS 2000

/* Idle window after which the displayed terminal mode is entered. */
#define TERMINAL_SELECT_TIMEOUT_MS 2000

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static struct gpio_callback button_cb_data;

/* Only a dedicated wake pad can wake from Hibernate, and it must be a digital
 * input for its buffer to reach the always-on wake detector.
 */
#define SW1_NODE DT_ALIAS(sw1)
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
static const struct gpio_dt_spec hib_wake = GPIO_DT_SPEC_GET(SW1_NODE, gpios);
#endif

/* Signalled from the button ISR; used to toggle the terminal mode selection. */
K_SEM_DEFINE(button_sem, 0, 1);

static uint32_t sleep_count;
static uint32_t deepsleep_count;
static uint32_t deepsleep_ram_count;

static void pm_notifier_entry(enum pm_state state)
{
	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		sleep_count++;
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		deepsleep_count++;
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		deepsleep_ram_count++;
		break;
	default:
		break;
	}
}

static struct pm_notifier pm_notif = {
	.state_entry = pm_notifier_entry,
};

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	k_sem_give(&button_sem);
}

static void print_pm_counts(void)
{
	printk("Sleep: %u | Deepsleep: %u | Deepsleep-RAM: %u\n", sleep_count, deepsleep_count,
	       deepsleep_ram_count);
}

/* PM entry count for a deep state, used to confirm it actually engaged. */
static uint32_t pm_state_entry_count(enum pm_state state)
{
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		return deepsleep_count;
	case PM_STATE_SUSPEND_TO_RAM:
		return deepsleep_ram_count;
	default:
		return 0;
	}
}

/* Run one retained/warm-boot mode (DeepSleep or DeepSleep-RAM): unlock its PM
 * policy state, sleep once, then restore the lock so the next mode starts from
 * a known state.
 */
static void run_retained_mode_test(enum pm_state test_state, const char *name)
{
	uint32_t entries_before = pm_state_entry_count(test_state);
	uint32_t t0;
	uint32_t t1;

	printk("Phase 2: exercising %s\n", name);

	pm_policy_state_lock_put(test_state, PM_ALL_SUBSTATES);

	t0 = k_uptime_get_32();
	k_msleep(MAIN_SLEEP_TIME_MS);
	t1 = k_uptime_get_32();

	/* The SoC's resume path has already rebuilt the console UART. */
	printk("Phase 2: woke after %u ms\n", t1 - t0);
	print_pm_counts();

	/* An unchanged count means the policy fell back to a shallower state. */
	if (pm_state_entry_count(test_state) == entries_before) {
		printk("Phase 2: WARNING - %s did not engage\n", name);
	}

	pm_policy_state_lock_get(test_state, PM_ALL_SUBSTATES);
	k_msleep(MAIN_SLEEP_TIME_MS);
}

#if defined(CONFIG_APP_DEEP_MODES)

/* Terminal power-down mode, toggled by the button. */
static bool terminal_is_hibernate;

static const char *terminal_mode_name(void)
{
	return terminal_is_hibernate ? "Hibernate" : "DeepSleep-OFF";
}

/* Powers the CPU domain down; wake is a cold boot, so this does not return on
 * success.
 */
static void run_ds_off_mode(void)
{
	printk("Phase 2: entering DeepSleep-OFF. Press the button to wake/reset.\n");

	pm_state_set(PM_STATE_SOFT_OFF, 1);

	__enable_irq();
	printk("Phase 2: DeepSleep-OFF did not engage\n");
}

/* Powers the whole chip down. The SoC arms the wake source from the devicetree
 * hibernate-wakeup node, so only the pad itself is configured here.
 */
static void run_hibernate_mode(void)
{
	printk("Phase 2: entering hibernate. Press the wake button to wake/reset.\n");

#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	if (gpio_is_ready_dt(&hib_wake) && (gpio_pin_configure_dt(&hib_wake, GPIO_INPUT) != 0)) {
		printk("Error: failed to configure hibernate wake pin %d\n", hib_wake.pin);
	}
#endif

	pm_state_set(PM_STATE_SOFT_OFF, 0);

	__enable_irq();
	printk("Phase 2: Hibernate did not engage\n");
}

/* Each button press toggles the terminal mode; the displayed one is entered
 * after TERMINAL_SELECT_TIMEOUT_MS without a press, which ends the run.
 */
static void run_terminal_mode(void)
{
	printk("Phase 2: press the button to toggle the terminal mode "
	       "(entered after %d ms idle)\n",
	       TERMINAL_SELECT_TIMEOUT_MS);
	printk("Phase 2: terminal mode = %s\n", terminal_mode_name());

	/* Ignore any press latched during the earlier tests. */
	k_sem_reset(&button_sem);

	while (k_sem_take(&button_sem, K_MSEC(TERMINAL_SELECT_TIMEOUT_MS)) == 0) {
		terminal_is_hibernate = !terminal_is_hibernate;
		printk("Phase 2: terminal mode = %s\n", terminal_mode_name());
	}

	if (terminal_is_hibernate) {
		run_hibernate_mode();
	} else {
		run_ds_off_mode();
	}
}

#endif /* CONFIG_APP_DEEP_MODES */

static void test_pm(void)
{
	int ret;

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return;
	}
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
		       button.pin);
		return;
	}
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
		       button.port->name, button.pin);
		return;
	}
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	pm_notifier_register(&pm_notif);

	/* Phase 1: runtime-idle only (WFI, clocks running); DeepSleep is locked out. */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);

	printk("Phase 1: runtime-idle only\n");
	k_msleep(MAIN_SLEEP_TIME_MS);
	print_pm_counts();

	printk("Phase 2: run each low-power mode in sequence\n");

	/* DeepSleep and DeepSleep-RAM return in-session, so run them back to back. */
	run_retained_mode_test(PM_STATE_SUSPEND_TO_IDLE, "DeepSleep");

#if defined(CONFIG_APP_DEEP_MODES)
	run_retained_mode_test(PM_STATE_SUSPEND_TO_RAM, "DeepSleep-RAM");

	/* The terminal mode powers the SoC down and ends the run. */
	run_terminal_mode();
#else
	printk("Phase 2: DeepSleep-RAM and terminal power-down skipped "
	       "(regular DeepSleep only)\n");
#endif /* CONFIG_APP_DEEP_MODES */

	printk("Sequence complete\n");
}

/* DUT entry, invoked from the worker thread in main.c. */
void app_main(void)
{
	test_pm();
}

#endif /* CONFIG_APP_ROLE_DUT */
