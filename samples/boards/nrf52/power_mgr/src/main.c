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
#include <board.h>
#include <device.h>
#include <gpio.h>

#define SECONDS_TO_SLEEP	60

/* In Tickless Kernel mode, time is passed in milliseconds instead of ticks */
#ifdef CONFIG_TICKLESS_KERNEL
#define TICKS_TO_SECONDS_MULTIPLIER 1000
#define TIME_UNIT_STRING "milliseconds"
#else
#define TICKS_TO_SECONDS_MULTIPLIER CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define TIME_UNIT_STRING "ticks"
#endif
#if defined(CONFIG_SYS_POWER_DEEP_SLEEP)
#define MAX_POWER_SAVING_STATE 3
#else
#define MAX_POWER_SAVING_STATE 2
#endif
#define NO_POWER_SAVING_STATE 0

#define MIN_TIME_TO_SUSPEND	((SECONDS_TO_SLEEP * \
				  TICKS_TO_SECONDS_MULTIPLIER) - \
				  (TICKS_TO_SECONDS_MULTIPLIER / 2))
#define GPIO_CFG_SENSE_LOW (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos)

/* To Create Ordered List of devices */
static void create_device_list(void);
/* Suspend Devices based on Ordered List */
static void suspend_devices(void);
/* Resume Devices */
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
static int pm_state;
static struct device *gpiob;

/* change this to use another GPIO port */
#define PORT    SW0_GPIO_NAME
#define PIN	SW0_GPIO_PIN
#define LOW	0

#define DEMO_DESCRIPTION	\
	"Demo Description\n"	\
	"Application creates Idleness, Due to which System Idle Thread is\n"\
	"scheduled and it enters into various Low Power States.\n"\
	"Press Button 1 on Board to wake device from Low Power State.\n"

extern int nrf_gpiote_interrupt_enable(uint32_t mask);
extern void nrf_gpiote_clear_port_event(void);

void configure_ram_retention(void)
{
	/* Place Holder for RAM retention */
	/* Turn Off the RAM Blocks which are not needed */
}

/* Initialize and configure GPIO */
static void gpio_setup(void)
{

	gpiob = device_get_binding(PORT);
	gpio_pin_configure(gpiob, PIN, GPIO_DIR_IN
				| GPIO_PUD_PULL_UP
				| GPIO_CFG_SENSE_LOW);
	nrf_gpiote_clear_port_event();
	/* Enable GPIOTE Port Event */
	nrf_gpiote_interrupt_enable(GPIOTE_INTENSET_PORT_Msk);
	NVIC_EnableIRQ(GPIOTE_IRQn);
}

/* Print Demo Discription String */
static void display_demo_description(void)
{
	printk(DEMO_DESCRIPTION);
}

/* Application main Thread */
void main(void)
{
	printk("\n\n***Power Management Demo on %s****\n", CONFIG_ARCH);
	display_demo_description();

	/* Setup GPIO */
	gpio_setup();
	create_device_list();

	while (1) {
		printk("\nApplication Thread\n");
		/* Create Idleness to make Idle thread run */
		k_sleep(SECONDS_TO_SLEEP * 1000);
	}
}

/* This Function decides which Low Power Mode to Enter based on Idleness.
 * In actual Application this decision can be made using time (ticks)
 * And RTC timer can be programmed to wake the device after time elapsed.
 */
static int check_pm_policy(s32_t ticks)
{
	static int policy = NO_POWER_SAVING_STATE;
	int power_states[] = {SYS_POWER_STATE_MAX, SYS_POWER_STATE_CPU_LPS,
			SYS_POWER_STATE_CPU_LPS_1, SYS_POWER_STATE_DEEP_SLEEP};
	/*
	 * Compare time available with wake latencies and select
	 * appropriate power saving policy
	 * For the demo we will alternate between following policies
	 * 0 = no power saving operation
	 * 1 = low power state Constant Latency
	 * 2 = Low Power State
	 * 3 = System Off State - Device will reset on Exit from this State
	 */
	policy = ++policy > MAX_POWER_SAVING_STATE ?
					NO_POWER_SAVING_STATE : policy;
	/* Pick Platform specific State */
	return power_states[policy];
}

