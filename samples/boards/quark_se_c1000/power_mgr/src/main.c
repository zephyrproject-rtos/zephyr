/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <power.h>
#include <soc_power.h>
#include <misc/printk.h>
#include <string.h>
#include <rtc.h>

#define SECONDS_TO_SLEEP	5
#define ALARM		(RTC_ALARM_SECOND * (SECONDS_TO_SLEEP - 1))
#define GPIO_IN_PIN	16

/* In Tickless Kernel mode, time is passed in milliseconds instead of ticks */
#ifdef CONFIG_TICKLESS_KERNEL
#define TICKS_TO_SECONDS_MULTIPLIER 1000
#define TIME_UNIT_STRING "milliseconds"
#else
#define TICKS_TO_SECONDS_MULTIPLIER CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define TIME_UNIT_STRING "ticks"
#endif

#define MIN_TIME_TO_SUSPEND	((SECONDS_TO_SLEEP * \
				  TICKS_TO_SECONDS_MULTIPLIER) - \
				  (TICKS_TO_SECONDS_MULTIPLIER / 2))

static void create_device_list(void);
static void suspend_devices(void);
static void resume_devices(void);

static struct device *device_list;
static int device_count;

static int post_ops_done = 1;

/*
 * Example ordered list to store devices on which
 * device power policies would be executed.
 */
#define DEVICE_POLICY_MAX 15
static char device_ordered_list[DEVICE_POLICY_MAX];
static char device_retval[DEVICE_POLICY_MAX];

static struct device *rtc_dev;
static u32_t start_time, end_time;
static void setup_rtc(void);
static void enable_wake_event(void);

int pm_state;

void main(void)
{
	printk("Power Management Demo on %s\n", CONFIG_ARCH);

	setup_rtc();

	create_device_list();

	while (1) {
		printk("\nApplication main thread\n");
		k_sleep(SECONDS_TO_SLEEP * 1000);
	}
}

static int check_pm_policy(s32_t ticks)
{
	static int policy;
	int power_states[] = {SYS_POWER_STATE_MAX, SYS_POWER_STATE_CPU_LPS,
		SYS_POWER_STATE_DEEP_SLEEP};

	/*
	 * Compare time available with wake latencies and select
	 * appropriate power saving policy
	 *
	 * For the demo we will alternate between following policies
	 *
	 * 0 = no power saving operation
	 * 1 = low power state
	 * 2 = deep sleep
	 *
	 */

#if (defined(CONFIG_SYS_POWER_DEEP_SLEEP))
	policy = (++policy > 2 ? 0 : policy);
	/*
	 * If deep sleep was selected, we need to check if any device is in
	 * the middle of a transaction
	 *
	 * Use device_busy_check() to check specific devices
	 */
	if ((policy == 2) && device_any_busy_check()) {
		/* Devices are busy - do CPU LPS instead */
		policy = 1;
	}
#else
	policy = (++policy > 1 ? 0 : policy);
#endif

	return power_states[policy];
}

