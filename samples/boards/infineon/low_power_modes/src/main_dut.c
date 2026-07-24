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

#include <cy_syspm.h>
#include <soc.h>
#include <power.h>

#if defined(CONFIG_APP_COMPANION_MBOX)
#include "ds_mbox.h"
#endif
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
 * Two independent capabilities gate the Phase-2 sequence, both set in Kconfig:
 *
 *   CONFIG_APP_DEEP_MODES     - the SoC power management implements the retained
 *                               (DeepSleep-RAM) and power-down (DeepSleep-OFF /
 *                               hibernate) modes.
 *   CONFIG_APP_COMPANION_MBOX - a second core runs the companion image and must
 *                               be commanded into the same mode over the
 *                               inter-core mailbox before this core sleeps.
 *
 * On PSE84 both track the SRF companion, so they are enabled together in the
 * TF-M flow and disabled together in the sysbuild (pse84_boot.c) flow.  A
 * single-core SoC enables APP_DEEP_MODES only and drives every register itself.
 */

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

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

static const struct device *const uart_console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
extern uint32_t ifx_cat1_uart_get_num_in_tx_fifo(const struct device *dev);
extern bool ifx_cat1_uart_get_tx_active(const struct device *dev);

static void wait_for_printk_to_complete(void)
{
	while (ifx_cat1_uart_get_num_in_tx_fifo(uart_console_dev)) {
	}
	while (ifx_cat1_uart_get_tx_active(uart_console_dev)) {
	}
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	k_sem_give(&button_sem);
}

static void print_pm_counts(void)
{
	printk("Sleep: %u | Deepsleep: %u\n", sleep_count, deepsleep_count);
	wait_for_printk_to_complete();
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
static void run_retained_mode_test(uint32_t mbox_mode, enum pm_state test_state, const char *name)
{
	printk("Phase 2: exercising %s\n", name);
	wait_for_printk_to_complete();

	pm_policy_state_lock_put(test_state, PM_ALL_SUBSTATES);

#if defined(CONFIG_APP_COMPANION_MBOX)
	ds_mbox_send(mbox_mode);
#else
	ARG_UNUSED(mbox_mode);
#endif

	uint32_t cnt_before = peripherals_counter_read();
	uint32_t entries_before = pm_state_entry_count(test_state);

	printk("Phase 2: enter %s (counter=%u, watch blue LED freeze)\n", name, cnt_before);
	wait_for_printk_to_complete();

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
		wait_for_printk_to_complete();
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
	wait_for_printk_to_complete();
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
	wait_for_printk_to_complete();

	printk("Phase 2: entering DeepSleep-OFF. Press the button to wake/reset.\n");
	wait_for_printk_to_complete();

#if defined(CONFIG_APP_COMPANION_MBOX)
	ds_mbox_send(PM_MODE_DS_OFF);
#endif
	pm_state_set(PM_STATE_SOFT_OFF, 1);

	__enable_irq();
	printk("Phase 2: DeepSleep-OFF did not engage\n");
	wait_for_printk_to_complete();
}

/* Terminal mode: system hibernate.  Commands the CM33-NS into the same mode,
 * then powers the whole chip down; wake is a button-press cold boot, so this
 * does not return and ends the run.  Re-arm PIN1 (P8.3 / SW2, active low) as the
 * hibernate wake source right before entry so its configuration is unambiguous.
 */
static void run_hibernate_mode(void)
{
	int ret;

	printk("Phase 2: exercising Hibernate\n");
	wait_for_printk_to_complete();

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

#if defined(CONFIG_APP_COMPANION_MBOX)
	ds_mbox_send(PM_MODE_HIB);
#endif
	printk("Phase 2: entering hibernate. Press the button to wake/reset.\n");
	wait_for_printk_to_complete();

	pm_state_set(PM_STATE_SOFT_OFF, 0);

	__enable_irq();
	printk("Phase 2: Hibernate did not engage\n");
	wait_for_printk_to_complete();
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
	wait_for_printk_to_complete();

	/* Ignore any press latched during the earlier tests. */
	k_sem_reset(&button_sem);

	while (k_sem_take(&button_sem, K_MSEC(TERMINAL_SELECT_TIMEOUT_MS)) == 0) {
		terminal_pm_mode =
			(terminal_pm_mode == PM_MODE_DS_OFF) ? PM_MODE_HIB : PM_MODE_DS_OFF;
		printk("Phase 2: terminal mode = %s\n", terminal_mode_name(terminal_pm_mode));
		wait_for_printk_to_complete();
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
	 * pm_state_set() already selects the correct Cy_SysPm DeepSleep mode for
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
	wait_for_printk_to_complete();
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
	wait_for_printk_to_complete();

	/* DeepSleep and DeepSleep-RAM return in-session, so run them back to
	 * back, each commanding the CM33-NS into the same mode and re-testing
	 * every peripheral after wake.
	 */
	run_retained_mode_test(PM_MODE_DS, PM_STATE_SUSPEND_TO_IDLE, "DeepSleep");

#if defined(CONFIG_APP_DEEP_MODES)
	run_retained_mode_test(PM_MODE_DS_RAM, PM_STATE_SUSPEND_TO_RAM, "DeepSleep-RAM");

	/* The terminal mode powers the SoC down; wake is a button-press cold
	 * boot, so it ends the run.  The button also selects DeepSleep-OFF vs
	 * hibernate at runtime (see run_terminal_mode()).
	 */
	run_terminal_mode();
#else
	printk("Phase 2: DeepSleep-RAM and terminal power-down skipped "
	       "(regular DeepSleep only)\n");
	wait_for_printk_to_complete();
#endif /* CONFIG_APP_DEEP_MODES */

	printk("Sequence complete\n");
	wait_for_printk_to_complete();

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
