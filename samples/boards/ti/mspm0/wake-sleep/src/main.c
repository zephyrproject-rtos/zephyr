#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

K_SEM_DEFINE(shutdown_sem, 0, 1); /* starts off "not available" */
static bool main_init = false;
#define SLEEP_TIME_MS   1000

#define SW0_NODE	DT_ALIAS(sw0) /* shutdown pin */
#define SW1_NODE	DT_ALIAS(sw1) /* wake pin */

/* shutdown pin */
#if 0
static const struct gpio_dt_spec shut_pin = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
#else
static const struct gpio_dt_spec shut_pin = GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, {0});
#endif
static struct gpio_callback button_cb_data;

/* wake */
static const struct gpio_dt_spec wake_pin = GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, {0});

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	k_sem_give(&shutdown_sem);
}

int main(void)
{
	int ret;
	uint32_t rst_cause;
	main_init = false;

	if (!gpio_is_ready_dt(&wake_pin)) {
		printk("Error: button device %s is not ready\n",
		       wake_pin.port->name);
		return 0;
	}

	printk("PIN LINE %u: %u\n", __LINE__, gpio_pin_get_dt(&wake_pin));
	ret = hwinfo_get_reset_cause(&rst_cause);
	if (ret != 0) {
		return ret;
	}

	if (RESET_LOW_POWER_WAKE == rst_cause)
	{
		printk("Woke from SHUTDOWN\n");
		ret = gpio_pin_configure_dt(&wake_pin, GPIO_DISCONNECTED | GPIO_INT_WAKEUP);
		if (ret != 0) {
			printk("Unable to disable wakeup: %d\n", ret);
		}

		ret = gpio_pin_configure_dt(&shut_pin, GPIO_INPUT);
		if (ret != 0) {
			printk("Unable to config INPUT only: %d\n", ret);
		}
		ret = gpio_pin_interrupt_configure_dt(&shut_pin,
				GPIO_INT_EDGE_TO_ACTIVE);
		if (ret != 0) {
			printk("Error %d: failed to configure interrupt on %s pin %d\n",
					ret, shut_pin.port->name, shut_pin.pin);
			return 0;
		}

		gpio_init_callback(&button_cb_data, button_pressed, BIT(shut_pin.pin));
		gpio_add_callback(shut_pin.port, &button_cb_data);
		printk("Press the button to SHUTDOWN %" PRIu32 "\n", k_cycle_get_32());

		k_msleep(SLEEP_TIME_MS);
		k_sem_take(&shutdown_sem, K_FOREVER);
		//ret = gpio_pin_interrupt_configure_dt(&wake_pin, GPIO_INT_DISABLE);
		k_msleep(SLEEP_TIME_MS);
	}

	printk("PIN LINE %u: %u\n", __LINE__, gpio_pin_get_dt(&wake_pin));
	ret = gpio_pin_configure_dt(&wake_pin, GPIO_INPUT | GPIO_INT_WAKEUP);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, wake_pin.port->name, wake_pin.pin);
		return 0;
	}

	printk("PIN LINE %u: %u\n", __LINE__, gpio_pin_get_dt(&wake_pin));
	printk("Press the button to WAKE %" PRIu32 "\n", k_cycle_get_32());

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
