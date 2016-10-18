/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <power.h>
#include <soc_power.h>
#include <gpio.h>
#include <misc/printk.h>
#include <string.h>
#include <rtc.h>

#define SECONDS_TO_SLEEP	5
#define ALARM		(RTC_ALARM_SECOND * (SECONDS_TO_SLEEP - 1))
#define SLEEPTICKS	SECONDS(SECONDS_TO_SLEEP)
#define GPIO_IN_PIN	16

static void create_device_list(void);
static void suspend_devices(void);
static void resume_devices(void);

static struct device *device_list;
static int device_count;

/*
 * Example ordered list to store devices on which
 * device power policies would be executed.
 */
#define DEVICE_POLICY_MAX 15
static char device_ordered_list[DEVICE_POLICY_MAX];
static char device_retval[DEVICE_POLICY_MAX];

static struct device *gpio_dev;
static int setup_gpio(void);
static int wait_gpio_low(void);

static struct device *rtc_dev;
static uint32_t start_time, end_time;
static void setup_rtc(void);
static void enable_wake_event(void);

int pm_state;

void main(void)
{
	printk("Power Management Demo\n");

	if (setup_gpio()) {
		printk("Demo aborted\n");
		return;
	}

	/*
	 * This is a safety measure to avoid
	 * bricking boards if anything went wrong.
	 * The pause here will allow re-flashing.
	 *
	 * Toggle GPIO pin 16 in following sequence:-
	 * (GPIO Pin 16 is DIO 8 in arduino_101 and
	 * DIO 4 in quark_se_c1000_devboard.)
	 * 1) Start connected to GND during power on or reset.
	 * 2) Disconnect from GND and connect to 3.3V
	 * 3) Disconnect from 3.3V. Test should start now.
	 *
	 */
	printk("Toggle gpio pin 16 to start demo\n");
	if (wait_gpio_low()) {
		printk("Demo aborted\n");
		return;
	}

	printk("PM demo started\n");
	setup_rtc();

	create_device_list();

	while (1) {
		task_sleep(SLEEPTICKS);
	}
}

static int check_pm_policy(int32_t ticks)
{
	static int policy;
	int power_states[] = {SYS_POWER_STATE_MAX, SYS_POWER_STATE_CPU_LPS,
		SYS_POWER_STATE_DEEP_SLEEP};

	/*
	 * Compare time available with wake latencies and select
	 * appropriate power saving policy
	 *
	 l
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
	resume_devices();

	end_time = rtc_read(rtc_dev);
	printk("\nLow power state exit!\n");
	printk("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

static void deep_sleep_exit(void)
{
	resume_devices();

	printk("Wake from Deep Sleep!\n");

	end_time = rtc_read(rtc_dev);
	printk("\nDeep sleep exit!\n");
	printk("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

static int low_power_state_entry(int32_t ticks)
{
	printk("\n\nLow power state entry!\n");

	start_time = rtc_read(rtc_dev);

	/* Turn off peripherals/clocks here */
	suspend_devices();

	enable_wake_event();

	_sys_soc_set_power_state(SYS_POWER_STATE_CPU_LPS);

	low_power_state_exit();

	return SYS_PM_LOW_POWER_STATE;
}

static int deep_sleep_entry(int32_t ticks)
{
	printk("\n\nDeep sleep entry!\n");

	start_time = rtc_read(rtc_dev);

	/* Don't need wake event notification */
	_sys_soc_disable_wake_event_notification();

	/* Turn off peripherals/clocks here */
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

int _sys_soc_suspend(int32_t ticks)
{
	int ret = SYS_PM_NOT_HANDLED;

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
		 */
		_sys_soc_power_state_post_ops(pm_state);
	}

	return ret;
}

void _sys_soc_resume(void)
{
	/*
	* Nothing to do in this example
	*
	* low_power_state_exit() was called inside low_power_state_entry
	* after exiting the CPU LPS states.

	* Some CPU LPS power states require enabling of interrupts
	* atomically when entering those states. The wake up from
	* such a state first executes code in the ISR of the interrupt
	* that caused the wake. This hook will be called from the ISR.
	* For such CPU LPS states, call low_power_state_exit() from here
	* so that restores are completed before scheduler swithes to other
	* tasks.
	*
	* Call _sys_soc_disable_wake_event_notification() if this
	* notification is not required.
	*/
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
}

static int setup_gpio(void)
{
	int ret;

	gpio_dev = device_get_binding("GPIO_0");
	if (!gpio_dev) {
		printk("Cannot find %s!\n", "GPIO_0");
		return 1;
	}

	/* Setup GPIO input */
	ret = gpio_pin_configure(gpio_dev, GPIO_IN_PIN, (GPIO_DIR_IN));
	if (ret) {
		printk("Error configuring GPIO!\n");
		return ret;
	}

	return 0;
}

static int wait_gpio_low(void)
{
	int ret;
	uint32_t v;

	/* Start with high */
	do {
		ret = gpio_pin_read(gpio_dev, GPIO_IN_PIN, &v);
		if (ret) {
			printk("Error reading GPIO!\n");
			return ret;
		}
	} while (!v);

	/* Wait till low */
	do {
		ret = gpio_pin_read(gpio_dev, GPIO_IN_PIN, &v);
		if (ret) {
			printk("Error reading GPIO!\n");
			return ret;
		}
	} while (v);

	return 0;
}

void rtc_interrupt_fn(struct device *rtc_dev)
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
	uint32_t alarm;

	alarm = (rtc_read(rtc_dev) + ALARM);
	rtc_set_alarm(rtc_dev, alarm);
}
