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
#include <zephyr/sys/poweroff.h>

#include <soc.h>

#include "peripherals.h"

#define MAIN_SLEEP_TIME_MS 2000

#define PM_MODE_DS     0
#define PM_MODE_DS_RAM 1
#define PM_MODE_DS_OFF 2
#define PM_MODE_HIB    3

/* Idle window after which the displayed terminal mode is entered; a button
 * press toggles between DeepSleep-OFF and hibernate (see run_terminal_mode()).
 */
#define TERMINAL_SELECT_TIMEOUT_MS 2000

/* CONFIG_APP_DEEP_MODES gates the Phase-2 deep modes (DeepSleep-RAM and the
 * terminal power-down modes); leave it off on targets that implement regular
 * DeepSleep only.
 */

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static struct gpio_callback button_cb_data;

/* Dedicated Hibernate wake pad (sw1 alias). Only dedicated wake pads can wake
 * from Hibernate, and the pad must be a digital input so its buffer feeds the
 * always-on wake detector. The SoC selects the wake source; the pad is
 * configured here via the normal GPIO driver.
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
	printk("Sleep: %u | Deepsleep: %u\n", sleep_count, deepsleep_count);
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
 * policy state, sleep once, re-test every peripheral after wake, then restore
 * the lock so the next mode starts from a known state.
 */
static void run_retained_mode_test(enum pm_state test_state, const char *name)
{
	printk("Phase 2: exercising %s\n", name);

	pm_policy_state_lock_put(test_state, PM_ALL_SUBSTATES);

	uint32_t cnt_before = peripherals_counter_read();
	uint32_t entries_before = pm_state_entry_count(test_state);

	printk("Phase 2: enter %s (counter=%u, watch blue LED freeze)\n", name, cnt_before);

	uint32_t t0 = k_uptime_get_32();

	k_msleep(MAIN_SLEEP_TIME_MS);

	uint32_t t1 = k_uptime_get_32();

	/*
	 * No manual rebuild needed: the SoC restores every power-cycled peripheral
	 * (including the console UART) from the DeepSleep-RAM resume path before
	 * control returns here.
	 */
	printk("Phase 2: woke after %u ms\n", t1 - t0);
	print_pm_counts();

	/* Confirm the state engaged; on a platform that only supports a shallower
	 * mode the policy falls back and the entry count does not advance.
	 */
	if (pm_state_entry_count(test_state) == entries_before) {
		printk("Phase 2: WARNING - %s did not engage; platform fell back to a "
		       "shallower state\n",
		       name);
	}

	/* Re-test every present peripheral now that the clocks are back. */
	peripherals_test_after_wake(name);

	/* Re-lock the mode (HF clocks stay on) and hold. The blue LED blinking here
	 * confirms the PWM channel was rebuilt after wake. Leave the lock held so
	 * the next mode starts from a known state.
	 */
	pm_policy_state_lock_get(test_state, PM_ALL_SUBSTATES);
	printk("Phase 2: watch blue LED blink (PWM resumed via pm_action)\n");
	k_msleep(MAIN_SLEEP_TIME_MS);
}

#if defined(CONFIG_APP_DEEP_MODES)

/* Runtime-selected terminal power-down mode; toggled by the button. */
static uint32_t terminal_pm_mode = PM_MODE_DS_OFF;

static const char *terminal_mode_name(uint32_t mode)
{
	return (mode == PM_MODE_HIB) ? "Hibernate" : "DeepSleep-OFF";
}

/* Terminal mode: DeepSleep-OFF. Powers the CPU domain down; wake is a cold
 * boot, so this does not return on success.
 */
static void run_ds_off_mode(void)
{
	printk("Phase 2: exercising DeepSleep-OFF\n");

	printk("Phase 2: entering DeepSleep-OFF. Press the button to wake/reset.\n");

	pm_state_set(PM_STATE_SOFT_OFF, 1);

	__enable_irq();
	printk("Phase 2: DeepSleep-OFF did not engage\n");
}

/* Terminal mode: hibernate. Powers the whole chip down; wake is a button cold
 * boot, so this does not return. The active-low wake source is armed by the SoC
 * from the devicetree hibernate-wakeup node; the app only configures the pad as
 * a GPIO input.
 */
