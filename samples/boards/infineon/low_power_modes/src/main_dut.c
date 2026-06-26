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
#include <power.h>

#include "peripherals.h"

#define MAIN_SLEEP_TIME_MS 2000

#define PM_MODE_DS     0
#define PM_MODE_DS_RAM 1
#define PM_MODE_DS_OFF 2
#define PM_MODE_HIB    3

/*
 * Terminal low-power mode that ends the Phase-2 sequence.  The sequence always
 * runs DeepSleep then DeepSleep-RAM (both return in-session), then one terminal
 * mode, which powers the SoC down; wake is a button-press cold boot, so the run
 * ends here.  The terminal mode is chosen at runtime: press the button to toggle
 * between DeepSleep-OFF and hibernate, and the displayed mode is entered once no
 * press arrives for this idle window (see run_terminal_mode()).
 */
#define TERMINAL_SELECT_TIMEOUT_MS 2000

/*
 * CONFIG_APP_DEEP_MODES gates the Phase-2 deep modes: it is enabled when the SoC
 * power management implements the retained (DeepSleep-RAM) and power-down
 * (DeepSleep-OFF / hibernate) modes.  Leave it disabled on a target whose power
 * management only implements regular DeepSleep, so the sample runs the shallow
 * phases only.
 */

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static struct gpio_callback button_cb_data;

/* Dedicated Hibernate wake pad (HIBERNATE_PIN0 = P2.0 / SW2 on the PSC3M5 EVK,
 * exposed as the sw1 alias).  Unlike the sw0/SW4 button - an ordinary GPIO used
 * for the terminal-mode toggle - only the dedicated wake pads can wake the chip
 * from Hibernate, and the pad must be configured as a digital input so its
 * buffer is enabled for the always-on wake detector to sense the press.  The
 * SoC only selects the wake source; the pad is configured here through the
 * ordinary GPIO driver.
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

/* Number of times the PM subsystem has entered a given deep state, used to
 * confirm that a requested low-power mode actually engaged.
 */
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

/* Run one retained/warm-boot low-power mode test (DeepSleep or DeepSleep-RAM).
 * Commands the CM33-NS into the same mode over the mailbox, unlocks this mode's
 * PM policy state so the idle loop can select it, exercises one sleep/wake cycle
 * re-testing every present peripheral after wake, then restores the policy lock
 * so the next mode in the sequence starts from a known state.
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

	/* After a DeepSleep-RAM warm boot the peripheral hardware is powered
	 * down and must be rebuilt before it is used - including the console
	 * UART - so this eager rebuild runs before the first post-wake console
	 * output.  ifx_pm_warm_boot_reinit_all() walks every device in link
	 * order and runs its TURN_ON handler from this thread.  It stays
	 * portable across Infineon devices: on a SoC whose warm boot already
	 * rebuilds peripherals automatically it is a harmless idempotent repeat,
	 * and on a SoC without DeepSleep-RAM it compiles to a no-op stub, so no
	 * SoC-specific guard is needed here.
	 */
	if (test_state == PM_STATE_SUSPEND_TO_RAM) {
		ifx_pm_warm_boot_reinit_all();
	}

	printk("Phase 2: woke after %u ms\n", t1 - t0);
	print_pm_counts();

	/* Confirm the requested state actually engaged.  If the platform only
	 * supports a shallower mode (for example the sysbuild secure boot stub
	 * configures regular DeepSleep only), the PM policy falls back and the
	 * entry count does not advance.
	 */
	if (pm_state_entry_count(test_state) == entries_before) {
		printk("Phase 2: WARNING - %s did not engage; platform fell back to a "
		       "shallower state\n",
		       name);
	}

	/* Re-test every present peripheral now that the clocks are back. */
	peripherals_test_after_wake(name);

	/* Lock the mode so the HF clocks stay on, then hold here.
	 * The blue LED should blink during this window, which visually
	 * confirms the PWM driver rebuilt the channel when the sample
	 * re-applied it after wake (see peripherals_test_after_wake()).
	 * Leave the lock held on exit so the next mode starts from a known
	 * (locked) state without re-entering DeepSleep here.
	 */
	pm_policy_state_lock_get(test_state, PM_ALL_SUBSTATES);
	printk("Phase 2: watch blue LED blink (PWM resumed via pm_action)\n");
	k_msleep(MAIN_SLEEP_TIME_MS);
}

#if defined(CONFIG_APP_DEEP_MODES)

/* Runtime-selected terminal power-down mode; toggled by the button in
 * run_terminal_mode().  Starts at DeepSleep-OFF.
 */
static uint32_t terminal_pm_mode = PM_MODE_DS_OFF;