static void low_power_state_exit(void)
{
	/* Perform some Application specific task on Low Power Mode Exit */

	/* Turn on suspended peripherals/clocks as necessary */
	printk("---- Low power state exit !\n");
}

static void deep_sleep_exit(void)
{
	/* Turn on peripherals and restore device states as necessary */
	resume_devices();
	printk("====Deep sleep exit!\n");
}

static int low_power_state_entry(void)
{
	printk("---->Low power state entry ");
	if (pm_state == SYS_POWER_STATE_CPU_LPS) {
		printk("- CONSTANT LATENCY MODE-");
	} else {
		printk("- LOW POWER MODE -");
	}
	configure_ram_retention();
	_sys_soc_set_power_state(pm_state);
	return SYS_PM_LOW_POWER_STATE;
}

static int deep_sleep_entry(void)
{
	printk("===> Entry Into Deep Sleep ==");
	/* Don't need pm idle exit event notification */
	_sys_soc_pm_idle_exit_notification_disable();
	/* Save device states and turn off peripherals as necessary */
	suspend_devices();
	configure_ram_retention();
	_sys_soc_set_power_state(SYS_POWER_STATE_DEEP_SLEEP);
	 /* Exiting from Deep Sleep State */
	deep_sleep_exit();

	return SYS_PM_DEEP_SLEEP;
}

/* Function name : _sys_soc_suspend
 * Return Value : Power Mode Entered Success/Failure
 * Input : Idleness in number of Ticks
 *
 * Description: All request from Idle thread to Enter into
 * Low Power Mode or Deep Sleep State land in this function
 */
int _sys_soc_suspend(s32_t ticks)
{
	int ret = SYS_PM_NOT_HANDLED;
	u32_t level = 0;

	post_ops_done = 0;

	if ((ticks != K_FOREVER) && (ticks < MIN_TIME_TO_SUSPEND)) {
		printk("Not enough time for PM operations " TIME_UNIT_STRING
							" : %d).\n", ticks);
		return SYS_PM_NOT_HANDLED;
	}

	pm_state = check_pm_policy(ticks);

	switch (pm_state) {
	case SYS_POWER_STATE_CPU_LPS:
	case SYS_POWER_STATE_CPU_LPS_1:
		/* Do CPU LPS operations */
		ret = low_power_state_entry();
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
		/* Do deep sleep operations */
		ret = deep_sleep_entry();
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
		 */
		if (!post_ops_done) {
			if ((pm_state == SYS_POWER_STATE_CPU_LPS_1) ||
					(pm_state == SYS_POWER_STATE_CPU_LPS)) {
				low_power_state_exit();
			}
			post_ops_done = 1;
			_sys_soc_power_state_post_ops(pm_state);
		}
	}

	/* DETECT Signal follows GPIO Sense. Wait until GPIO button is pulled
	 * High to make sure that DETECT has gone Low.
	 */
	do {
		gpio_pin_read(gpiob, PIN, &level);
		k_busy_wait(1000);
	} while (level == LOW);

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

	for (int i = device_count - 1; i >= 0; i--) {
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
	 */
	device_list_get(&device_list, &count);
	device_count = 4; /* Reserve for 32KHz, 16MHz, system clock, and uart */

	for (i = 0; (i < count) && (device_count < DEVICE_POLICY_MAX); i++) {
		if (!strcmp(device_list[i].config->name, "clk_k32src")) {
			device_ordered_list[0] = i;
		} else if (!strcmp(device_list[i].config->name, "clk_m16src")) {
			device_ordered_list[1] = i;
		} else if (!strcmp(device_list[i].config->name, "sys_clock")) {
			device_ordered_list[2] = i;
		} else if (!strcmp(device_list[i].config->name, "UART_0")) {
			device_ordered_list[3] = i;
		} else {
			device_ordered_list[device_count++] = i;
		}
	}
}

