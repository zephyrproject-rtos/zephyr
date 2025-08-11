/*
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Jorge Ramirez-Ortiz <jorge.ramirez@oss.qualcomm.com>
 */

 #include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(latmon, CONFIG_LATMON_LOG_LEVEL);

#include <zephyr/net/latmon.h>
#include <zephyr/net/socket.h>

/* Latmon < -- > Latmus Interface */
#define LATMON_NET_PORT CONFIG_NET_LATMON_PORT

struct latmon_net_request {
	uint32_t period_usecs;
	uint32_t histogram_cells;
} __packed;

struct latmon_net_data {
	int32_t sum_lat_hi;
	int32_t sum_lat_lo;
	int32_t min_lat;
	int32_t max_lat;
	uint32_t overruns;
	uint32_t samples;
} __packed;

/* Private IPC: Zephyr application to Latmon service */
struct latmon_message {
	net_latmon_measure_t measure_func;
	int latmus; /* latmus connection */
};

K_MSGQ_DEFINE(latmon_msgq, sizeof(struct latmon_message), 2, 4);

/*
 * Note: Using a small period (e.g., less than 100 microseconds) may result in
 * the reporting too good interrupt latencies during a short test due to cache
 * effects.
 */
struct latmus_conf {
	uint32_t max_samples;
	uint32_t period; /* in usecs */
	uint32_t cells;
};

/*
 * Each cell represents a 1 usec timespan.
 * note: the sampling period cannot be longer than 1 sec.
 */
#define MAX_SAMPLING_PERIOD_USEC 1000000
#define HISTOGRAM_CELLS_MAX 1000
struct latmon_data {
	bool warmed; /* sample data can be used */
	uint32_t histogram[HISTOGRAM_CELLS_MAX];
	uint32_t current_samples;
	uint32_t overruns;
	uint32_t min_lat;
	uint32_t max_lat;
	uint64_t sum_lat;
};

/* Message queue for sample data transfers */
K_MSGQ_DEFINE(xfer_msgq, sizeof(struct latmon_data), 10, 4);

/* Network transfer thread: sends data to Latmus  */
#define XFER_THREAD_STACK_SIZE CONFIG_NET_LATMON_XFER_THREAD_STACK_SIZE
#define XFER_THREAD_PRIORITY CONFIG_NET_LATMON_XFER_THREAD_PRIORITY
K_THREAD_STACK_DEFINE(xfer_thread_stack, XFER_THREAD_STACK_SIZE);
static struct k_thread xfer_thread;

/* Latmon thread: receives application requests */
#define LATMON_THREAD_PRIORITY CONFIG_NET_LATMON_THREAD_PRIORITY
#define LATMON_STACK_SIZE CONFIG_NET_LATMON_THREAD_STACK_SIZE

/* Monitor thread: performs the sampling */
#define MONITOR_THREAD_PRIORITY CONFIG_NET_LATMON_MONITOR_THREAD_PRIORITY
#define MONITOR_STACK_SIZE CONFIG_NET_LATMON_MONITOR_THREAD_STACK_SIZE
static K_THREAD_STACK_DEFINE(monitor_stack, MONITOR_STACK_SIZE);

static struct k_thread monitor_thread;
static k_tid_t monitor_tid;
static bool abort_monitor;

/* Synchronization */
static K_SEM_DEFINE(latmon_done, 0, 1);
static K_SEM_DEFINE(monitor_done, 0, 1);

static ssize_t send_net_data(int latmus, const void *buf, size_t count)
{
	ssize_t total_written = 0;
	ssize_t bytes_written;

	while (count > 0) {
		const char *p = (const char *)buf + total_written;

		bytes_written = zsock_send(latmus, p, count, 0);
		if (bytes_written < 0) {
			if (errno == EINTR) {
				continue;
			}
			return -1;
		}
		if (bytes_written == 0) {
			break;
		}

		total_written += bytes_written;
		count -= bytes_written;
	}

	return total_written;
}

