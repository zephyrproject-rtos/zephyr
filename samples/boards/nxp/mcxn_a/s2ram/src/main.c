/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/drivers/wuc.h>

#define WAKEUP_DELAY_S 3U

#define APP_STACK_SIZE 2048
#define APP_PRIORITY   5

/* Incremented before every suspend. Its value surviving the suspend/resume
 * cycle proves the RAM was retained while the CORE domain was powered down.
 */
static uint32_t cycles;

#define CONSOLE_NODE   DT_CHOSEN(zephyr_console)
#define CONSOLE_PARENT DT_PARENT(CONSOLE_NODE)

static const struct device *const console_dev = DEVICE_DT_GET(CONSOLE_NODE);
#if DT_NODE_HAS_COMPAT(CONSOLE_PARENT, nxp_lp_flexcomm)
#define HAS_FLEXCOMM_PARENT 1
static const struct device *const flexcomm_dev = DEVICE_DT_GET(CONSOLE_PARENT);
#endif

/*
 * Deep Power Down resets every CORE-domain peripheral. On a transparent resume
 * the SoC restores the CPU context and clocks, but the console UART driver has
 * no device pm hooks, so re-initialise it before printing. Three clock gates
 * that Deep Power Down switched off have to be re-opened, bottom up, before the
 * console can drive its pins again:
 *
 *   1. the PORT pin-mux controllers - their gate must be on for the LPUART
 *      pinctrl re-apply (step 3) to actually reach the pad-mux registers,
 *   2. the parent LP_FLEXCOMM - its init re-enables the peripheral clock gate
 *      and re-selects LPUART mode, then
 *   3. the LPUART itself - its init re-applies pinctrl and the baud/format.
 *
 * device_init() is a no-op on an already-initialised device, so clear the
 * initialised flag to force each driver to re-run its init against the reset
 * hardware. (A production driver would do this through a PM_DEVICE_ACTION_RESUME
 * handler instead.) printk keeps working once the LPUART is back: its console
 * output hook still points at the same device.
 */
#define REINIT_DEVICE(dev)				\
	do {						\
		(dev)->state->initialized = false;	\
		(void)device_init(dev);			\
	} while (0)

#define REINIT_PORT(node_id) REINIT_DEVICE(DEVICE_DT_GET(node_id));

static void resume_console(void)
{
	/* Re-open every PORT pin-mux clock gate that Deep Power Down closed. */
	DT_FOREACH_STATUS_OKAY(nxp_port_pinmux, REINIT_PORT)

#ifdef HAS_FLEXCOMM_PARENT
	REINIT_DEVICE(flexcomm_dev);
#endif
	REINIT_DEVICE(console_dev);
}

#if defined(CONFIG_SAMPLE_S2RAM_WAKEUP_BUTTON)
/* In real-world applications, the button's wake-up event should be handled
 * via the driver hook, allowing the application to directly wait for input
 * events without needing to consider the existence of WUU. However, due to
 * the current lack of hook handling, we use the PM notifier to handle the
 * wake-up event.
 */
static K_SEM_DEFINE(button_wakeup, 0, 1);
static const struct wuc_dt_spec button = WUC_DT_SPEC_GET(DT_ALIAS(wakeup_button));

static void on_pm_state_exit(enum pm_state state)
{
	if (state == PM_STATE_SUSPEND_TO_RAM) {
		k_sem_give(&button_wakeup);
	}
}

static struct pm_notifier button_pm_notifier = {
	.state_exit = on_pm_state_exit,
};
#else
/* LPTMR0 (the system-timer companion) reaches the core as a WUU internal-module
 * wakeup source.
 */
static const struct wuc_dt_spec timer_wakeup = WUC_DT_SPEC_GET(DT_NODELABEL(lptmr0));
#endif /* CONFIG_SAMPLE_S2RAM_WAKEUP_BUTTON */

static void arm_wakeup_source(void)
{
#if defined(CONFIG_SAMPLE_S2RAM_WAKEUP_BUTTON)
	(void)wuc_enable_wakeup_source_dt(&button);
#else
	(void)wuc_enable_wakeup_source_dt(&timer_wakeup);
#endif
}

static void app_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (cycles < CONFIG_SAMPLE_APP_TEST_CYCLES) {
		cycles++;

#if defined(CONFIG_SAMPLE_S2RAM_WAKEUP_BUTTON)
		printk("Entering suspend-to-RAM (cycle %u/%u); press SW2 to wake\n", cycles,
		       CONFIG_SAMPLE_APP_TEST_CYCLES);
#else
		printk("Entering suspend-to-RAM (cycle %u/%u); wake in %u s\n", cycles,
		       CONFIG_SAMPLE_APP_TEST_CYCLES, WAKEUP_DELAY_S);
#endif

		/* Let the UART finish shifting out the line above before DPD cuts its clock. */
		k_busy_wait(2000);

#if defined(CONFIG_SOC_SERIES_MCXAXX6) || defined(CONFIG_SOC_SERIES_MCXAXX7)
		/* On MCXAxx6 / MCXAxx7 the Deep Power Down wakeup reset clears the
		 * WUU wakeup-source configuration, so the source has to be re-armed
		 * before every entry.
		 */
		arm_wakeup_source();
#endif

		pm_state_force(0U, &(struct pm_state_info){PM_STATE_SUSPEND_TO_RAM, 0, 0});
#if defined(CONFIG_SAMPLE_S2RAM_WAKEUP_BUTTON)
		/* Block until SW2 wakes the SoC; the PM notifier gives this. */
		k_sem_take(&button_wakeup, K_FOREVER);
#else
		k_sleep(K_SECONDS(WAKEUP_DELAY_S));
#endif

		resume_console();
		printk("Resumed from suspend-to-RAM; retained counter is %u\n", cycles);
	}

	/* All threads are now idle for good, but every PM state is locked, so the
	 * SoC stays awake and the board remains debuggable after the test.
	 */
	printk("Completed %u suspend-to-RAM cycles\n", cycles);
}

K_THREAD_STACK_DEFINE(app_stack, APP_STACK_SIZE);
static struct k_thread app_thread_data;

int main(void)
{
	printk("%s suspend-to-RAM demo\n", CONFIG_BOARD);
	printk("Retained S2RAM cycle counter: %u\n", cycles);

	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);

	/* The application selects and enables its own wakeup source through the
	 * WUC subsystem; the SoC power code does not hard-code one.
	 */
#if defined(CONFIG_SAMPLE_S2RAM_WAKEUP_BUTTON)
	pm_notifier_register(&button_pm_notifier);
#endif
	arm_wakeup_source();

	k_thread_create(&app_thread_data, app_stack, APP_STACK_SIZE, app_thread,
			NULL, NULL, NULL, APP_PRIORITY, 0, K_NO_WAIT);

	return 0;
}
