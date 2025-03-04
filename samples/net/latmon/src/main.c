/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Philippe Gerum <rpm@xenomai.org>
 * Copyright (c) 2024 Jorge Ramirez-Ortiz <jorge.ramirez@oss.qualcomm.com>
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(latency_monitor, LOG_LEVEL_DBG);

#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/atomic.h>
#include "latmon.h"

#define	INADDR_BROADCAST	((uint32_t) 0xffffffff)

/*
 * Blink Control
 * DHCP: red
 * waiting for connection: blue
 * sampling: green
 */
#define LED_WAIT_PERIOD 1000000
#define LED_DHCP_PERIOD 500000
#define LED_RUN_PERIOD  200000

static const struct gpio_dt_spec *running_led;
static const struct gpio_dt_spec led_run =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led_wait =
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec led_dhcp =
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

#define BLINK_THREAD_PRIORITY K_IDLE_PRIO
#define BLINK_STACK_SIZE 4096
static K_THREAD_STACK_DEFINE(blink_stack, BLINK_STACK_SIZE);
static struct k_thread blink_thread;
static k_tid_t blink_tid;

#define LATMON_PULSE DT_NODELABEL(latmon_pulse)
#define LATMON_ACK DT_NODELABEL(latmon_ack)

#if !DT_NODE_EXISTS(LATMON_PULSE)
#error "Latmon pulse node not properly defined."
#endif

#if !DT_NODE_EXISTS(LATMON_ACK)
#error "Latmon ack node not properly defined."
#endif


PINCTRL_DT_DEFINE(LATMON_PULSE);
PINCTRL_DT_DEFINE(LATMON_ACK);
static const struct gpio_dt_spec pulse =
	GPIO_DT_SPEC_GET_OR(LATMON_PULSE, gpios, {0});
static const struct gpio_dt_spec ack =
	GPIO_DT_SPEC_GET_OR(LATMON_ACK, gpios, {0});
const struct pinctrl_dev_config *pulse_pcfg =
	PINCTRL_DT_DEV_CONFIG_GET(LATMON_PULSE);
const struct pinctrl_dev_config *ack_pcfg =
	PINCTRL_DT_DEV_CONFIG_GET(LATMON_ACK);

#define MONITOR_THREAD_PRIORITY K_PRIO_COOP(8)
#define MONITOR_STACK_SIZE 4096
static K_THREAD_STACK_DEFINE(monitor_stack, MONITOR_STACK_SIZE);
static struct k_thread monitor_thread;
static k_tid_t monitor_tid;
static bool abort_monitor;


static K_SEM_DEFINE(monitor_done, 0, 1);
static K_SEM_DEFINE(dhcp_done, 0, 1);
static K_SEM_DEFINE(ack_event, 0, 1);
static K_MUTEX_DEFINE(stat_mutex);
static struct in_addr local_ip;
/* data */
static uint32_t overruns, current_samples;
static uint32_t max_samples_per_bulk;
static uint32_t min_lat, max_lat;
static uint64_t sum_lat;

/*
 * Note: a small period (ie < 100 usecs) might report great interrupt latencies
 * over a short test due to the caches kept hot.
 */
static uint32_t latmus_period;
static uint32_t latmus_cells;
static int latmus_socket;

/*
 * Each cell represents a 1 usec timespan.
 * note: the sampling period cannot be longer than 1 sec.
 */
#define MAX_SAMPLING_PERIOD 1000000
#define HISTOGRAM_CELLS_MAX 1000
static uint32_t histogram[HISTOGRAM_CELLS_MAX];

/*
 * Discard redundant ACK events since the device under test might
 * lag a bit too long for devalidating our input signal.
 */
static atomic_t ack_wait;

/* Raw ticks */
#define CALCULATE_DELTA(ack, pulse) \
    ((ack) < (pulse) ? \
    (~(pulse) + 1 + (ack)) : ((ack) - (pulse)))
static uint32_t ack_date;

static int setup_pinctrl(void)
{
	int ret = 0;

	if (DT_PINCTRL_HAS_IDX(LATMON_ACK, 0)) {
		ret = pinctrl_apply_state(ack_pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("failed applying ack pin state");
			return ret;
		}
	}

	if (DT_PINCTRL_HAS_IDX(LATMON_PULSE, 0)) {
		ret = pinctrl_apply_state(pulse_pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("failed applying pulse pin state");
			return ret;
		}
	}

	return ret;
}

