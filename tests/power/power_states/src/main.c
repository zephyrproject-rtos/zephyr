/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <power.h>
#include <misc/printk.h>
#include <rtc.h>
#include <gpio.h>
#include <counter.h>
#include <aio_comparator.h>
#include <ztest.h>
#include <power.h>
#include <soc_power.h>
#include <string.h>

static enum power_states states_list[] = {
	SYS_POWER_STATE_CPU_LPS,
	SYS_POWER_STATE_CPU_LPS_1,
#ifdef CONFIG_X86
	SYS_POWER_STATE_CPU_LPS_2,
#endif
#if (CONFIG_SYS_POWER_DEEP_SLEEP)
	SYS_POWER_STATE_DEEP_SLEEP,
	SYS_POWER_STATE_DEEP_SLEEP_1,
#endif
};

#define TIMEOUT 5 /* in seconds */
#define MAX_SUSPEND_DEVICE_COUNT 15
#define NB_STATES ARRAY_SIZE(states_list)
#define MAX_SYS_PM_STATES	5

/* In Tickless Kernel mode, time is passed in milliseconds instead of ticks */
#ifdef CONFIG_TICKLESS_KERNEL
#define TICKS_TO_SECONDS_MULTIPLIER 1000
#define TIME_UNIT_STRING "milliseconds"
#else
#define TICKS_TO_SECONDS_MULTIPLIER CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define TIME_UNIT_STRING "ticks"
#endif

#define MIN_TIME_TO_SUSPEND	((TIMEOUT * TICKS_TO_SECONDS_MULTIPLIER) - \
				  (TICKS_TO_SECONDS_MULTIPLIER / 2))

static struct device *suspend_devices[MAX_SUSPEND_DEVICE_COUNT];
static int suspend_device_count;
static unsigned int current_state = NB_STATES - 1;
static int post_ops_done = 1;
static s64_t start_time, milliseconds_spent;
static int test_complete;

static enum power_states get_next_state(void)
{
	current_state = (current_state + 1) % NB_STATES;
	return states_list[current_state];
}

static const char *state_to_string(int state)
{
	switch (state) {
	case SYS_POWER_STATE_CPU_LPS:
		return "SYS_POWER_STATE_CPU_LPS";
	case SYS_POWER_STATE_CPU_LPS_1:
		return "SYS_POWER_STATE_CPU_LPS_1";
#ifdef CONFIG_X86
	case SYS_POWER_STATE_CPU_LPS_2:
		return "SYS_POWER_STATE_CPU_LPS_2";
#endif
	case SYS_POWER_STATE_DEEP_SLEEP:
		return "SYS_POWER_STATE_DEEP_SLEEP";
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		return "SYS_POWER_STATE_DEEP_SLEEP_1";
	default:
		return "Unknown state";
	}
}

#if (CONFIG_RTC)
static struct device *rtc_dev;

static void setup_rtc(void)
{
	struct rtc_config cfg;

	/* Configure RTC device. RTC interrupt is used as 'wake event' when we
	 * are in C2LP state.
	 */
	cfg.init_val = 0;
	cfg.alarm_enable = 0;
	cfg.alarm_val = 0;
	cfg.cb_fn = NULL;
	rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	rtc_enable(rtc_dev);
	rtc_set_config(rtc_dev, &cfg);
}

static void set_rtc_alarm(void)
{
	u32_t now = rtc_read(rtc_dev);
	u32_t alarm = now + (RTC_ALARM_SECOND * (TIMEOUT - 1));

	rtc_set_alarm(rtc_dev, alarm);

	/* Wait a few ticks to ensure the 'Counter Match Register' was loaded
	 * with the 'alarm' value.
	 * Refer to the documentation in qm_rtc.h for more details.
	 */
	while (rtc_read(rtc_dev) < now + 5)
		;
}
#elif (CONFIG_COUNTER)
static struct device *counter_dev;

static void setup_counter(void)
{
	volatile u32_t delay = 0;

	counter_dev = device_get_binding("AON_TIMER");

	/* TESTPOINT: Check timer device binding */
	zassert_true(counter_dev, "timer device not found.");

	counter_start(counter_dev);

	/* The AON timer runs from the RTC clock at 32KHz (rather than
	 * the system clock which is 32MHz) so we need to spin for a few cycles
	 * to allow the register change to propagate.
	 */
	for (delay = 5000; delay--;) {
	}
}