static int send_sample_data(int latmus, struct latmon_data *data)
{
	struct latmon_net_data ndata = {
		.sum_lat_lo = htonl(data->sum_lat & 0xffffffff),
		.sum_lat_hi = htonl(data->sum_lat >> 32),
		.samples = htonl(data->current_samples),
		.overruns = htonl(data->overruns),
		.min_lat = htonl(data->min_lat),
		.max_lat = htonl(data->max_lat),
	};

	/* Reset the data */
	data->min_lat = UINT32_MAX;
	data->current_samples = 0;
	data->overruns = 0;
	data->max_lat = 0;
	data->sum_lat = 0;

	return (send_net_data(latmus, &ndata, sizeof(ndata)) <= 0 ? -1 : 0);
}

static int send_trailing_data(int latmus, struct latmus_conf *conf,
			struct latmon_data *data)
{
	int count = conf->cells * sizeof(data->histogram[0]);
	ssize_t ret = 0;

	if (data->current_samples != 0 && send_sample_data(latmus, data) < 0) {
		return -1;
	}

	/* send empty frame */
	if (send_sample_data(latmus, data) < 0) {
		return -1;
	}

	/* send histogram if enabled (ie, conf->cells > 0) */
	for (int cell = 0; cell < conf->cells; cell++) {
		data->histogram[cell] = htonl(data->histogram[cell]);
	}

	ret = send_net_data(latmus, data->histogram, count);
	memset(data->histogram, 0, count);

	if (ret < 0) {
		LOG_INF("failed tx histogram (ret=%d, errno %d)", ret, errno);
		return -1;
	}

	return 0;
}

static int prepare_sample_data(uint32_t delta, struct latmus_conf *conf,
			struct latmon_data *data)
{
	uint32_t delta_ns = k_cyc_to_ns_floor64(delta);
	uint32_t delta_us = delta_ns / 1000;

	data->sum_lat += delta_ns;

	if (delta_ns < data->min_lat) {
		data->min_lat = delta_ns;
	}

	if (delta_ns > data->max_lat) {
		data->max_lat = delta_ns;
	}

	while (delta_us > conf->period) {
		data->overruns++;
		delta_us -= conf->period;
	}

	if (conf->cells != 0) {
		if (delta_us >= conf->cells) {
			/* histogram outlier */
			delta_us = conf->cells - 1;
		}

		data->histogram[delta_us]++;
	}

	return ++data->current_samples < conf->max_samples ? -EAGAIN : 0;
}

static int enqueue_sample_data(struct latmon_data *data)
{
	int ret = 0;

	/* Drop the warming samples */
	if (data->warmed == false) {
		data->warmed = true;
		goto out;
	}

	/* Enqueue the data for transfer */
	ret = k_msgq_put(&xfer_msgq, data, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to enqueue netdata (queue full)");
	}
out:
	/* Reset the data */
	data->min_lat = UINT32_MAX;
	data->current_samples = 0;
	data->overruns = 0;
	data->max_lat = 0;
	data->sum_lat = 0;

	return ret;
}

static void xfer_thread_func(void *p1, void *p2, void *p3)
{
	int latmus = *(int *)p1;
	struct latmon_data sample;

	LOG_INF("Transfer thread priority: %d", XFER_THREAD_PRIORITY);

	for (;;) {
		if (k_msgq_get(&xfer_msgq, &sample, K_FOREVER) != 0) {
			LOG_ERR("Failed to get sample data to transfer");
			continue;
		}

		if (send_sample_data(latmus, &sample) < 0) {
			LOG_ERR("Failed to transfer sample data");
			break;
		}
	}
}

static void start_xfer_thread(int *latmus)
{
	k_thread_create(&xfer_thread, xfer_thread_stack, XFER_THREAD_STACK_SIZE,
			xfer_thread_func, latmus, NULL, NULL,
			XFER_THREAD_PRIORITY, 0, K_MSEC(10));
}

static void abort_xfer_thread(void)
{
	k_thread_abort(&xfer_thread);
}

static int measure(uint32_t *delta, struct latmon_message *msg,
		struct latmon_data *data,
		struct latmus_conf *conf)
{
	if (data->warmed == true) {
		k_usleep(conf->period);
	}

	if (msg->measure_func(delta) < 0) {
		if (data->overruns++ > conf->max_samples / 2) {
			return -1;
		}
		/* Just an overrun */
		return 1;
	}
	return 0;
}