static void gpio_ack_handler(const struct device *port,
			struct gpio_callback *cb,
			gpio_port_pins_t pins)
{
	if (!atomic_test_bit(&ack_wait, 0))
		return;

	ack_date = k_cycle_get_32();
	if (!atomic_clear(&ack_wait))
		LOG_ERR("unexpected value clearing ack_wait ");

	k_sem_give(&ack_event);
}

static int setup_gpio_pins(void)
{
	static struct gpio_callback gpio_cb = { };
	int ret = 0;

	if (setup_pinctrl() < 0)
		return -ENODEV;

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

	ret = gpio_pin_interrupt_configure_dt(&ack, GPIO_INT_EDGE_RISING);
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

	if (gpio_is_ready_dt(&led_run))
		 gpio_pin_configure_dt(&led_run, GPIO_OUTPUT_INACTIVE);

	if (gpio_is_ready_dt(&led_wait))
		 gpio_pin_configure_dt(&led_wait, GPIO_OUTPUT_INACTIVE);

	if (gpio_is_ready_dt(&led_dhcp))
		 gpio_pin_configure_dt(&led_dhcp, GPIO_OUTPUT_INACTIVE);

	return ret;
}

static ssize_t write_latmus_socket(const void *buf, size_t count)
{
	ssize_t total_written = 0;
	ssize_t bytes_written;

	while (count > 0) {
		const char *p = (const char *)buf + total_written;
		bytes_written = send(latmus_socket ,p ,count, 0);
		if (bytes_written < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (!bytes_written)
			break;

		total_written += bytes_written;
		count -= bytes_written;
	}

    return total_written;
}

static void reset_sampling_counters(void)
{
	current_samples = 0;
	min_lat = UINT_MAX;
	overruns = 0;
	sum_lat = 0;
	max_lat = 0;
}

static int send_sample_data(void)
{
	struct latmon_net_data ndata = { };

	ndata.sum_lat_hi = htonl(sum_lat >> 32);
	ndata.sum_lat_lo = htonl(sum_lat & 0xffffffff);
	ndata.min_lat = htonl(min_lat);
	ndata.max_lat = htonl(max_lat);
	ndata.overruns = htonl(overruns);
	ndata.samples = htonl(current_samples);

	reset_sampling_counters();

	return (write_latmus_socket(&ndata, sizeof(ndata)) <= 0 ? -1 : 0);
}

static int send_trailing_data(void)
{
	int count = latmus_cells * sizeof(histogram[0]);
	ssize_t ret = 0;
	int cell = 0;

	if (current_samples && send_sample_data() < 0)
		return -1;

	if (send_sample_data() < 0)
		return -1;

	for (cell = 0; cell < latmus_cells; cell++)
		histogram[cell] = htonl(histogram[cell]);

	ret = write_latmus_socket(histogram, count);
	if (ret < 0) {
		LOG_INF("failed tx histogram (ret=%d, errno %d)", ret, errno);
		memset(histogram, 0, sizeof(histogram));
		return -1;
	}

	return 0;
}

static void reset_ack_event(void)
{
	k_sem_reset(&ack_event);
	atomic_clear(&ack_wait);
}

static int wait_for_ack(bool *warm)
{
	static uint32_t cnt = 0;

	if (k_sem_take(&ack_event, K_SECONDS(1)) == 0) {
		if (*warm)
			return 0;

		if (++cnt < max_samples_per_bulk)
			return -1;

		*warm = true;
		cnt = 0;

		return 0;
	}

	LOG_ERR("timeout waiting for ack ");

	if (!atomic_clear(&ack_wait))
		LOG_ERR("unexpected value clearing ack_wait ");

	overruns++;

	return -1;

}

static void get_deltas(uint32_t pulse, uint32_t ack,
		       uint32_t *nsecs, uint32_t *usecs)
{
	uint32_t delta = CALCULATE_DELTA(ack, pulse);

	*nsecs = (uint32_t)k_cyc_to_ns_floor64(delta);
	*usecs = *nsecs / 1000;
}

static void update_stats(uint32_t delta_ns)
{
	sum_lat += delta_ns;

	if (delta_ns < min_lat)
		min_lat = delta_ns;

	if (delta_ns > max_lat)
		max_lat = delta_ns;
}

static void update_histogram(uint32_t delta_usecs)
{
	int cell = delta_usecs;

	if (!latmus_cells)
		return;

	if (cell >= latmus_cells) /* Outlier */
		cell = latmus_cells - 1;

	histogram[cell]++;
}

static void update_overruns(uint32_t delta_usecs)
{
	while (delta_usecs > latmus_period) {
		overruns++;
		delta_usecs -= latmus_period;
	}
}

static int prepare_sample_data(uint32_t pulse_date)
{
	uint32_t delta_usecs = 0;
	uint32_t delta_nsecs = 0;

	get_deltas(pulse_date, ack_date, &delta_nsecs, &delta_usecs);

	update_stats(delta_nsecs);
	update_overruns(delta_usecs);
	update_histogram(delta_usecs);

	if (++current_samples < max_samples_per_bulk)
		return -1;

	return 0;
}

static void latmus_sync(void)
{
	if (latmus_period >= 1000)
		k_usleep(latmus_period);
	else
		k_busy_wait(latmus_period);
}

static void blink(void)
{
	const struct gpio_dt_spec *led = NULL, *tmp = NULL;
	uint32_t period = LED_DHCP_PERIOD;

	for (;;) {
		k_usleep(period);
		led = running_led;

		if (tmp && led != tmp)
			gpio_pin_set_dt(tmp, 0);

		if (!gpio_is_ready_dt(led))
			continue;

		if (led == &led_wait)
			period = LED_WAIT_PERIOD;

		if (led == &led_run)
			period = LED_RUN_PERIOD;

		gpio_pin_toggle_dt(led);

		tmp = led;
	}
	gpio_pin_set_dt(led, 0);
}

static uint32_t signal_latmus(void)
{
	unsigned int key = 0;
	uint32_t date = 0;

	latmus_sync();

	/* Latmus should track the falling edge of the signal */
	key = irq_lock();

	/* wait for interrupt now */
	if (atomic_set(&ack_wait, 1))
		LOG_ERR("unexpected value setting ack_wait ");

	/* falling edge */
	gpio_pin_set_dt(&pulse, 0);
	date = k_cycle_get_32();
	irq_unlock(key);
	gpio_pin_set_dt(&pulse, 1);

	return date;
}

static int monitor(void)
{
	bool warm = false;
	uint32_t t = 0;

	LOG_INF("\tmonitoring started");
	reset_ack_event();

	while (!abort_monitor) {
		t = signal_latmus();

		if (wait_for_ack(&warm) < 0)
			continue;

		if (prepare_sample_data(t) < 0)
			continue;

		if (send_sample_data() < 0)
			break;
	}

	k_sem_give(&monitor_done);
	monitor_tid = NULL;
	LOG_INF("\tmonitoring stopped");

	return 0;
}

static int check_period(uint32_t period_usecs)
{
	if (!period_usecs)
		return -1;

	if (period_usecs > MAX_SAMPLING_PERIOD) {
		LOG_INF("invalid period received: %u usecs\n", period_usecs);
		return -1;
	}

	return 0;
}

static int check_histogram_cells(uint32_t histogram_cells)
{
	if (histogram_cells <= HISTOGRAM_CELLS_MAX)
		return 0;

	LOG_INF("invalid histogram size received: %u > %u cells\n",
		histogram_cells, HISTOGRAM_CELLS_MAX);

	return -1;
}

static int check_latmus_request(ssize_t len, struct latmon_net_request *req)
{
	if (len != sizeof(*req))
		return -1;

	if (check_period(ntohl(req->period_usecs)) < 0)
		return -1;

	if (check_histogram_cells(ntohl(req->histogram_cells)) < 0)
		return -1;

	return 0;
}

static void initialize_monitoring(uint32_t period_usecs,
				  uint32_t histogram_cells, int socket)
{
	latmus_period = period_usecs;
	max_samples_per_bulk = MAX_SAMPLING_PERIOD / latmus_period;
	latmus_cells = histogram_cells;

	if (latmus_cells)
		memset(histogram, 0, sizeof(histogram));

	reset_sampling_counters();
	k_sem_reset(&monitor_done);
	abort_monitor = false;
	latmus_socket = socket;
	running_led = &led_run;
}

static void start_monitoring(int socket, struct latmon_net_request *req)
{
	uint32_t period_usecs = ntohl(req->period_usecs);
	uint32_t histogram_cells = ntohl(req->histogram_cells);

	initialize_monitoring(period_usecs, histogram_cells, socket);

	monitor_tid = k_thread_create(&monitor_thread, monitor_stack,
			MONITOR_STACK_SIZE, (k_thread_entry_t)monitor,
			NULL, NULL, NULL,
			MONITOR_THREAD_PRIORITY, 0, K_NO_WAIT);
}

static void stop_monitoring(void)
{
	if (!monitor_tid)
		return;

	abort_monitor = true;
	k_sem_take(&monitor_done, K_FOREVER);

	running_led = &led_wait;
}

static int setup_socket(void)
{
	struct sockaddr_in bind_addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(LATMON_NET_PORT)
	};
	int s, on = 1;

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0) {
		perror("socket");
		exit(1);
	}

	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (bind(s, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
		perror("bind");
		exit(1);
	}

	if (listen(s, 1) < 0) {
		perror("listen");
		exit(1);
	}

	return s;
}