static void run_hibernate_mode(void)
{
	int ret;

	printk("Phase 2: exercising Hibernate\n");

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

	printk("Phase 2: entering hibernate. Press the wake button to wake/reset.\n");

	/* Configure the wake pad as a digital input so its pull-up holds it inactive
	 * and its buffer feeds the wake detector. Any second hibernate pad that is
	 * unconnected on the board is left unarmed (it would self-wake).
	 */
#if DT_NODE_HAS_STATUS(SW1_NODE, okay)
	if (gpio_is_ready_dt(&hib_wake)) {
		ret = gpio_pin_configure_dt(&hib_wake, GPIO_INPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure hibernate wake pin %d\n", ret,
			       hib_wake.pin);
		}
	}
#endif

	pm_state_set(PM_STATE_SOFT_OFF, 0);

	__enable_irq();
	printk("Phase 2: Hibernate did not engage\n");
}

/* Let the operator pick the terminal mode: each button press toggles
 * DeepSleep-OFF / hibernate; the displayed mode is entered after
 * TERMINAL_SELECT_TIMEOUT_MS idle. Entering it powers the SoC down and ends
 * the run.
 */
static void run_terminal_mode(void)
{
	printk("Phase 2: press the button to toggle the terminal mode "
	       "(entered after %d ms idle)\n",
	       TERMINAL_SELECT_TIMEOUT_MS);
	printk("Phase 2: terminal mode = %s\n", terminal_mode_name(terminal_pm_mode));

	/* Ignore any press latched during the earlier tests. */
	k_sem_reset(&button_sem);

	while (k_sem_take(&button_sem, K_MSEC(TERMINAL_SELECT_TIMEOUT_MS)) == 0) {
		terminal_pm_mode =
			(terminal_pm_mode == PM_MODE_DS_OFF) ? PM_MODE_HIB : PM_MODE_DS_OFF;
		printk("Phase 2: terminal mode = %s\n", terminal_mode_name(terminal_pm_mode));
	}

	if (terminal_pm_mode == PM_MODE_HIB) {
		run_hibernate_mode();
	} else {
		run_ds_off_mode();
	}
}

#endif /* CONFIG_APP_DEEP_MODES */

static int test_pm(void)
{
	int ret;

	if (!gpio_is_ready_dt(&red_led)) {
		return 0;
	}
	ret = gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return 0;
	}

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return 0;
	}
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
		       button.pin);
		return 0;
	}
	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
		       button.port->name, button.pin);
		return 0;
	}
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	/* Configure every present peripheral and run its baseline self-test; absent
	 * peripherals are compiled out in peripherals.c.
	 */
	peripherals_setup();

	/* Leave the DeepSleep mode selection to the SoC's pm_state_set(); do not
	 * override it here.
	 */

	pm_notifier_register(&pm_notif);

	/*
	 * Phase 1: runtime-idle only (WFI, clocks running). DeepSleep is locked
	 * out, so the LED keeps blinking and the counter keeps advancing.
	 */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);

	printk("Phase 1: runtime-idle only\n");
	k_msleep(MAIN_SLEEP_TIME_MS);
	print_pm_counts();

	/*
	 * Phase 2: exercise the deep sleep modes. Suspend long enough for the
	 * policy to select the mode, then re-test every peripheral after wake.
	 */
	printk("Phase 2: run each low-power mode in sequence\n");

	/* DeepSleep and DeepSleep-RAM return in-session, so run them back to back. */
	run_retained_mode_test(PM_STATE_SUSPEND_TO_IDLE, "DeepSleep");

#if defined(CONFIG_APP_DEEP_MODES)
	run_retained_mode_test(PM_STATE_SUSPEND_TO_RAM, "DeepSleep-RAM");

	/* The terminal mode powers the SoC down (button cold boot) and ends the
	 * run; the button also selects DeepSleep-OFF vs hibernate.
	 */
	run_terminal_mode();
#else
	printk("Phase 2: DeepSleep-RAM and terminal power-down skipped "
	       "(regular DeepSleep only)\n");
#endif /* CONFIG_APP_DEEP_MODES */

	printk("Sequence complete\n");

	return 0;
}

/* DUT entry, invoked from the worker thread in main.c. */
void app_main(void)
{
	(void)test_pm();
}

#endif /* CONFIG_APP_ROLE_DUT */
