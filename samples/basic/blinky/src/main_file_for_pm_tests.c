/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <inttypes.h>
#include <power/power_state.h>
#include <power/power.h>
#include <soc_power.h>


#define SLEEP_TIME_MS	7000
/*
 * The led0 devicetree alias is optional. If present, we'll use it
 * to turn on the LED whenever the button is pressed.
 */

#define LED0_NODE	DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay) && DT_NODE_HAS_PROP(LED0_NODE, gpios)
#define LED0_GPIO_LABEL	DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED0_GPIO_PIN	DT_GPIO_PIN(LED0_NODE, gpios)
#define LED0_GPIO_FLAGS	(GPIO_OUTPUT | DT_GPIO_FLAGS(LED0_NODE, gpios))
#endif


#ifdef CONFIG_PM


#define SLP_STATES_SUPPORTED      (PM_STATE_SOFT_OFF + 1)

/* Thread properties */
#undef TASK_STACK_SIZE
#define TASK_STACK_SIZE           1024ul
#define PRIORITY                  K_PRIO_COOP(5)

/* Sleep time should be lower than SUSPEND_TO_IDLE */
#define THREAD_A_SLEEP_TIME       100ul
#define THREAD_B_SLEEP_TIME       1000ul

/* Maximum latency should be less than 300 ms */
#define MAX_EXPECTED_MS_LATENCY   500ul

/* Sleep some extra time than minimum residency */
#define DP_EXTRA_SLP_TIME         1000ul
#define LT_EXTRA_SLP_TIME         2000ul

#define SEC_TO_MSEC               1000ul

struct pm_counter {
	uint8_t entry_cnt;
	uint8_t exit_cnt;
};
static int64_t trigger_time;
static bool checks_enabled;
/* Track entry/exit to sleep */
struct pm_counter pm_counters[SLP_STATES_SUPPORTED];

static const struct pm_state_info residency_info[] =
	PM_STATE_INFO_DT_ITEMS_LIST(DT_NODELABEL(cpu0));
static size_t residency_info_len = PM_STATE_DT_ITEMS_LEN(DT_NODELABEL(cpu0));


const struct device *led;


static void pm_latency_check(void)
{
	int64_t latency;
	int secs;
	int msecs;

	latency = k_uptime_delta(&trigger_time);
	secs = (int)(latency / SEC_TO_MSEC);
	msecs = (int)(latency % SEC_TO_MSEC);
	printk("PM sleep entry latency %d.%03d seconds\n", secs, msecs);

}

static void notify_pm_state_entry(enum pm_state state)
{
	if (!checks_enabled) {
		return;
	}

	pm_counters[(int)state].entry_cnt++;
	pm_latency_check();
}

static void notify_pm_state_exit(enum pm_state state)
{
	if (!checks_enabled) {
		return;
	}

	pm_counters[(int)state].exit_cnt++;
}

static struct pm_notifier notifier = {
	.state_entry = notify_pm_state_entry,
	.state_exit = notify_pm_state_exit,
};
static void pm_check_counters(uint8_t cycles)
{
	for (int i = 0; i < SLP_STATES_SUPPORTED; i++) {

		printk("PM state[%d] entry counter %d\n", i,
			pm_counters[i].entry_cnt);
		printk("PM state[%d] exit counter %d\n", i,
			pm_counters[i].exit_cnt);

		zassert_equal(pm_counters[i].entry_cnt,
				pm_counters[i].exit_cnt,
				"PM counters entry/exit mismatch");

		pm_counters[i].entry_cnt = 0;
		pm_counters[i].exit_cnt = 0;
	}
}

static void pm_reset_counters(void)
{
	for (int i = 0; i < SLP_STATES_SUPPORTED; i++) {
		pm_counters[i].entry_cnt = 0;
		pm_counters[i].exit_cnt = 0;
	}

	checks_enabled = false;
}

static void pm_trigger_marker(void)
{
	trigger_time = k_uptime_get();

	printk("PM >\n");
}
static void pm_exit_marker(void)
{
	int64_t residency_delta;
	int secs;
	int msecs;

	printk("PM <\n");

	if (trigger_time > 0) {
		residency_delta = k_uptime_delta(&trigger_time);
		secs = (int)(residency_delta / SEC_TO_MSEC);
		msecs = (int)(residency_delta % SEC_TO_MSEC);
		printk("PM sleep residency %d.%03d seconds\n", secs, msecs);
	}
}

#endif 

    gpc_lpm_config_t config;
    
static bool prev_state=false;

void my_work_handler(struct k_work *work)
{
	prev_state=!prev_state;
	if(prev_state){
 /*       __DSB();
        __ISB();
        __WFI();
		printk("Entered WAIT MODE iA\n");
		GPC_EnterWaitMode(GPC, &config);*/
		printk("Entered WAIT MODE iA\n");
		soc_enter_lpm((enum pm_state)PM_STATE_SUSPEND_TO_IDLE);
	}
	else{
/*        __DSB();
        __ISB();
        __WFI();
		printk("Entered STOP MODE iA\n");
		GPC_EnterStopMode(GPC, &config);*/
		printk("Entered STOP MODE iA\n");
		soc_enter_lpm((enum pm_state)PM_STATE_SUSPEND_TO_RAM);
	}    
}