static void set_counter_alarm(void)
{
	u32_t timer_initial_value = (RTC_ALARM_SECOND * (TIMEOUT - 1));

	if (counter_set_alarm(counter_dev, NULL,
			      timer_initial_value, NULL)
			      != 0) {
		printk("Periodic Timer was not started yet\n");
	}
}
#elif (CONFIG_GPIO_QMSI_1)
static struct device *gpio_dev;
#define GPIO_INTERRUPT_PIN 4

static void setup_aon_gpio(void)
{
	gpio_dev = device_get_binding("GPIO_1");

	/* TESTPOINT: Check gpio device binding */
	zassert_true(gpio_dev, "gpio device not found.");

	gpio_pin_configure(gpio_dev, GPIO_INTERRUPT_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);
}
#elif (CONFIG_AIO_COMPARATOR)
static struct device *cmp_dev;
#define CMP_INTERRUPT_PIN 13

static void setup_aon_comparator(void)
{
	volatile u32_t delay = 0;

	cmp_dev = device_get_binding("AIO_CMP_0");

	/* TESTPOINT: Check comparator device binding */
	zassert_true(cmp_dev, "comparator device not found.");

	/* Wait for the comparator to be grounded. */
	printk("USER_ACTION: Ground the comparator pin.\n");
	for (delay = 0; delay < 5000000; delay++) {
	}

	aio_cmp_configure(cmp_dev, CMP_INTERRUPT_PIN,
			  AIO_CMP_POL_RISE, 0,
			  NULL, NULL);

	printk("USER_ACTION: Set the comparator pin to 3.3V/1.8V.\n");
}
#endif

static void setup_wake_event(void)
{
#if (CONFIG_RTC)
	set_rtc_alarm();
#elif (CONFIG_COUNTER)
	set_counter_alarm();
#elif (CONFIG_GPIO_QMSI_1)
	printk("USER_ACTION: Press AON_GPIO 4.\n");
#elif (CONFIG_AIO_COMPARATOR)
	setup_aon_comparator();
#endif
}

static void do_soc_sleep(enum power_states state)
{
	int woken_up = 0;
	int i, devices_retval[suspend_device_count];

	setup_wake_event();

	for (i = suspend_device_count - 1; i >= 0; i--) {
		devices_retval[i] = device_set_power_state(suspend_devices[i],
						DEVICE_PM_SUSPEND_STATE);
	}

	_sys_soc_set_power_state(state);

	/*
	 * Before enabling the interrupts, check the wake source
	 * as it will get cleared after.
	 */
#if (CONFIG_RTC)
	woken_up = rtc_get_pending_int(rtc_dev);
	const char source[] = "RTC";
#elif (CONFIG_COUNTER)
	woken_up = counter_get_pending_int(counter_dev);
	const char source[] = "counter";
#elif (CONFIG_GPIO_QMSI_1)
	woken_up = gpio_get_pending_int(gpio_dev);
	const char source[] = "GPIO";
#elif (CONFIG_AIO_COMPARATOR)
	woken_up = aio_cmp_get_pending_int(cmp_dev);
	const char source[] = "AON compare";
#endif

	for (i = 0; i < suspend_device_count; i++) {
		if (!devices_retval[i]) {
			device_set_power_state(suspend_devices[i],
						DEVICE_PM_ACTIVE_STATE);
		}
	}

	if (woken_up) {
		printk("Woke up with %s (pin:%x)\n", source, woken_up);
	}
}

int _sys_soc_suspend(s32_t ticks)
{
	enum power_states state;
	int pm_operation = SYS_PM_NOT_HANDLED;

	post_ops_done = 0;

	if ((ticks != K_FOREVER) && (ticks < MIN_TIME_TO_SUSPEND)) {
		printk("Not enough time for PM operations (" TIME_UNIT_STRING
		       ": %d).\n", ticks);
		return SYS_PM_NOT_HANDLED;
	}

	/* If test is completed then do not enter LPS states */
	if (test_complete) {
		return SYS_PM_NOT_HANDLED;
	}

	state = get_next_state();

	printk("Entering %s state\n", state_to_string(state));

	switch (state) {
	case SYS_POWER_STATE_CPU_LPS:
	case SYS_POWER_STATE_CPU_LPS_1:
#ifdef CONFIG_X86
	case SYS_POWER_STATE_CPU_LPS_2:
#endif
		/*
		 * A wake event is needed in the following cases:
		 *
		 * On Quark SE C1000 x86:
		 * - SYS_POWER_STATE_CPU_LPS:
		 *   The PIC timer is gated and cannot wake the core from
		 *   that state.
		 *
		 * - SYS_POWER_STATE_CPU_LPS_1:
		 *   If the ARC enables LPSS, the PIC timer will
		 *   not wake us up from SYS_POWER_STATE_CPU_LPS_1
		 *   which is mapped to C2.
		 *
		 *   As the ARC enables LPSS, it should as well take care of
		 *   setting up the relevant wake event or communicate
		 *   to the x86 that information.
		 *
		 * On Quark SE C1000 ARC:
		 * - SYS_POWER_STATE_CPU_LPS:
		 *   The ARC timer is gated and cannot wake the core from
		 *   that state.
		 *
		 * - SYS_POWER_STATE_CPU_LPS_1:
		 *   The ARC timer is gated and cannot wake the core from
		 *   that state.
		 */
		setup_wake_event();
		pm_operation = SYS_PM_LOW_POWER_STATE;
		_sys_soc_set_power_state(state);
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		/* Don't need pm idle exit notification */
		_sys_soc_pm_idle_exit_notification_disable();

		pm_operation = SYS_PM_DEEP_SLEEP;
		do_soc_sleep(state);
		break;
	default:
		printk("State not supported\n");
		break;
	}

	if (pm_operation != SYS_PM_NOT_HANDLED) {
		if (!post_ops_done) {
			post_ops_done = 1;
			printk("Exiting %s state\n", state_to_string(state));
			_sys_soc_power_state_post_ops(state);
		}
	}

	return pm_operation;
}

