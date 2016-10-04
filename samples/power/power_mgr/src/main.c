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

#define ALARM (RTC_ALARM_MINUTE / 12)
#define SLEEPTICKS	SECONDS(5)
#define GPIO_IN_PIN	16

static void create_device_list(void);
static void suspend_devices(int pm_policy);
static void resume_devices(int pm_policy);

static struct device *device_list;
static int device_count;

/*
 * Example ordered list to store devices on which
 * device power policies would be executed.
 */
#define DEVICE_POLICY_MAX 15
static char device_policy_list[DEVICE_POLICY_MAX];
static char device_retval[DEVICE_POLICY_MAX];

static struct device *gpio_dev;
static int setup_gpio(void);
static int wait_gpio_low(void);

static struct device *rtc_dev;
static uint32_t start_time, end_time;
static void setup_rtc(void);
static void enable_wake_event(void);

static int test_started;

void main(void)
{
	printk("Power Management Demo\n");

	if (setup_gpio()) {
		printk("Test aborted\n");
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
	printk("Toggle gpio pin 16 to start test\n");
	if (wait_gpio_low()) {
		printk("Test aborted\n");
		return;
	}

	printk("PM test started\n");
	setup_rtc();
	test_started = 1;

	create_device_list();

	while (1) {
		task_sleep(SLEEPTICKS);
	}
}

static int check_pm_policy(int32_t ticks)
{
	static int policy;

	/*
	 * Compare time available with wake latencies and select
	 * appropriate power saving policy
	 *
	 * For the demo we will alternate between following states
	 *
	 * 0 = no power saving operation
	 * 1 = low power state
	 * 2 = device suspend only
	 * 3 = deep sleep
	 *
	 */

	/* Set the max val to 2 if deep sleep is not supported */
	policy = (policy > 3 ? 0 : policy);

	return policy++;
}

static void low_power_state_exit(void)
{
	resume_devices(SYS_PM_LOW_POWER_STATE);

	end_time = rtc_read(rtc_dev);
	printk("\nLow power state policy exit!\n");
	printk("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

static void device_suspend_only_exit(void)
{
	resume_devices(SYS_PM_DEVICE_SUSPEND_ONLY);

	end_time = rtc_read(rtc_dev);
	printk("\nDevice suspend only policy exit!\n");
	printk("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

static void deep_sleep_exit(void)
{
	resume_devices(SYS_PM_DEEP_SLEEP);

	printk("Wake from Deep Sleep!\n");

	end_time = rtc_read(rtc_dev);
	printk("\nDeep sleep policy exit!\n");
	printk("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

static int low_power_state_entry(int32_t ticks)
{
	printk("\n\nLow power state policy entry!\n");

	/* Turn off peripherals/clocks here */
	suspend_devices(SYS_PM_LOW_POWER_STATE);

	_sys_soc_set_power_policy(SYS_PM_LOW_POWER_STATE);

	_sys_soc_put_low_power_state();

	return SYS_PM_LOW_POWER_STATE;
}

static int device_suspend_only_entry(int32_t ticks)
{
	printk("Device suspend only policy entry!\n");

	/* Turn off peripherals/clocks here */
	suspend_devices(SYS_PM_DEVICE_SUSPEND_ONLY);

	_sys_soc_set_power_policy(SYS_PM_DEVICE_SUSPEND_ONLY);

	return SYS_PM_DEVICE_SUSPEND_ONLY;
}

static int deep_sleep_entry(int32_t ticks)
{
	printk("\n\nDeep sleep policy entry!\n");

	/* Turn off peripherals/clocks here */
	suspend_devices(SYS_PM_DEEP_SLEEP);

	_sys_soc_set_power_policy(SYS_PM_DEEP_SLEEP);

	/*
	 * Returns 0 when context is saved.
	 * Returns 1 when context was restored and control was
	 * transferred to it during DS resume.
	 */
	if (!_sys_soc_save_cpu_context()) {
		_sys_soc_put_deep_sleep();
	}

	/*
	 * At this point system has woken up from
	 * deep sleep.
	 */

	deep_sleep_exit();

	/* Clear current power policy */
	_sys_soc_set_power_policy(SYS_PM_NOT_HANDLED);

	return SYS_PM_DEEP_SLEEP;
}

int _sys_soc_suspend(int32_t ticks)
{
	int pm_state;
	int ret = SYS_PM_NOT_HANDLED;

	pm_state = check_pm_policy(ticks);

	switch (pm_state) {
	case 1: /* LPS */
		start_time = rtc_read(rtc_dev);
		ret = low_power_state_entry(ticks);
		break;
	case 2: /* Device Suspend Only */
		start_time = rtc_read(rtc_dev);
		ret = device_suspend_only_entry(ticks);
		break;
	case 3: /* Deep Sleep */
		/*
		 * if the policy manager chooses to go to deep sleep, we need to
		 * check if any device is in the middle of a transaction
		 */
		if (!device_any_busy_check()) {
			/* Do deep sleep operations */
			start_time = rtc_read(rtc_dev);
			enable_wake_event();
			ret = deep_sleep_entry(ticks);
			if (ret == SYS_PM_DEEP_SLEEP) {
				/*
				 * Do any arch or soc specific post
				 * operations specific to deep sleep.
				 *
				 * This would enable interrupts so
				 * it should be done right before
				 * function return
				 */
				_sys_soc_deep_sleep_post_ops();
			}
		}
		break;
	default:
		/* No PM operations */
		ret = SYS_PM_NOT_HANDLED;
		break;
	}

	return ret;
}

void _sys_soc_resume(void)
{
	uint32_t pm_policy;

	pm_policy = _sys_soc_get_power_policy();

	/* Clear current power policy */
	_sys_soc_set_power_policy(SYS_PM_NOT_HANDLED);

	switch (pm_policy) {
	case SYS_PM_DEEP_SLEEP:
		/*
		 * This should transfer control to the point
		 * where CPU context was saved. Context was
		 * saved in _sys_power_save_cpu_context(), which
		 * was called in deep_sleep_entry();
		 *
		 * deep_sleep_exit() will be called at the point
		 * of resume inside deep_sleep_entry().
		 */
		_sys_soc_restore_cpu_context();
		break;
	case SYS_PM_LOW_POWER_STATE:
		low_power_state_exit();
		break;
	case SYS_PM_DEVICE_SUSPEND_ONLY:
		device_suspend_only_exit();
		break;
	default:
		/* cold boot */
		break;
	}
}

static void suspend_devices(int pm_policy)
{
	int i;

	for (i = device_count - 1; i >= 0; i--) {
		int idx = device_policy_list[i];

		/* If necessary  the policy manager can check if a specific
		 * device in the policy list is busy as shown below :
		 * if(device_busy_check(&device_list[idx])) {do something}
		 */
		device_retval[i] = device_set_power_state(&device_list[idx],
						DEVICE_PM_SUSPEND_STATE);
	}
}

static void resume_devices(int pm_policy)
{
	int i;

	for (i = 0; i < device_count; i++) {
		if (!device_retval[i]) {
			int idx = device_policy_list[i];
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
	 * will be placed first in the device policy list. Move any
	 * other devices to the beginning as necessary. e.g. uart
	 * is useful to enable early prints.
	 */
	device_list_get(&device_list, &count);

	device_count = 3; /* Reserve for ioapic, loapic and uart */

	for (i = 0; (i < count) && (device_count < DEVICE_POLICY_MAX); i++) {
		if (!strcmp(device_list[i].config->name, "loapic")) {
			device_policy_list[0] = i;
		} else if (!strcmp(device_list[i].config->name, "ioapic")) {
			device_policy_list[1] = i;
		} else if (!strcmp(device_list[i].config->name, "UART_0")) {
			device_policy_list[2] = i;
		} else {
			device_policy_list[device_count++] = i;
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
	printk("Deep Sleep wake up event handler\n");
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