static void dhcp_handler(struct net_mgmt_event_callback *cb,
			uint32_t mgmt_event, struct net_if *iface)
{
	struct net_if_config *cf = &iface->config;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD)
		return;

	for (int n = 0; n < NET_IF_MAX_IPV4_ADDR; n++) {
		if (cf->ip.ipv4->unicast[n].ipv4.addr_type != NET_ADDR_DHCP)
			continue;

		local_ip = cf->ip.ipv4->unicast[n].ipv4.address.in_addr;
		k_sem_give(&dhcp_done);
		break;
	}
}

static void broadcast_ip_address(struct in_addr *ip_addr)
{
	struct sockaddr_in broadcast_addr;
	char ip_str[NET_IPV4_ADDR_LEN];
	int sock;

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create socket");
		return;
	}

	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_port = htons(LATMON_NET_PORT);
	broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	net_addr_ntop(AF_INET, ip_addr, ip_str, sizeof(ip_str));

	if (sendto(sock, ip_str, strlen(ip_str), 0,
		(struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0)
		LOG_ERR("Failed to send broadcast message");

	close(sock);
}

static int wait_for_connection(int socket)
{
	struct pollfd fds[1] = {
		{ .fd = socket, .events = POLLIN },
	};

	LOG_INF("Waiting for connection...");
	running_led = &led_wait;

	int ret = poll(fds, 1, 5000);
	if (ret < 0) {
		LOG_ERR("poll error: %d", errno);
		return -1;
	} else if (ret == 0) {
		broadcast_ip_address(&local_ip);
		return -1;
	}

	return 0;
}

static void setup_network(int *socket, struct net_mgmt_event_callback *mgmt_cb,
			  char *ip_buf)
{
	struct net_if *iface = net_if_get_default();

	LOG_INF("DHCPv4: binding...");
	net_mgmt_init_event_callback(mgmt_cb, dhcp_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(mgmt_cb);

	for (;;) {
		net_dhcpv4_start(iface);
		if (!k_sem_take(&dhcp_done, K_SECONDS(5))) {
			LOG_INF("DHCPv4: done");
			break;
		}
		LOG_INF("\tretrying...");
	}

	*socket = setup_socket();

	LOG_INF("Listening on %s:%d",
		net_addr_ntop(AF_INET, &local_ip, ip_buf, sizeof(ip_buf)),
		(uint32_t)LATMON_NET_PORT);
}

static void handle_connection(int cl)
{
	struct latmon_net_request req;
	ssize_t len;

	for (;;) {
		len = recv(cl, &req, sizeof(req), 0);
		stop_monitoring();
		if (check_latmus_request(len, &req) < 0) {
			if (send_trailing_data() < 0)
				break;

			continue;
		}
		start_monitoring(cl, &req);
	}
	close(cl);
}

static void setup_leds(void)
{
	running_led = &led_dhcp;
	blink_tid = k_thread_create(&blink_thread, blink_stack, BLINK_STACK_SIZE,
				    (k_thread_entry_t)blink, NULL, NULL, NULL,
				    BLINK_THREAD_PRIORITY, 0, K_NO_WAIT);
}

int main(void)
{
	struct net_mgmt_event_callback mgmt_cb;
	char ip_buf[NET_IPV4_ADDR_LEN];
	struct sockaddr_in clnt_addr;
	int socket, client;
	socklen_t len;

	if (setup_gpio_pins() < 0)
		exit(1);

	setup_leds();
	setup_network(&socket, &mgmt_cb, ip_buf);

	for (;;) {
		if (wait_for_connection(socket) < 0)
			continue;

		len = sizeof(clnt_addr);
		client = accept(socket, (struct sockaddr *)&clnt_addr, &len);
		if (client < 0) {
			LOG_INF("Failed accepting new connection...");
			continue;
		}
		handle_connection(client);
	}

	close(socket);
	k_thread_abort(blink_tid);

	return 0;
}