void _sys_soc_resume(void)
{
	enum power_states state = states_list[current_state];

	switch (state) {
	case SYS_POWER_STATE_CPU_LPS:
	case SYS_POWER_STATE_CPU_LPS_1:
#ifdef CONFIG_X86
	case SYS_POWER_STATE_CPU_LPS_2:
#endif
		if (!post_ops_done) {
			post_ops_done = 1;
			printk("Exiting %s state\n", state_to_string(state));
			_sys_soc_power_state_post_ops(state);
		}
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		/* Do not perform post_ops in _sys_soc_resume for deep sleep.
		 * This would make the application task run without the full
		 * context restored.
		 */
		break;
	default:
		break;
	}
}

static void build_suspend_device_list(void)
{
	int i, devcount;
	struct device *devices;

	device_list_get(&devices, &devcount);

	/* TESTPOINT: Check if device list is in tracking range */
	zassert_false((devcount > MAX_SUSPEND_DEVICE_COUNT),
			"Error: List of devices exceeds what we can track "
			"for suspend. Built: %d, Max: %d",
			devcount, MAX_SUSPEND_DEVICE_COUNT);

#if (CONFIG_X86)
	suspend_device_count = 3;
	for (i = 0; i < devcount; i++) {
		if (!strcmp(devices[i].config->name, "loapic")) {
			suspend_devices[0] = &devices[i];
		} else if (!strcmp(devices[i].config->name, "ioapic")) {
			suspend_devices[1] = &devices[i];
		} else if (!strcmp(devices[i].config->name,
				 CONFIG_UART_CONSOLE_ON_DEV_NAME)) {
			suspend_devices[2] = &devices[i];
		} else {
			suspend_devices[suspend_device_count++] = &devices[i];
		}
	}
#elif (CONFIG_ARC)
	suspend_device_count = 2;
	for (i = 0; i < devcount; i++) {
		if (!strcmp(devices[i].config->name, "arc_v2_irq_unit")) {
			suspend_devices[0] = &devices[i];
		} else if (!strcmp(devices[i].config->name, "sys_clock")) {
			suspend_devices[1] = &devices[i];
		} else {
			suspend_devices[suspend_device_count++] = &devices[i];
		}
	}
#endif
}

void test_power_state(void)
{
	int i;

	printk("Quark SE(%s): Power Management sample application\n",
								CONFIG_ARCH);

#if (CONFIG_RTC)
	setup_rtc();
#elif (CONFIG_COUNTER)
	setup_counter();
#elif (CONFIG_GPIO_QMSI_1)
	setup_aon_gpio();
#endif

	build_suspend_device_list();

	/* All our application does is putting the task to sleep so the kernel
	 * triggers the suspend operation.
	 */
	for (i = 0; i < MAX_SYS_PM_STATES; i++) {
		start_time = k_uptime_get();
		k_sleep(TIMEOUT * 1000);
		milliseconds_spent = k_uptime_delta(&start_time);
		printk("Time elapsed from suspend to resume is %lld milliseconds\n",
				milliseconds_spent);
		printk("Back to the application\n\n");
	}
	test_complete = 1;
}

void test_main(void)
{
	ztest_test_suite(test_power_states,
			ztest_unit_test(test_power_state));
	ztest_run_test_suite(test_power_states);
}
