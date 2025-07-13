/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Jorge Ramirez-Ortiz <jorge.ramirez@oss.qualcomm.com>
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_latmon, LOG_LEVEL_DBG);

#include <zephyr/drivers/gpio.h>
#include <zephyr/net/latmon.h>
#include <zephyr/net/socket.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
/*
 * Blink Control
 * DHCP: red
 * waiting for connection: blue
 * sampling: green
 */
#define LED_WAIT_PERIOD 1000000
#define LED_DHCP_PERIOD 500000
#define LED_RUN_PERIOD  200000

#define BLINK_THREAD_PRIORITY K_IDLE_PRIO
#define BLINK_STACK_SIZE 4096
static K_THREAD_STACK_DEFINE(blink_stack, BLINK_STACK_SIZE);

static const struct gpio_dt_spec pulse =
	GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), pulse_gpios, {0});
static const struct gpio_dt_spec ack =
	GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), ack_gpios, {0});

static K_SEM_DEFINE(ack_event, 0, 1);

#define DHCP_DONE		(atomic_test_bit(&dhcp_done, 0) == true)
#define SET_DHCP_DONE		atomic_set_bit(&dhcp_done, 0)
static atomic_val_t dhcp_done;

static struct k_spinlock lock;

static void gpio_ack_handler(const struct device *port,
			     struct gpio_callback *cb,
			     gpio_port_pins_t pins)
{
	k_sem_give(&ack_event);
}

static int configure_measurement_hardware(void)
{
	static struct gpio_callback gpio_cb = { };
	int ret = 0;

	if (!gpio_is_ready_dt(&pulse) || !gpio_is_ready_dt(&ack)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&pulse, GPIO_OUTPUT_HIGH);
	if (ret < 0) {
		LOG_ERR("failed configuring pulse pin");
		return ret;
	}

	ret = gpio_pin_configure_dt(&ack, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("failed configuring ack pin");
		return ret;
	}

#if defined(CONFIG_LATMON_LOOPBACK_CALIBRATION)
	/*
	 * Connect GPIO pins in loopback mode for validation (tx to ack)
	 * On FRDM_K64F, Latmus will show around 3.2 usec of latency.
	 *
	 * You can then use these values to adjust the reported latencies (ie,
	 * subtract the loopback latency from the measured latencies).
	 */
	ret = gpio_pin_interrupt_configure_dt(&ack, GPIO_INT_EDGE_FALLING);
#else
	ret = gpio_pin_interrupt_configure_dt(&ack, GPIO_INT_EDGE_RISING);
#endif
	if (ret < 0) {
		LOG_ERR("failed configuring ack pin interrupt");
		return ret;
	}

	gpio_init_callback(&gpio_cb, gpio_ack_handler, BIT(ack.pin));

	ret = gpio_add_callback_dt(&ack, &gpio_cb);
	if (ret < 0) {
		LOG_ERR("failed adding ack pin callback");
		return ret;
	}

	return ret;
}

static void blink(void*, void*, void*)
{
	const struct gpio_dt_spec led_run =
		GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
	const struct gpio_dt_spec led_wait =
		GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});
	const struct gpio_dt_spec led_dhcp =
		GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0});
	const struct gpio_dt_spec *led = &led_dhcp, *tmp = NULL;
	uint32_t period = LED_DHCP_PERIOD;

	if (gpio_is_ready_dt(&led_run)) {
		gpio_pin_configure_dt(&led_run, GPIO_OUTPUT_INACTIVE);
	}

	if (gpio_is_ready_dt(&led_wait)) {
		gpio_pin_configure_dt(&led_wait, GPIO_OUTPUT_INACTIVE);
	}

	if (gpio_is_ready_dt(&led_dhcp)) {
		gpio_pin_configure_dt(&led_dhcp, GPIO_OUTPUT_INACTIVE);
	}

	for (;;) {
		k_usleep(period);
		if (DHCP_DONE) {
			led = net_latmon_running() ? &led_run : &led_wait;
		}

		if (tmp && led != tmp) {
			gpio_pin_set_dt(tmp, 0);
		}

		if (!gpio_is_ready_dt(led)) {
			continue;
		}

		if (led == &led_wait) {
			period = LED_WAIT_PERIOD;
		}

		if (led == &led_run) {
			period = LED_RUN_PERIOD;
		}

		gpio_pin_toggle_dt(led);
		tmp = led;
	}
	gpio_pin_set_dt(led, 0);
}