static void monitor_thread_func(void *p1, void *p2, void *p3)
{
	struct latmon_message *msg = p1;
	struct latmus_conf *conf = p2;
	struct latmon_data *data = p3;
	uint32_t delta = 0;
	int ret = 0;

	LOG_INF("Monitor thread priority: %d", MONITOR_THREAD_PRIORITY);

	/* Prepare transfer thread */
	start_xfer_thread(&msg->latmus);

	LOG_INF("\tmonitoring started:");
	LOG_INF("\t - samples per period: %u", conf->max_samples);
	LOG_INF("\t - period: %u usecs", conf->period);
	LOG_INF("\t - histogram cells: %u", conf->cells);

	/* Sampling loop */
	memset(data, 0, sizeof(*data));
	data->min_lat = UINT32_MAX;
	data->warmed = false;
	delta = 0;
	do {
		ret = measure(&delta, msg, data, conf);
		if (ret != 0) {
			if (ret < 0) {
				LOG_ERR("\tExcessive overruns, abort!");
				goto out;
			}
			continue;
		}

		if (prepare_sample_data(delta, conf, data) == -EAGAIN) {
			continue;
		}

		ret = enqueue_sample_data(data);
		/* Abort allowed after all samples have been queued */
	} while (abort_monitor == false && ret == 0);
out:
	abort_xfer_thread();
	k_sem_give(&monitor_done);
	monitor_tid = NULL;

	LOG_INF("\tmonitoring stopped");
}

static int broadcast_ip_address(struct in_addr *ip_addr)
{
	char ip_str[NET_IPV4_ADDR_LEN];
	struct sockaddr_in broadcast;
	int sock = -1;
	int ret = -1;

	if (ip_addr == NULL || ip_addr->s_addr == INADDR_ANY) {
		LOG_ERR("Invalid IP address for broadcast");
		return -1;
	}

	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create broadcast socket : %d", errno);
		return -1;
	}

	broadcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	broadcast.sin_port = htons(LATMON_NET_PORT);
	broadcast.sin_family = AF_INET;

	if (net_addr_ntop(AF_INET, ip_addr, ip_str, sizeof(ip_str)) == NULL) {
		LOG_ERR("Failed to convert IP address to string");
		ret = -1;
		goto out;
	}

	ret = zsock_sendto(sock, ip_str, strlen(ip_str), 0,
			(struct sockaddr *)&broadcast, sizeof(broadcast));

out:
	zsock_close(sock);

	return ret;
}

/* Get a socket to listen to Latmus requests */
int net_latmon_get_socket(struct sockaddr *connection_addr)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(LATMON_NET_PORT)
	};
	int s, on = 1;

	if (connection_addr != NULL) {
		memcpy(&addr, connection_addr, sizeof(addr));
	}

	s = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0) {
		LOG_ERR("failed to create latmon socket : %d", errno);
		return -1;
	}

	zsock_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (zsock_bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		LOG_ERR("failed to bind latmon socket : %d", errno);
		zsock_close(s);
		return -1;
	}

	if (zsock_listen(s, 1) < 0) {
		LOG_ERR("failed to listen on latmon socket : %d", errno);
		zsock_close(s);
		return -1;
	}

	return s;
}

/* Waits for connection from Latmus */
int net_latmon_connect(int socket, struct in_addr *ip)
{
	struct zsock_pollfd fd[1] = { {.fd = socket, .events = ZSOCK_POLLIN } };
	struct sockaddr_in clnt_addr;
	const int timeout = 5000;
	socklen_t len;
	int latmus = -1;
	int ret;

	LOG_INF("Waiting for Latmus ... ");

	/* Broadcast Latmon's address every timeout seconds until connected */
	ret = zsock_poll(fd, 1, timeout);
	if (ret < 0) {
		LOG_ERR("Poll error: %d", errno);
		return -1;
	} else if (ret == 0) {
		/* Timeout waiting for connection */
		if (broadcast_ip_address(ip) < 0) {
			LOG_ERR("Broadcast error");
			return -1;
		}

		/* Client should retry the connection if broadcast succeeded */
		return -EAGAIN;
	}

	/*
	 * As per MISRA guidelines, an 'else' clause is required. However, we
	 * chose to prioritize adherence to the project's code style guidelines.
	 */
	len = sizeof(clnt_addr);
	latmus = zsock_accept(socket, (struct sockaddr *)&clnt_addr, &len);
	if (latmus < 0) {
		LOG_INF("Failed accepting new connection...");
		return -1;
	}

	return latmus;
}