K_WORK_DEFINE(my_work, my_work_handler);

void my_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&my_work);
}

K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);



/*
 * Get button configuration from the devicetree sw0 alias.
 *
 * At least a GPIO device and pin number must be provided. The 'flags'
 * cell is optional.
 */

#define SW0_NODE	DT_ALIAS(sw0)

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW0_GPIO_LABEL	DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))
#else
#error "Unsupported board: sw0 devicetree alias is not defined"
#define SW0_GPIO_LABEL	""
#define SW0_GPIO_PIN	0
#define SW0_GPIO_FLAGS	0
#endif

/* LED helpers, which use the led0 devicetree alias if it's available. */
static const struct device *initialize_led(void);
static void match_led_to_button(const struct device *button,
				const struct device *led);

static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
//	__enable_irq();
//	GPC->LPCR_M7 = GPC->LPCR_M7 & (~GPC_LPCR_M7_LPM0_MASK);
	soc_enter_lpm((enum pm_state)PM_STATE_ACTIVE);
	printk("Left LP state.\tButton pressed at %" PRIu32 "\n", k_cycle_get_32());
	k_usleep(100);
}

void main(void)
{
   
#ifdef CONFIG_PM

	led = initialize_led();
	
	gpio_pin_set(led, LED0_GPIO_PIN, 1);
	

		
	while(1){
		pm_notifier_register(&notifier);
		
		printk("Entered WAIT MODE iA\n");
		//pm_trigger_marker();
		k_msleep((residency_info[0].min_residency_us / 1000U) +
				 LT_EXTRA_SLP_TIME);
		//pm_exit_marker();
		//printk("Exit WAIT MODE iA\n");
		//k_cpu_idle();
		pm_notifier_unregister(&notifier);
	}
#else

	//TimerInterruptSetup();
		struct k_timer my_timer;
	extern void my_expiry_function(struct k_timer *timer_id);

	k_timer_init(&my_timer, my_timer_handler, NULL);
	
	//k_timer_start(&my_timer, K_SECONDS(4), K_SECONDS(4));

/*	
	 config.enCpuClk              = false;
    config.enFastWakeUp          = false;
    config.enDsmMask             = false;
    config.enWfiMask             = false;
    config.enVirtualPGCPowerdown = true;
    config.enVirtualPGCPowerup   = true;
    
	#define APP_PowerUpSlot (5U)
	#define APP_PowerDnSlot (6U)
	
  	GPC_Init(GPC, APP_PowerUpSlot, APP_PowerDnSlot);
	GPC_EnableIRQ(GPC, GPIO2_Combined_16_31_IRQn);  //GPIO2_20 or SW2
	
*/
	soc_config_lpm(GPIO2_Combined_16_31_IRQn);
    
	const struct device *button;
	const struct device *led;
	int ret;

	button = device_get_binding(SW0_GPIO_LABEL);
	if (button == NULL) {
		printk("Error: didn't find %s device\n", SW0_GPIO_LABEL);
		return;
	}

	ret = gpio_pin_configure(button, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return;
	}

	ret = gpio_pin_interrupt_configure(button,
					   SW0_GPIO_PIN,
					   GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button, &button_cb_data);
	printk("Set up button at %s pin %d\n", SW0_GPIO_LABEL, SW0_GPIO_PIN);

	led = initialize_led();
	gpio_pin_set(led, LED0_GPIO_PIN, 1);

	printk("Press the button\n");
	while (1) {
		//match_led_to_button(button, led);
		printk("Entering WAIT iA\n");
		gpio_pin_set(led, LED0_GPIO_PIN, 0);
		soc_enter_lpm((enum pm_state)PM_STATE_SUSPEND_TO_RAM);
		printk("Entering STOP iA\n");
		gpio_pin_set(led, LED0_GPIO_PIN, 1);
		soc_enter_lpm((enum pm_state)PM_STATE_SUSPEND_TO_IDLE);
	}
#endif
}


#ifdef LED0_GPIO_LABEL
static const struct device *initialize_led(void)
{
	const struct device *led;
	int ret;

	led = device_get_binding(LED0_GPIO_LABEL);
	if (led == NULL) {
		printk("Didn't find LED device %s\n", LED0_GPIO_LABEL);
		return NULL;
	}

	ret = gpio_pin_configure(led, LED0_GPIO_PIN, LED0_GPIO_FLAGS);
	if (ret != 0) {
		printk("Error %d: failed to configure LED device %s pin %d\n",
		       ret, LED0_GPIO_LABEL, LED0_GPIO_PIN);
		return NULL;
	}

	printk("Set up LED at %s pin %d\n", LED0_GPIO_LABEL, LED0_GPIO_PIN);

	return led;
}

static void match_led_to_button(const struct device *button,
				const struct device *led)
{
	bool val;

	val = gpio_pin_get(button, SW0_GPIO_PIN);
	gpio_pin_set(led, LED0_GPIO_PIN, val);
}

#else  /* !defined(LED0_GPIO_LABEL) */
static const struct device *initialize_led(void)
{
	printk("No LED device was defined\n");
	return NULL;
}

static void match_led_to_button(const struct device *button,
				const struct device *led)
{
	return;
}
#endif	/* LED0_GPIO_LABEL */
