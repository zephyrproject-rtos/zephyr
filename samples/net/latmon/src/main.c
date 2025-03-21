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

static K_SEM_DEFINE(dhcp_done, 0, 1);
static K_SEM_DEFINE(ack_event, 0, 1);

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

#if defined(LATMON_LOOPBACK_DBG)
	/*
	 * Connect GPIO pins in loopback mode for validation (tx to ack)
	 * On FRDM_K64F, Latmus will show around 3.2 usec of latency.
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

static struct in_addr *get_dhcp_address(void)
{
	struct net_if_config *cf = &net_if_get_default()->config;
	static struct in_addr *ip;

	for (int n = 0; !ip && n < NET_IF_MAX_IPV4_ADDR; n++) {
		if (cf->ip.ipv4->unicast[n].ipv4.addr_type == NET_ADDR_DHCP) {
			ip = &cf->ip.ipv4->unicast[n].ipv4.address.in_addr;
		}
	}

	return ip;
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
		if (get_dhcp_address()) {
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

static void dhcp_handler(struct net_mgmt_event_callback *cb,
			uint32_t mgmt_event, struct net_if *iface)
{
	struct net_if_config *cf = &iface->config;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (int n = 0; n < NET_IF_MAX_IPV4_ADDR; n++) {
		if (cf->ip.ipv4->unicast[n].ipv4.addr_type != NET_ADDR_DHCP) {
			continue;
		}
		k_sem_give(&dhcp_done);
		return;
	}
}

static struct in_addr *acquire_dhcp_address(struct net_mgmt_event_callback *cb,
				char *ip_buf)
{
	struct net_if *iface = net_if_get_default();

	LOG_INF("DHCPv4: binding...");
	net_mgmt_init_event_callback(cb, dhcp_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(cb);

	for (;;) {
		net_dhcpv4_start(iface);
		if (!k_sem_take(&dhcp_done, K_SECONDS(5))) {
			LOG_INF("DHCPv4: done");
			break;
		}
		LOG_INF("\tretrying...");
	}

	return get_dhcp_address();
}

/* Raw ticks */
#define CALCULATE_DELTA(ack, pulse) \
((ack) < (pulse) ? \
(~(pulse) + 1 + (ack)) : ((ack) - (pulse)))

static int measure_latency_cycles(uint32_t *delta)
{
	unsigned int key = 0;
	uint32_t tx = 0;
	uint32_t rx = 0;
	int ret = 0;

	/* Remove spurious events */
	k_sem_reset(&ack_event);

	/* Generate a falling edge pulse to the DUT */
	key = irq_lock();
	if (gpio_pin_set_dt(&pulse, 0)) {
		irq_unlock(key);
		LOG_ERR("Failed to set pulse pin");
		ret = -1;
		goto out;
	}
	tx = k_cycle_get_32();
	irq_unlock(key);

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
	struct net_mgmt_event_callback mgmt_cb;
	char ip_buf[NET_IPV4_ADDR_LEN];
	struct k_thread blink_thread;
	static k_tid_t blink_tid;
	struct in_addr *ip;
	int client, socket = 0;

	/* Prepare the instrumentation */
	if (configure_measurement_hardware() < 0) {
		LOG_ERR("failed to configure the measurement hardware");
		return -1;
	}

	/* Start visual indicators - dhcp/blue, waiting/red, running/green */
	blink_tid = start_led_blinking_thread(&blink_thread, blink);
	if (!blink_tid) {
		LOG_WRN("failed to start led blinking thread");
	}

	/* Get a valid ip */
	ip = acquire_dhcp_address(&mgmt_cb, ip_buf);
	if (!ip) {
		LOG_ERR("failed to acquire dhcp address");
		return -1;
	}

	LOG_INF("Listening on %s", net_addr_ntop(AF_INET, ip, ip_buf,
		sizeof(ip_buf)));

	/* Get a socket to the Latmus port */
	socket = net_latmon_get_socket();
	if (socket < 0) {
		LOG_ERR("failed to get a socket to latmon (errno %d)", socket);
		goto out;
	}

	for (;;) {
		/* Wait for Latmus to connect */
		client = net_latmon_connect(socket, ip);
		if (client < 0) {
			if (client == -EAGAIN) {
				continue;
			}
			LOG_ERR("failed to connect to latmon");
			goto out;
		}

		/* Provide latency data until Latmus closes the connection */
		net_latmon_start(client, measure_latency_cycles);
	}
out:
	k_thread_abort(blink_tid);
	close(socket);

	return 0;
}