static const char *terminal_mode_name(uint32_t mode)
{
	return (mode == PM_MODE_HIB) ? "Hibernate" : "DeepSleep-OFF";
}

/* Terminal mode: system DeepSleep-OFF.  Commands the CM33-NS into the same mode,
 * then powers the CPU domain down: wake is a full cold boot, so this does not
 * return on success and ends the run.  Enter through pm_state_set() (it masks
 * interrupts before WFI) and press the wake button to trigger the cold boot.
 */
static void run_ds_off_mode(void)
{
	printk("Phase 2: exercising DeepSleep-OFF\n");

	printk("Phase 2: entering DeepSleep-OFF. Press the button to wake/reset.\n");

	pm_state_set(PM_STATE_SOFT_OFF, 1);

	__enable_irq();
	printk("Phase 2: DeepSleep-OFF did not engage\n");
}

/* Terminal mode: system hibernate.  Commands the CM33-NS into the same mode,
 * then powers the whole chip down; wake is a button-press cold boot, so this
 * does not return and ends the run.  The Hibernate wake source (HIBERNATE_PIN0
 * = P2.0 / SW2, active low) is armed by the SoC from the devicetree
 * hibernate-wakeup node; the application only configures the wake pad (P2.0) as
 * an ordinary GPIO input so its buffer feeds the always-on wake detector.
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

	printk("Phase 2: entering hibernate. Press SW2 (P2.0) to wake/reset.\n");

	/* Configure the dedicated wake pad HIBERNATE_PIN0 (P2.0 / SW2) as a digital
	 * input so its pull-up holds it inactive (active low) and its input buffer
	 * feeds the always-on wake detector; without this the SW2 press is never
	 * sensed.  This is an ordinary GPIO configuration - the SoC only selects the
	 * wake source, not the pad.  P9.0 / HIBERNATE_PIN1 is unconnected on this
	 * board, so it is never armed (arming PIN1_LOW would self-wake immediately).
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

/* Let the operator choose the terminal power-down mode at runtime.  Each button
 * press toggles between DeepSleep-OFF and hibernate; the displayed mode is
 * entered once no press arrives for TERMINAL_SELECT_TIMEOUT_MS.  Entering a
 * terminal mode powers the SoC down, so the selected mode ends the run.
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

	/* Configure every peripheral present in the devicetree and run its
	 * baseline self-test.  Peripherals absent on this board are compiled
	 * out inside peripherals.c.
	 */
	peripherals_setup();

	/* Leave the DeepSleep mode configuration at its default.  The SoC's
	 * pm_state_set() already selects the correct DeepSleep mode for
	 * each transition (regular DeepSleep, DeepSleep-RAM, DeepSleep-OFF), so
	 * the application must not override it here.
	 */

	pm_notifier_register(&pm_notif);

	/*
	 * Phase 1: Runtime-idle only (WFI sleep, clocks running)
	 *
	 * DeepSleep is locked out so the policy can only pick runtime-idle.
	 * The blue LED keeps blinking and the counter keeps advancing.
	 */
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);

	printk("Phase 1: runtime-idle only\n");
	k_msleep(MAIN_SLEEP_TIME_MS);
	print_pm_counts();

	/*
	 * Phase 2: exercise the deep sleep modes
	 *
	 * For the retained (DS) and warm-boot (DS-RAM) modes, suspend the LED
	 * thread so idle periods are long enough for the PM policy to select
	 * the mode, then re-test every peripheral after wake to confirm the
	 * device pm_action callbacks restored it.
	 */
	printk("Phase 2: run each low-power mode in sequence\n");

	/* DeepSleep and DeepSleep-RAM return in-session, so run them back to
	 * back, each commanding the CM33-NS into the same mode and re-testing
	 * every peripheral after wake.
	 */
	run_retained_mode_test(PM_STATE_SUSPEND_TO_IDLE, "DeepSleep");

#if defined(CONFIG_APP_DEEP_MODES)
	run_retained_mode_test(PM_STATE_SUSPEND_TO_RAM, "DeepSleep-RAM");

	/* The terminal mode powers the SoC down; wake is a button-press cold
	 * boot, so it ends the run.  The button also selects DeepSleep-OFF vs
	 * hibernate at runtime (see run_terminal_mode()).
	 */
	run_terminal_mode();
#else
	printk("Phase 2: DeepSleep-RAM and terminal power-down skipped "
	       "(regular DeepSleep only)\n");
#endif /* CONFIG_APP_DEEP_MODES */

	printk("Sequence complete\n");

	return 0;
}

/* Application entry for the DUT (device under test).  Invoked from the shared
 * worker thread in main.c, which owns the enlarged stack this deep call path
 * needs.
 */
void app_main(void)
{
	(void)test_pm();
}

#endif /* CONFIG_APP_ROLE_DUT */