static k_tid_t start_led_blinking_thread(struct k_thread *blink_thread,
					k_thread_entry_t blink_thread_func)
{
	return k_thread_create(blink_thread, blink_stack, BLINK_STACK_SIZE,
				(k_thread_entry_t)blink_thread_func,
				NULL, NULL, NULL,
				BLINK_THREAD_PRIORITY, 0, K_NO_WAIT);
}

/* Raw ticks */
#define CALCULATE_DELTA(ack, pulse) \
((ack) < (pulse) ? \
(~(pulse) + 1 + (ack)) : ((ack) - (pulse)))

static int measure_latency_cycles(uint32_t *delta)
{
	k_spinlock_key_t key;
	uint32_t tx = 0;
	uint32_t rx = 0;
	int ret = 0;

	/* Remove spurious events */
	k_sem_reset(&ack_event);

	/* Generate a falling edge pulse to the DUT */
	key = k_spin_lock(&lock);
	if (gpio_pin_set_dt(&pulse, 0)) {
		k_spin_unlock(&lock, key);
		LOG_ERR("Failed to set pulse pin");
		ret = -1;
		goto out;
	}
	tx = k_cycle_get_32();
	k_spin_unlock(&lock, key);

	/* Wait for a rising edge from the Latmus controlled DUT */
	if (k_sem_take(&ack_event, K_MSEC(1)) == 0) {
		rx = k_cycle_get_32();
		/* Measure the cycles */
		*delta = CALCULATE_DELTA(rx, tx);
	} else {
		ret = -1;
	}
out:
	if (gpio_pin_set_dt(&pulse, 1)) {
		LOG_ERR("Failed to clear pulse pin");
		ret = -1;
	}

	return ret;
}

int main(void)
{
	struct net_if *iface = net_if_get_default();
	struct k_thread blink_thread;
	static k_tid_t blink_tid;
	int client, socket = 0;
	int ret = 0;

	/* Prepare the instrumentation */
	if (configure_measurement_hardware() < 0) {
		LOG_ERR("Failed to configure the measurement hardware");
		return -1;
	}

	/* Start visual indicators - dhcp/blue, waiting/red, running/green */
	blink_tid = start_led_blinking_thread(&blink_thread, blink);
	if (!blink_tid) {
		LOG_WRN("Failed to start led blinking thread");
	}

	/* Get a valid ip */
	LOG_INF("DHCPv4: binding...");
	net_dhcpv4_start(iface);
	for (;;) {
		ret = net_mgmt_event_wait(NET_EVENT_IPV4_DHCP_BOUND, NULL,
					  NULL, NULL, NULL, K_SECONDS(10));
		if (ret == -ETIMEDOUT) {
			LOG_WRN("DHCPv4: binding timed out, retrying...");
			continue;
		}
		if (ret < 0) {
			LOG_ERR("DHCPv4: binding failed, aborting...");
			goto out;
		}
		break;
	}

	SET_DHCP_DONE;

	/* Get a socket to the Latmus port */
	socket = net_latmon_get_socket(NULL);
	if (socket < 0) {
		LOG_ERR("Failed to get a socket to latmon (errno %d)", socket);
		ret = -1;
		goto out;
	}

	for (;;) {
		/* Wait for Latmus to connect */
		client = net_latmon_connect(socket,
					    &iface->config.dhcpv4.requested_ip);
		if (client < 0) {
			if (client == -EAGAIN) {
				continue;
			}
			LOG_ERR("Failed to connect to latmon");
			ret = -1;
			goto out;
		}

		/* Provide latency data until Latmus closes the connection */
		net_latmon_start(client, measure_latency_cycles);
	}
out:
	k_thread_abort(blink_tid);
	close(socket);

	return ret;
}