static void low_power_state_exit(void)
{
	/* Turn on suspended peripherals/clocks as necessary */

	end_time = rtc_read(rtc_dev);
	printk("\nLow power state exit!\n");
	printk("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

static void deep_sleep_exit(void)
{
	/* Turn on peripherals and restore device states as necessary */
	resume_devices();

	printk("Wake from Deep Sleep!\n");

	end_time = rtc_read(rtc_dev);
	printk("\nDeep sleep exit!\n");
	printk("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

static int low_power_state_entry(s32_t ticks)
{
	printk("\nLow power state entry!\n");

	start_time = rtc_read(rtc_dev);

	/* Turn off peripherals/clocks as necessary */

	enable_wake_event();

	_sys_soc_set_power_state(SYS_POWER_STATE_CPU_LPS);

	return SYS_PM_LOW_POWER_STATE;
}

static int deep_sleep_entry(s32_t ticks)
{
	printk("\nDeep sleep entry!\n");

	start_time = rtc_read(rtc_dev);

	/* Don't need pm idle exit event notification */
	_sys_soc_pm_idle_exit_notification_disable();

	/* Save device states and turn off peripherals as necessary */
	suspend_devices();

	enable_wake_event();

	_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP);

	/*
	 * At this point system has woken up from
	 * deep sleep.
	 */

	deep_sleep_exit();

	return SYS_PM_DEEP_SLEEP;
}

int _sys_soc_suspend(s32_t ticks)
{
	int ret = SYS_PM_NOT_HANDLED;

	post_ops_done = 0;

	if ((ticks != K_FOREVER) && (ticks < MIN_TIME_TO_SUSPEND)) {
		printk("Not enough time for PM operations (" TIME_UNIT_STRING
		       ": %d).\n", ticks);
		return SYS_PM_NOT_HANDLED;
	}

	pm_state = check_pm_policy(ticks);

	switch (pm_state) {
	case SYS_POWER_STATE_CPU_LPS:
		/* Do CPU LPS operations */
		ret = low_power_state_entry(ticks);
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
		/* Do deep sleep operations */
		ret = deep_sleep_entry(ticks);
		break;
	default:
		/* No PM operations */
		printk("\nNo PM operations done\n");
		ret = SYS_PM_NOT_HANDLED;
		break;
	}

	if (ret != SYS_PM_NOT_HANDLED) {
		/*
		 * Do any arch or soc specific post operations specific to the
		 * power state.
		 *
		 * If this enables interrupts, then it should be done
		 * right before function return.
		 *
		 * Some CPU power states would require interrupts to be
		 * enabled at the time of entering the low power state.
		 * For such states the post operations need to be done
		 * at _sys_soc_resume. To avoid doing it twice, check a
		 * flag.
		 */
		if (!post_ops_done) {
			if (pm_state == SYS_POWER_STATE_CPU_LPS) {
				low_power_state_exit();
			}
			post_ops_done = 1;
			_sys_soc_power_state_post_ops(pm_state);
		}
	}

	return ret;
}

void _sys_soc_resume(void)
{
	/*
	 * This notification is called from the ISR of the event
	 * that caused exit from kernel idling after PM operations.
	 *
	 * Some CPU low power states require enabling of interrupts
	 * atomically when entering those states. The wake up from
	 * such a state first executes code in the ISR of the interrupt
	 * that caused the wake. This hook will be called from the ISR.
	 * For such CPU LPS states, do post operations and restores here.
	 * The kernel scheduler will get control after the ISR finishes
	 * and it may schedule another thread.
	 *
	 * Call _sys_soc_pm_idle_exit_notification_disable() if this
	 * notification is not required.
	 */
	if (!post_ops_done) {
		if (pm_state == SYS_POWER_STATE_CPU_LPS) {
			low_power_state_exit();
		}
		post_ops_done = 1;
		_sys_soc_power_state_post_ops(pm_state);
	}
}

static void suspend_devices(void)
{
	int i;

	for (i = device_count - 1; i >= 0; i--) {
		int idx = device_ordered_list[i];

		/* If necessary  the policy manager can check if a specific
		 * device in the device list is busy as shown below :
		 * if(device_busy_check(&device_list[idx])) {do something}
		 */
		device_retval[i] = device_set_power_state(&device_list[idx],
						DEVICE_PM_SUSPEND_STATE);
	}
}

static void resume_devices(void)
{
	int i;

	for (i = 0; i < device_count; i++) {
		if (!device_retval[i]) {
			int idx = device_ordered_list[i];

			device_set_power_state(&device_list[idx],
						DEVICE_PM_ACTIVE_STATE);
		}
	}
}

static void create_device_list(void)
{
	int count;
	int i;

	/*
	 * Following is an example of how the device list can be used
	 * to suspend devices based on custom policies.
	 *
	 * Create an ordered list of devices that will be suspended.
	 * Ordering should be done based on dependencies. Devices
	 * in the beginning of the list will be resumed first.
	 *
	 * Other devices depend on APICs so ioapic and loapic devices
	 * will be placed first in the device ordered list. Move any
	 * other devices to the beginning as necessary. e.g. uart
	 * is useful to enable early prints.
	 */
	device_list_get(&device_list, &count);

#if (CONFIG_X86)
	device_count = 3; /* Reserve for ioapic, loapic and uart */

	for (i = 0; (i < count) && (device_count < DEVICE_POLICY_MAX); i++) {
		if (!strcmp(device_list[i].config->name, "loapic")) {
			device_ordered_list[0] = i;
		} else if (!strcmp(device_list[i].config->name, "ioapic")) {
			device_ordered_list[1] = i;
		} else if (!strcmp(device_list[i].config->name, "UART_0")) {
			device_ordered_list[2] = i;
		} else {
			device_ordered_list[device_count++] = i;
		}
	}
#elif (CONFIG_ARC)
	device_count = 1; /* Reserve for irq unit */

	for (i = 0; (i < count) && (device_count < DEVICE_POLICY_MAX); i++) {
		if (!strcmp(device_list[i].config->name, "arc_v2_irq_unit")) {
			device_ordered_list[0] = i;
		} else {
			device_ordered_list[device_count++] = i;
		}
	}
#endif
}

static void rtc_interrupt_fn(struct device *rtc_dev)
{
	printk("Wake up event handler\n");
}

static void setup_rtc(void)
{
	struct rtc_config config;

	rtc_dev = device_get_binding("RTC_0");

	config.init_val = 0;
	config.alarm_enable = 0;
	config.alarm_val = ALARM;
	config.cb_fn = rtc_interrupt_fn;

	rtc_enable(rtc_dev);

	rtc_set_config(rtc_dev, &config);

}

static void enable_wake_event(void)
{
	u32_t now = rtc_read(rtc_dev);
	u32_t alarm;

	alarm = (rtc_read(rtc_dev) + ALARM);
	rtc_set_alarm(rtc_dev, alarm);

	/* Wait a few ticks to ensure the alarm value gets loaded */
	while (rtc_read(rtc_dev) < now + 5)
		;
}
