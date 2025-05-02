#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

static bool main_init = false;
#define SLEEP_TIME_MS   1000
#define SW1_NODE	DT_ALIAS(sw1)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, {0});
static uint32_t rst_cause;

static int ti_mspm0l2xxx_pm_init(void)
{
	int ret;
	ret = hwinfo_get_reset_cause(&rst_cause);
	if (ret != 0) {
		return ret;
	}

	if (RESET_LOW_POWER_WAKE == rst_cause)
	{
		DL_SYSCTL_releaseShutdownIO();
	}

	return 0;
}
SYS_INIT(ti_mspm0l2xxx_pm_init, POST_KERNEL, 0);

int main(void)
{
	int ret;

	if (RESET_LOW_POWER_WAKE == rst_cause)
	{
		printk("Woke from SHUTDOWN\n");
		k_msleep(SLEEP_TIME_MS);
	}

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_INT_WAKEUP);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return 0;
	}

	printk("Press the button\n");

	main_init = true;
	k_sleep(K_FOREVER);
}

/* custom power policy */
#include <zephyr/pm/policy.h>
#include <zephyr/sys_clock.h>
#include <zephyr/pm/device.h>

extern int32_t max_latency_cyc;

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	int64_t cyc = -1;
	uint8_t num_cpu_states;
	const struct pm_state_info *cpu_states;

	if (main_init == false)
		return NULL;

#ifdef CONFIG_PM_NEED_ALL_DEVICES_IDLE
	if (pm_device_is_any_busy()) {
		return NULL;
	}
#endif

	if (ticks != K_TICKS_FOREVER) {
		cyc = k_ticks_to_cyc_ceil32(ticks);
	}

	num_cpu_states = pm_state_cpu_get_all(cpu, &cpu_states);

	for (int16_t i = (int16_t)num_cpu_states - 1; i >= 0; i--) {
		const struct pm_state_info *state = &cpu_states[i];
		uint32_t min_residency_cyc, exit_latency_cyc;

		/* check if there is a lock on state + substate */
		if (pm_policy_state_lock_is_active(state->state, state->substate_id)) {
			continue;
		}

		min_residency_cyc = k_us_to_cyc_ceil32(state->min_residency_us);
		exit_latency_cyc = k_us_to_cyc_ceil32(state->exit_latency_us);

		/* skip state if it brings too much latency */
		if ((max_latency_cyc >= 0) &&
		    (exit_latency_cyc >= max_latency_cyc)) {
			continue;
		}

		if ((cyc < 0) ||
		    (cyc >= (min_residency_cyc + exit_latency_cyc))) {
			return state;
		}
	}

	return NULL;
}