void net_latmon_start(int latmus, net_latmon_measure_t measure_f)
{
	struct latmon_message msg = {
		.measure_func = measure_f,
		.latmus = latmus,
	};

	k_msgq_put(&latmon_msgq, &msg, K_NO_WAIT);
	k_sem_take(&latmon_done, K_FOREVER);
}

bool net_latmon_running(void)
{
	return monitor_tid ? true : false;
}

static int get_latmus_conf(ssize_t len, struct latmon_net_request *req,
			struct latmus_conf *conf)
{
	if (len != sizeof(*req)) {
		return -1;
	}

	if (ntohl(req->period_usecs) == 0) {
		LOG_ERR("null period received, invalid\n");
		return -1;
	}

	if (ntohl(req->period_usecs) > MAX_SAMPLING_PERIOD_USEC) {
		LOG_ERR("invalid period received: %u usecs\n",
			ntohl(req->period_usecs));
		return -1;
	}

	if (ntohl(req->histogram_cells) > HISTOGRAM_CELLS_MAX) {
		LOG_ERR("invalid histogram size received: %u > %u cells\n",
			ntohl(req->histogram_cells), HISTOGRAM_CELLS_MAX);
		return -1;
	}

	conf->period = ntohl(req->period_usecs);
	conf->cells = ntohl(req->histogram_cells);
	conf->max_samples = MAX_SAMPLING_PERIOD_USEC / conf->period;

	return 0;
}

static void start_monitoring(struct latmon_message *msg,
			struct latmus_conf *conf,
			struct latmon_data *data)
{
	k_sem_reset(&monitor_done);
	abort_monitor = false;

	memset(data, 0, sizeof(*data));
	monitor_tid = k_thread_create(&monitor_thread, monitor_stack,
			MONITOR_STACK_SIZE, monitor_thread_func,
			msg, conf, data, MONITOR_THREAD_PRIORITY, 0, K_NO_WAIT);
}

static void stop_monitoring(void)
{
	if (monitor_tid == 0) {
		return;
	}

	abort_monitor = true;
	k_sem_take(&monitor_done, K_FOREVER);
}

static void handle_connection(struct latmon_message *msg)
{
#if (K_HEAP_MEM_POOL_SIZE > 0)
	struct latmus_conf *conf = k_malloc(sizeof(*conf));
	struct latmon_data *data = k_malloc(sizeof(*data));
	struct latmon_net_request req;
	ssize_t len;

	if (conf == 0 || data == 0) {
		LOG_ERR("Failed to allocate memory, check HEAP_MEM_POOL_SIZE");
		goto out;
	}

	memset(conf, 0, sizeof(*conf));

	for (;;) {
		len = zsock_recv(msg->latmus, &req, sizeof(req), 0);
		stop_monitoring();
		if (get_latmus_conf(len, &req, conf) < 0) {
			/* Send the histogram */
			if (send_trailing_data(msg->latmus, conf, data) < 0) {
				break;
			}
			memset(conf, 0, sizeof(*conf));
			continue;
		}
		start_monitoring(msg, conf, data);
	}
out:
	k_free(conf);
	k_free(data);
	zsock_close(msg->latmus);
	k_sem_give(&latmon_done);
#else
	LOG_ERR("No heap configured");
#endif
}

static int latmon_server_thread_func(void *p1, void *p2, void *p3)
{
	struct latmon_message msg = { };

	LOG_INF("Latmon server thread priority: %d", LATMON_THREAD_PRIORITY);

	for (;;) {
		k_msgq_get(&latmon_msgq, &msg, K_FOREVER);

		/* Only latmus can stop the monitoring, so hang in there */
		handle_connection(&msg);
	}

	return 0;
}

K_THREAD_DEFINE(latmon_server_id, LATMON_STACK_SIZE,
		latmon_server_thread_func, NULL, NULL, NULL,
		LATMON_THREAD_PRIORITY, 0, 0);
