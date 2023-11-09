/*
 * Copyright (c) 2023 Sendrato B.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "nordic-nrf9160.h"
#include <zephyr/drivers/modem/nordic-nrf9160.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem, CONFIG_MODEM_LOG_LEVEL);

/* Static DNS buffers */
static struct zsock_addrinfo dns_result;
static struct sockaddr dns_result_addr;
static char dns_result_canonname[DNS_MAX_NAME_SIZE + 1];

/*
 * Modem RX ringbuffer
 * TODO: We'll need a ring buffer for each socket, not done at the moment
 *       as the application uses only one and to reduce memory usage.
 */
RING_BUF_DECLARE(rx_ringbuf, CONFIG_MODEM_NORDIC_NRF9160_RX_RINGBUF_SIZE);

/* Private work queue */
static struct k_work_q _modem_workq;
K_THREAD_STACK_DEFINE(_modem_workq_stack_area, CONFIG_MODEM_NORDIC_NRF9160_WORKQ_STACK_SIZE);

/* ~~~~ Functions pre-declaration ~~~~ */

static void modem_bus_pipe_handler(struct modem_pipe *pipe, enum modem_pipe_event event,
				   void *user_data);
static void modem_enter_state(struct modem_data *data, enum modem_state state);

static int do_iface_enable(struct modem_data *data);
static int do_iface_disable(struct modem_data *data);

static int offload_gnss(struct modem_data *data, bool enable);

static int do_get_addrinfo(struct modem_data *data);

static int do_socket_open(struct modem_data *data);
static int do_socket_close(struct modem_data *data);
static int do_socket_connect(struct modem_data *data);
static int do_data_mode(struct modem_data *data);
static ssize_t do_socket_send(struct modem_data *data);
static int do_socket_recv(struct modem_data *data);
static int do_get_active_socket(struct modem_data *data);
static int do_select_socket(struct modem_data *data);

/* ~~~~ Utils functions ~~~~ */

static inline uint32_t hash32(const char *str, int len)
{
#define HASH_MULTIPLIER 37

	uint32_t h = 0;
	int i;

	for (i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}

	return h;
}

static inline uint8_t *modem_get_mac(const struct device *dev)
{
	struct modem_data *data = dev->data;
	uint32_t hash_value;

	data->iface.mac_addr[0] = 0x00;
	data->iface.mac_addr[1] = 0x10;

	/* use IMEI for mac_addr */
	hash_value = hash32(data->imei, (int)strlen(data->imei));

	UNALIGNED_PUT(hash_value, (uint32_t *)(data->iface.mac_addr + 2));

	return data->iface.mac_addr;
}

/*
 * Function called by offloaded APIs to wait for the semaphore sem_script_done to
 * be release <count> times, waiting for <timeout> seconds.
 * The semaphore is reset every time before waiting for it to be released.
 * After successfully taking the semaphore the dynamic_script_res variable
 * is checked to verify the script success or failure.
 */
static int wait_script_done(const char *func, struct modem_data *data, int timeout, uint8_t count)
{
	int ret = 0;

	/* Make sure only one thread is waiting for script done semaphores */
	k_sem_take(&data->sem_script_sync, K_FOREVER);

	for (int i = 0; i < count; i++) {
		/* Reset semaphore before waiting for it to be released */
		k_sem_reset(&data->sem_script_done);

		ret = k_sem_take(&data->sem_script_done, K_SECONDS(timeout));
		if (ret < 0) {
			LOG_ERR("%s: Failed to take script done sem (%d of %d), error %d", func, i,
				count, ret);
			break;
		} else {
			/*
			 * Script execution done.
			 * Return dynamic_script_res as it will
			 * contain an error if the script finished with
			 * event MODEM_EVENT_SCRIPT_FAILED
			 */
			ret = data->dynamic_script_res;
		}
	}

	k_sem_give(&data->sem_script_sync);

	return ret;
}

/* IP address to string */
int sprint_ip_addr(const struct sockaddr *addr, char *buf, size_t buf_size)
{
	static const char unknown_str[] = "unk";

	if (addr->sa_family == AF_INET6) {
		if (buf_size < NET_IPV6_ADDR_LEN) {
			return -ENOMEM;
		}

		if (net_addr_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, buf, buf_size) == NULL) {
			return -ENOMEM;
		}
		return 0;
	}

	if (addr->sa_family == AF_INET) {
		if (buf_size < NET_IPV4_ADDR_LEN) {
			return -ENOMEM;
		}
		if (net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr, buf, buf_size) == NULL) {
			return -ENOMEM;
		}
		return 0;
	}

	LOG_ERR("Unknown IP address family:%d", addr->sa_family);

	if (buf_size < sizeof(unknown_str)) {
		return -ENOMEM;
	}
	strcpy(buf, unknown_str);
	return 0;
}

/* Get port from IP address */
int get_addr_port(const struct sockaddr *addr, uint16_t *port)
{
	if (!addr || !port) {
		return -EINVAL;
	}

	if (addr->sa_family == AF_INET6) {
		*port = ntohs(net_sin6(addr)->sin6_port);
		return 0;
	} else if (addr->sa_family == AF_INET) {
		*port = ntohs(net_sin(addr)->sin_port);
		return 0;
	}

	return -EPROTONOSUPPORT;
}

/*
 * Func: modem_atoi
 * Desc: Convert string to integer handling errors
 */
static int modem_atoi(const char *s, const int err_value, const char *desc, const char *func)
{
	int ret;
	char *endptr;

	ret = (int)strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", s, desc, func);
		return err_value;
	}

	return ret;
}

/*
 * Func: modem_atol
 * Desc: Convert string to unsigned integer handling errors
 */
static int modem_atol(const char *s, const char *desc, uint32_t *res, const char *func)
{
	int ret = 0;
	char *endptr;

	*res = strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", s, desc, func);
		ret = -1;
	}

	return ret;
}

/* Function to convert a string containing floating point number to unsigned int (float * 10^6) */
static int str_float_to_uint32(char *str, uint32_t *res)
{
	int ret = 0;
	size_t len = strlen(str);
	int idx = 0;

	/* Look for the dot */
	for (; idx < len; idx++) {
		if (str[idx] == '.') {
			break;
		}
	}
	/* Check if we actually found it, can't be the last char */
	if (idx == (len - 1)) {
		ret = -1;
		goto error;
	}

	/* Calculate number of  decimal digits */
	int dec_digits = (int)strlen(str) - (idx + 1);
	/* Override the dot */
	memcpy(&str[idx], &str[idx + 1], dec_digits);
	/* Override last char with string terminator */
	str[len - 1] = '\0';

	/* Cast string to unsigned integer */
	ret = ATOL(str, "tmp", res);
	if (ret < 0) {
		LOG_ERR("Failed to convert string to unsigned int");
	}

error:
	return ret;
}

/* Parse string containing date and time: "yyyy-mm-dd hh-mm-ss" into struct gnss_time */
static int parse_date_time_str(char *str, struct gnss_time *res)
{
	int ret = 0;

	/* Look for dash between year and month */
	char *ref = strstr(str, "-");
	if (ref == NULL) {
		LOG_ERR("Couldn't find first dash");
		ret = -EINVAL;
		goto error;
	}

	char year_str[3] = {0};
	memcpy(year_str, ref - 2, 2);

	res->century_year = ATOI(year_str, -1, "year");
	if (res->century_year < 0) {
		LOG_ERR("Failed to convert year to int");
		ret = -EINVAL;
		goto error;
	}

	/* Look for dash between month and day */
	ref = strstr(ref + 1, "-");
	if (ref == NULL) {
		LOG_ERR("Couldn't find second dash");
		ret = -EINVAL;
		goto error;
	}

	char month_str[3] = {0};
	memcpy(month_str, ref - 2, 2);

	res->month = ATOI(month_str, -1, "month");
	if (res->month < 0) {
		LOG_ERR("Failed to convert month to int");
		ret = -EINVAL;
		goto error;
	}

	/* Look for space between day and hour */
	ref = strstr(ref + 1, " ");
	if (ref == NULL) {
		LOG_ERR("Couldn't find space");
		ret = -EINVAL;
		goto error;
	}

	char day_str[3] = {0};
	memcpy(day_str, ref - 2, 2);

	res->month_day = ATOI(day_str, -1, "day");
	if (res->month_day < 0) {
		LOG_ERR("Failed to convert day to int");
		ret = -EINVAL;
		goto error;
	}

	/* Look for colon between hour and minutes */
	ref = strstr(ref + 1, ":");
	if (ref == NULL) {
		LOG_ERR("Couldn't find first colon");
		ret = -EINVAL;
		goto error;
	}

	char hour_str[3] = {0};
	memcpy(hour_str, ref - 2, 2);

	res->hour = ATOI(hour_str, -1, "hour");
	if (res->hour < 0) {
		LOG_ERR("Failed to convert hour to int");
		ret = -EINVAL;
		goto error;
	}

	/* Look for colon between minutes and seconds */
	ref = strstr(ref + 1, ":");
	if (ref == NULL) {
		LOG_ERR("Couldn't find second colon");
		ret = -EINVAL;
		goto error;
	}

	char minutes_str[3] = {0};
	memcpy(minutes_str, ref - 2, 2);

	res->minute = ATOI(minutes_str, -1, "minutes");
	if (res->minute < 0) {
		LOG_ERR("Failed to convert minutes to int");
		ret = -EINVAL;
		goto error;
	}

	/* Look for " at the end of the string */
	ref = strstr(ref + 1, "\"");
	if (ref == NULL) {
		LOG_ERR("Couldn't find \" at end of string");
		ret = -EINVAL;
		goto error;
	}

	char seconds_str[3] = {0};
	memcpy(seconds_str, ref - 2, 2);

	res->millisecond = ATOI(seconds_str, -1, "seconds");
	res->millisecond *= 1000;
	if (res->millisecond < 0) {
		LOG_ERR("Failed to convert seconds to int");
		ret = -EINVAL;
		goto error;
	}

error:
	return ret;
}

/*
 * Thread safe function to get modem state
 */
static enum modem_state modem_state_get(struct modem_data *data)
{
	enum modem_state state;

	k_sem_take(&data->sem_state, K_FOREVER);
	state = data->state;
	k_sem_give(&data->sem_state);

	return state;
}

/*
 * Thread safe function to get modem connection state
 */
static bool modem_connected_get(struct modem_data *data)
{
	bool connected = false;

	k_sem_take(&data->sem_state, K_FOREVER);

	connected = data->connected;

	k_sem_give(&data->sem_state);

	return connected;
}

/*
 * Thread safe function to set modem connection state
 */
static void modem_connected_set(struct modem_data *data, bool connected)
{
	k_sem_take(&data->sem_state, K_FOREVER);

	data->connected = connected;

	LOG_INF("Modem %s!", connected ? "connected" : "disconnected");

	k_sem_give(&data->sem_state);
}

/* ~~~~ Modem FSM functions ~~~~ */

static void modem_ready_handler(struct modem_data *data, enum modem_event evt)
{
	LOG_DBG("%s evt %d", __func__, evt);

	switch (evt) {
	case MODEM_EVENT_SUSPEND:
		modem_enter_state(data, MODEM_STATE_IDLE);
		break;
	case MODEM_EVENT_SCRIPT_SUCCESS:
	case MODEM_EVENT_SCRIPT_FAILED:
		LOG_DBG("Script %s", evt == MODEM_EVENT_SCRIPT_SUCCESS ? "success" : "failed");

		/* Set dynamic script result */
		if (evt == MODEM_EVENT_SCRIPT_SUCCESS) {
			data->dynamic_script_res = 0;
		} else {
			data->dynamic_script_res = -EIO;
		}

		/* Give script done semaphore */
		k_sem_give(&data->sem_script_done);
		/* Give script execution semaphore */
		k_sem_give(&data->sem_script_exec);
		break;
	default:
		LOG_DBG("%s got %d, not handled", __func__, evt);
		break;
	}
}

static int modem_init_state_enter(struct modem_data *data)
{
	modem_pipe_attach(data->uart_pipe, modem_bus_pipe_handler, data);
	return modem_pipe_open_async(data->uart_pipe);
}

static void modem_init_handler(struct modem_data *data, enum modem_event evt)
{
	LOG_DBG("%s evt %d", __func__, evt);

	const struct modem_config *config = (const struct modem_config *)data->dev->config;

	switch (evt) {
	case MODEM_EVENT_BUS_OPENED:
		modem_chat_attach(&data->chat, data->uart_pipe);
		modem_chat_run_script_async(&data->chat, config->init_chat_script);
		break;
	case MODEM_EVENT_SCRIPT_SUCCESS:
		/* Give script done semaphore */
		k_sem_give(&data->sem_script_done);
		modem_enter_state(data, MODEM_STATE_READY);
		break;
	case MODEM_EVENT_SCRIPT_FAILED:
	case MODEM_EVENT_SUSPEND:
		modem_enter_state(data, MODEM_STATE_IDLE);
		break;
	default:
		LOG_DBG("%s got %d, not handled", __func__, evt);
		break;
	}
}

static int modem_idle_state_enter(struct modem_data *data)
{
	int ret = 0;
	struct modem_config *config = (struct modem_config *)data->dev->config;

	/* If configured, driver power GPIO low */
	if (config->power_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->power_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Failed to set power pin, error %d", ret);
		}
	}
	/* If configured, driver reset GPIO low */
	if (config->reset_gpio.port != NULL) {
		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			LOG_ERR("Failed to set reset pin, error %d", ret);
		}
	}

	modem_chat_release(&data->chat);
	modem_pipe_close_async(data->uart_pipe);

	return ret;
}

static void modem_idle_handler(struct modem_data *data, enum modem_event evt)
{
	struct modem_config *config = (struct modem_config *)data->dev->config;

	LOG_DBG("%s evt %d", __func__, evt);

	switch (evt) {
	case MODEM_EVENT_RESUME:
		/* If configured, driver power GPIO high */
		if (config->power_gpio.port != NULL) {
			gpio_pin_set_dt(&config->power_gpio, 1);
		}
		/* If configured, driver reset GPIO high */
		if (config->reset_gpio.port != NULL) {
			gpio_pin_set_dt(&config->reset_gpio, 1);
		}

		modem_enter_state(data, MODEM_STATE_INIT);
		break;
	default:
		LOG_DBG("%s got %d, not handled", __func__, evt);
		break;
	}
}

static int modem_on_state_enter(struct modem_data *data)
{
	int ret;

	LOG_DBG("%s state %d", __func__, data->state);

	switch (data->state) {
	case MODEM_STATE_IDLE:
		ret = modem_idle_state_enter(data);
		break;
	case MODEM_STATE_INIT:
		ret = modem_init_state_enter(data);
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

static int modem_on_state_leave(struct modem_data *data)
{
	int ret;

	LOG_DBG("%s state %d", __func__, data->state);

	switch (data->state) {
	default:
		ret = 0;
		break;
	}

	return ret;
}

static void modem_enter_state(struct modem_data *data, enum modem_state state)
{
	int ret;

	LOG_DBG("%s state %d", __func__, data->state);

	k_sem_take(&data->sem_state, K_FOREVER);

	ret = modem_on_state_leave(data);

	if (ret < 0) {
		LOG_WRN("failed to leave state, error: %i", ret);

		return;
	}

	data->state = state;
	ret = modem_on_state_enter(data);

	if (ret < 0) {
		LOG_WRN("failed to enter state error: %i", ret);
	}

	k_sem_give(&data->sem_state);
}

static void modem_event_handler(struct modem_data *data, enum modem_event evt)
{
	enum modem_state state;

	state = data->state;

	LOG_DBG("%s state %d evt %d", __func__, state, evt);

	switch (data->state) {
	case MODEM_STATE_IDLE:
		modem_idle_handler(data, evt);
		break;
	case MODEM_STATE_INIT:
		modem_init_handler(data, evt);
		break;
	case MODEM_STATE_READY:
		modem_ready_handler(data, evt);
		break;
	}

	if (state != data->state) {
		LOG_DBG("%s: %d => %d", __func__, state, data->state);
	}
}

/* Worker that pops events from the ringbuf and calls the main event handler */
static void modem_event_dispatch_handler(struct k_work *item)
{
	struct modem_data *data = CONTAINER_OF(item, struct modem_data, event_dispatch_work);

	uint8_t events[sizeof(data->event_buf)];
	uint8_t events_cnt;

	k_mutex_lock(&data->event_rb_lock, K_FOREVER);

	events_cnt = (uint8_t)ring_buf_get(&data->event_rb, events, sizeof(data->event_buf));

	k_mutex_unlock(&data->event_rb_lock);

	for (uint8_t i = 0; i < events_cnt; i++) {
		modem_event_handler(data, (enum modem_event)events[i]);
	}
}

/* Add an event to the event's ringbuf */
static void modem_add_event(struct modem_data *data, enum modem_event evt)
{
	k_mutex_lock(&data->event_rb_lock, K_FOREVER);
	ring_buf_put(&data->event_rb, (uint8_t *)&evt, 1);
	k_mutex_unlock(&data->event_rb_lock);
	k_work_submit_to_queue(&_modem_workq, &data->event_dispatch_work);
}

static void modem_request_handler(struct modem_data *data, enum modem_request req)
{
	int ret = 0;
	const struct modem_config *config = data->dev->config;
	enum modem_state state;

	state = data->state;

	LOG_DBG("%s state %d req %d", __func__, state, req);

	switch (req) {
	case MODEM_REQ_RESET:
		ret = modem_chat_run_script_async(&data->chat, config->reset_chat_script);
		break;
	case MODEM_REQ_IFACE_ENABLE:
		ret = do_iface_enable(data);
		break;
	case MODEM_REQ_IFACE_DISABLE:
		ret = do_iface_disable(data);
		break;
	case MODEM_REQ_GNSS_RESUME:
		ret = offload_gnss(data, true);
		break;
	case MODEM_REQ_GNSS_SUSPEND:
		ret = offload_gnss(data, false);
		break;
	case MODEM_REQ_OPEN_SOCK:
		ret = do_socket_open(data);
		break;
	case MODEM_REQ_CLOSE_SOCK:
		ret = do_socket_close(data);
		break;
	case MODEM_REQ_CONNECT_SOCK:
		ret = do_socket_connect(data);
		break;
	case MODEM_REQ_DATA_MODE:
		ret = do_data_mode(data);
		break;
	case MODEM_REQ_SEND_DATA:
		ret = do_socket_send(data);
		break;
	case MODEM_REQ_RECV_DATA:
		ret = do_socket_recv(data);
		break;
	case MODEM_REQ_SELECT_SOCK:
		ret = do_select_socket(data);
		break;
	case MODEM_REQ_GET_ACTIVE_SOCK:
		ret = do_get_active_socket(data);
		break;
	case MODEM_REQ_GET_ADDRINFO:
		ret = do_get_addrinfo(data);
		break;
	default:
		LOG_DBG("%s got %d, not handled", __func__, req);
		break;
	}

	LOG_DBG("Request %d executed with %d", req, ret);
}

/* Worker that pops requests from the ringbuf and calls the request handler */
static void modem_request_dispatch_handler(struct k_work *item)
{
	struct k_work_delayable *item_delayable = k_work_delayable_from_work(item);
	struct modem_data *data =
		CONTAINER_OF(item_delayable, struct modem_data, request_dispatch_work);

	bool need_sched = false;
	uint8_t request;
	uint8_t cnt;

	/* Check if modem is ready to handle request */
	if (modem_state_get(data) < MODEM_STATE_READY) {
		LOG_WRN("Can't execute requests before modem is ready");
		k_work_schedule_for_queue(&_modem_workq, item_delayable,
					  K_MSEC(MDM_REQUEST_WAIT_READY_MSEC));
		return;
	}

	/*
	 * Do not wait for too long,
	 * better to schedule the work again if chat is busy
	 */
	int ret = k_sem_take(&data->sem_script_exec, K_MSEC(10));
	if (ret < 0) {
		LOG_DBG("A script is running, trying again in %dms..",
			MDM_REQUEST_SCHED_DELAY_MSEC);
		/* Busy running another script, need to wait adn try again in a bit */
		k_work_schedule_for_queue(&_modem_workq, item_delayable,
					  K_MSEC(MDM_REQUEST_SCHED_DELAY_MSEC));
		return;
	}

	k_mutex_lock(&data->request_rb_lock, K_FOREVER);

	/* Get one request */
	cnt = (uint8_t)ring_buf_get(&data->request_rb, &request, sizeof(request));

	/*
	 * Check if we have more requests left in the ringbuf
	 * and if we do, set a flag to schedule the work again
	 */
	if (!ring_buf_is_empty(&data->request_rb)) {
		need_sched = true;
	}

	k_mutex_unlock(&data->request_rb_lock);

	if (cnt) {
		modem_request_handler(data, (enum modem_request)request);
	}

	/*
	 * Schedule the work if needed
	 */
	if (need_sched) {
		k_work_schedule_for_queue(&_modem_workq, item_delayable,
					  K_MSEC(MDM_REQUEST_SCHED_DELAY_MSEC));
	}
}

/* Add a request to the request's ringbuf */
static void modem_add_request(struct modem_data *data, enum modem_request req)
{
	LOG_DBG("%s request %d", __func__, req);
	k_mutex_lock(&data->request_rb_lock, K_FOREVER);
	if (ring_buf_put(&data->request_rb, (uint8_t *)&req, 1) < 1) {
		LOG_ERR("Failed to add request, ringbuf is full");
	}
	k_mutex_unlock(&data->request_rb_lock);
	k_work_schedule_for_queue(&_modem_workq, &data->request_dispatch_work, K_MSEC(10));
}

/* Modem pipe events handler */
static void modem_bus_pipe_handler(struct modem_pipe *pipe, enum modem_pipe_event event,
				   void *user_data)
{
	LOG_DBG("%s evt:%d", __func__, event);

	struct modem_data *data = (struct modem_data *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		modem_add_event(data, MODEM_EVENT_BUS_OPENED);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		modem_add_event(data, MODEM_EVENT_BUS_CLOSED);
		break;

	default:
		break;
	}
}

/* ~~~~ Modem chat handlers ~~~~ */

/* Modem chat events handler */
static void modem_chat_handler(struct modem_chat *chat, enum modem_chat_script_result result,
			       void *user_data)
{
	struct modem_data *data = (struct modem_data *)user_data;

	if (result == MODEM_CHAT_SCRIPT_RESULT_SUCCESS) {
		modem_add_event(data, MODEM_EVENT_SCRIPT_SUCCESS);
	} else {
		modem_add_event(data, MODEM_EVENT_SCRIPT_FAILED);
	}
}

static void modem_chat_on_imei(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct modem_data *data = (struct modem_data *)user_data;

	ARG_UNUSED(chat);

	if (argc != 2) {
		LOG_ERR("Too few arguments");
		return;
	}

	if (strlen(argv[1]) != MDM_IMEI_LENGTH) {
		LOG_ERR("IMEI length %d doesn't match %d", strlen(argv[1]), MDM_IMEI_LENGTH);
		return;
	}

	for (size_t i = 0; i < strlen(argv[1]); i++) {
		data->imei[i] = argv[1][i];
	}

	LOG_INF("IMEI: %s", data->imei);
}

static void modem_chat_on_manufacturer(struct modem_chat *chat, char **argv, uint16_t argc,
				       void *user_data)
{
	struct modem_data *data = (struct modem_data *)user_data;

	ARG_UNUSED(chat);

	if (argc != 2) {
		LOG_ERR("Too few arguments");
		return;
	}

	if (strlen(argv[1]) > MDM_MANUFACTURER_LENGTH) {
		LOG_ERR("Manufacturer str length %d too long, max %d", strlen(argv[1]),
			MDM_MANUFACTURER_LENGTH);
		return;
	}

	for (size_t i = 0; i < strlen(argv[1]); i++) {
		data->manufacturer[i] = argv[1][i];
	}

	LOG_INF("Manufacturer: %s", data->manufacturer);
}

static void modem_chat_on_model(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	struct modem_data *data = (struct modem_data *)user_data;

	ARG_UNUSED(chat);

	if (argc != 2) {
		LOG_ERR("Too few arguments");
		return;
	}

	if (strlen(argv[1]) > MDM_MODEL_LENGTH) {
		LOG_ERR("Model str length %d too long, max %d", strlen(argv[1]), MDM_MODEL_LENGTH);
		return;
	}

	for (size_t i = 0; i < strlen(argv[1]); i++) {
		data->model[i] = argv[1][i];
	}

	LOG_INF("Model: %s", data->model);
}

static void modem_chat_on_revision(struct modem_chat *chat, char **argv, uint16_t argc,
				   void *user_data)
{
	struct modem_data *data = (struct modem_data *)user_data;

	ARG_UNUSED(chat);

	if (argc != 2) {
		LOG_ERR("Too few arguments");
		return;
	}

	if (strlen(argv[1]) > MDM_REVISION_LENGTH) {
		LOG_ERR("Revision str length %d too long, max %d", strlen(argv[1]),
			MDM_REVISION_LENGTH);
		return;
	}

	for (size_t i = 0; i < strlen(argv[1]); i++) {
		data->revision[i] = argv[1][i];
	}

	LOG_INF("Revision: %s", data->revision);
}

/*
 * Handler: +CEREG: <n>[1],<reg_status>[2] (read command)
 * Handler: +CEREG: <reg_status>[1] (notification)
 */
static void modem_chat_on_cereg(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	int status = 0;
	struct modem_data *data = chat->user_data;

	if (argc == 2) {
		/* Unsolicited notification */
		status = ATOI(argv[1], -1, "reg_status");
	} else if (argc == 3) {
		/* Read command */
		status = ATOI(argv[2], -1, "reg_status");
	} else {
		LOG_WRN("%s got %d args", __func__, argc);
	}

	if (status == 1 || status == 5) {
		modem_connected_set(data, true);
	} else {
		modem_connected_set(data, false);
	}
}

/* Handler: #XGETADDRINFO: <hostname>[1] */
void modem_chat_on_xgetaddrinfo(struct modem_chat *chat, char **argv, uint16_t argc,
				void *user_data)
{
	int ret = 0;
	char ips[256];

	/* Check args count is correct */
	if (argc != 2) {
		LOG_ERR("%s received %d args", __func__, argc);
		return;
	}

	/* Copy string containing IP address removing the
	 * leading and trailing " characters
	 */
	memcpy(ips, &argv[1][1], strlen(argv[1]) - 2);
	ips[strlen(ips)] = '\0';

	LOG_DBG("IP %s", ips);

	/* Set addr family type based on str len */
	if (strlen(ips) > INET_ADDRSTRLEN) {
		/* IPV6 */
		dns_result.ai_family = AF_INET6;
		dns_result_addr.sa_family = AF_INET6;
	} else {
		/* IPV4 */
		dns_result.ai_family = AF_INET;
		dns_result_addr.sa_family = AF_INET;
	}

	ret = net_addr_pton(dns_result.ai_family, ips,
			    &((struct sockaddr_in *)&dns_result_addr)->sin_addr);
	if (ret < 0) {
		LOG_ERR("Failed to convert string to ip addr %d", ret);
	}
}

/* Handler for XSOCKET: <handle>[1],<type>[2],<protocol>[3]
 * Handler for XSOCKET: <result>[1],<result_str>[2] */
void modem_chat_on_xsocket(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct modem_data *data = chat->user_data;

	if (argc == 4) {
		data->sock_fd = ATOI(argv[1], -1, "sock_fd");
		LOG_DBG("Got socket fd %d", data->sock_fd);
	} else if (argc == 3) {
		int res = ATOI(argv[1], -1, "result");
		if (!res) {
			LOG_DBG("Socket closed successfully");
		}
	}
}

/*
 * Handler: #XSOCKETSELECT:
 * <handle>[1],<family>[2],<role>[3],<type>[4],<sec_tag>[5],<ranking>[6],<cid>[7] Handler:
 * #XSOCKETSELECT: <handle_active>[1]
 */
void modem_chat_on_xsocketselect(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct modem_data *data = chat->user_data;

	if (argc == 2) {
		int handle = ATOI(argv[1], -1, "handle");
		if (handle >= 0) {
			data->sock_fd = handle;
		}
	} else if (argc == 8) {
		/* Nothing to do here really, just log */
		int handle = 0;

		handle = ATOI(argv[0], -1, "handle");
		if (handle >= 0) {
			LOG_DBG("Socket %d exists", handle);
		}
	} else {
		LOG_WRN("%s received %d args", __func__, argc);
	}
}

/* Handler: #XCONNECT: <status>[1] */
void modem_chat_on_xconnect(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	int ret = 0;
	int status = 0;
	struct modem_socket *sock = NULL;
	struct modem_data *data = chat->user_data;

	status = ATOI(argv[1], 0, "status");

	/* Retrieve socket */
	sock = modem_socket_from_fd(&data->socket_config, data->sock_fd);
	if (!sock) {
		LOG_ERR("Socket %d not found", data->sock_fd);
		ret = -EINVAL;
		return;
	}

	switch (status) {
	case 0:
		/* Disconnected */
		LOG_DBG("Disconnected");
		sock->is_connected = false;
		break;
	case 1:
		/* Connected */
		LOG_DBG("Connected");
		sock->is_connected = true;
		break;
	default:
		LOG_WRN("Received unknown status from XCONNECT %d", status);
	}
}

/*
 * Handler: #XDATAMODE: <status>[1]
 * This handler assumes CONFIG_SLM_DATAMODE_URC is enabled in
 * the SLM application running on the nRF9160.
 * This way it's possible to know how much data has been
 * effectively sent over the socket.
 */
void modem_chat_on_xdata(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	int ret = 0;
	struct modem_data *data = chat->user_data;

	ret = ATOI(argv[1], -1, "sent");
	if (ret < 0) {
		LOG_ERR("Data mode error %d", ret);
		/* Return the error */
		data->send_sock.sent = ret;
	} else if (ret == 0) {
		/* Received 0, data mode successful */
		LOG_DBG("Data mode success");
	} else {
		/* Received number of bytes sent */
		data->send_sock.sent = ret;
	}
}

/* Handler: #XRECV: <size>[1] */
void modem_chat_on_xrecv(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	int recv_len = 0;
	struct modem_data *data = chat->user_data;

	recv_len = ATOI(argv[1], -1, "size");
	LOG_INF("Received %d bytes", recv_len);
	if (recv_len >= 0) {
		/* Save number of bytes received */
		data->recv_sock.nbytes = recv_len;
	}
}

/*
 * Handler: Data received from socket.
 * After #XRECV (in a new line) and no specific pattern to find
 * Received data will be pushed to a ring buffer, this because
 * the Serial LTE Modem application doesn't allow to specify the
 * number of bytes to receive, but just returns all data available
 * in the socket.
 * The amount of data requested by the application will be popped
 * from the ring buffer instead.
 */
void modem_chat_on_xrecvdata(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	int ret = 0;
	int available = 0;
	uint8_t *ringbuf_ptr;
	uint32_t claimed_len = 0;
	uint32_t data_len = 0;
	size_t offset = 0;
	struct modem_data *data = chat->user_data;

	data_len = data->recv_sock.nbytes;

	LOG_DBG("%s got %d bytes", __func__, data_len);
	if (data_len) {
		LOG_HEXDUMP_DBG(argv[1], data_len, "Received bytes");
	} else if (strlen(argv[1])) {
		LOG_HEXDUMP_DBG(argv[1], strlen(argv[1]), "Received bytes");
	}

	if (data_len > 0) {
		/* Reset counter of received bytes */
		data->recv_sock.nbytes = 0;

		/* Retrieve available space in rx_ringbuf */
		available = (int)ring_buf_space_get(&rx_ringbuf);
		if (available < data_len) {
			LOG_ERR("Not enough space available in ring buf (%d < %d)", available,
				data_len);
			ret = -ENOMEM;
			return;
		}

		/*
		 * It's possible that we can't claim all bytes at once
		 * if we are close to the end of the ringbuf
		 */
		while (data_len > 0) {
			/* Claim tbc bytes in rx_ringbuf */
			claimed_len = (int)ring_buf_put_claim(&rx_ringbuf, &ringbuf_ptr, data_len);
			if (claimed_len != data_len) {
				LOG_DBG("Couldn't claim enough bytes, %d instead of %d",
					claimed_len, data_len);
			}

			/* Update len to the number of bytes that we still need to claim */
			data_len -= claimed_len;

			/* Copy received data to rx_ringbuf */
			memcpy(ringbuf_ptr, &argv[1][offset], claimed_len);

			/* Update offset in case we couldn't claim bytes all at once */
			offset += claimed_len;

			/*
			 * Finalize copying bytes to rx_ringbuf
			 * claimed_len at this point contains the number of bytes we actually copied
			 */
			ret = ring_buf_put_finish(&rx_ringbuf, claimed_len);
			if (ret) {
				LOG_ERR("Failed to copy all data to ringbuf");
				ret = -ENOMEM;
				return;
			}
		}
	}
}

/*
 * Handler: #XGPS: <service>[1], <status>[2]
 * Handler: #XGPS:
 * <latitude>[1],<longitude>[2],<altitude>[3],<accuracy>[4],<speed>[5],<heading>[6],<datetime>[7]
 */
void modem_chat_on_xgps(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct modem_data *data = chat->user_data;

	if (argc == 3) {
		int service = ATOI(argv[1], -1, "service");
		int status = ATOI(argv[2], -1, "status");

		LOG_DBG("%s service:%d status:%d", __func__, service, status);
	} else if (argc >= 7) {
		/* PVT data */
		LOG_DBG("Received PVT data:");
		uint32_t latitude = 0;
		if (str_float_to_uint32(argv[1], &latitude) < 0) {
			LOG_ERR("Failed to convert latitude");
		}
		uint32_t longitude = 0;
		if (str_float_to_uint32(argv[2], &longitude) < 0) {
			LOG_ERR("Failed to convert longitude");
		}
		uint32_t altitude = 0;
		if (str_float_to_uint32(argv[3], &altitude) < 0) {
			LOG_ERR("Failed to convert altitude");
		}
		uint32_t accuracy = 0;
		if (str_float_to_uint32(argv[4], &accuracy) < 0) {
			LOG_ERR("Failed to convert accuracy");
		}
		uint32_t speed = 0;
		if (str_float_to_uint32(argv[5], &speed) < 0) {
			LOG_ERR("Failed to convert speed");
		}
		uint32_t heading = 0;
		if (str_float_to_uint32(argv[6], &heading) < 0) {
			LOG_ERR("Failed to convert heading");
		}
		LOG_DBG("latitude:%d longitude:%d altitude:%d", latitude, longitude, altitude);
		LOG_DBG("accuracy:%d speed:%d heading:%d", accuracy, speed, heading);
		LOG_DBG("datetime:%s", argv[7]);

		struct gnss_data fix_data = {.nav_data =
						     {
							     .latitude = latitude,
							     .longitude = longitude,
							     .altitude = (int32_t)altitude,
							     .speed = speed,
							     .bearing = heading,
						     },
					     .info = {
						     .fix_quality = GNSS_FIX_QUALITY_GNSS_SPS,
						     .fix_status = GNSS_FIX_STATUS_GNSS_FIX,
					     }};

		if (parse_date_time_str(argv[7], &fix_data.utc) < 0) {
			LOG_ERR("Failed to parse date time string");
		}

		/* Publish fix data */
		gnss_publish_data(data->gnss_dev, &fix_data);
	} else {
		LOG_WRN("%s received %d args", __func__, argc);
	}
}

/* Handler: NMEA string coming from SLM, used for debug */
void modem_chat_on_nmea(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	LOG_DBG("%s", argv[1]);
}

/* Handler: PVT messages coming from SLM, used for debug */
void modem_chat_on_pvt(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	LOG_DBG("%s", argv[1]);
}

/* ~~~~ Modem chat matches ~~~~ */

MODEM_CHAT_MATCH_DEFINE(ok_match, "OK", "", NULL);
MODEM_CHAT_MATCH_DEFINE(ready_match, "Ready", "", NULL);
MODEM_CHAT_MATCHES_DEFINE(abort_matches, MODEM_CHAT_MATCH("ERROR", "", NULL));
MODEM_CHAT_MATCHES_DEFINE(unsol_matches, MODEM_CHAT_MATCH("+CEREG: ", ",", modem_chat_on_cereg),
			  MODEM_CHAT_MATCH("#XGPS: ", ",", modem_chat_on_xgps),
			  MODEM_CHAT_MATCH("#XDATAMODE: ", "", modem_chat_on_xdata),
			  MODEM_CHAT_MATCH("$", "", modem_chat_on_nmea),
			  MODEM_CHAT_MATCH("PVT: ", "", modem_chat_on_pvt));

/*
 * The response to the CGSN command is:
 * "": IMEI string
 * "OK": end of response to CGSN command
 */
MODEM_CHAT_MATCHES_DEFINE(imei_match,
			  MODEM_CHAT_MATCH_INITIALIZER("", "", modem_chat_on_imei, false, true),
			  MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false));
/*
 * The response to the CGMI command is:
 * "": manufacturer string
 * "OK": end of response to CGMI command
 */
MODEM_CHAT_MATCHES_DEFINE(manufacturer_match,
			  MODEM_CHAT_MATCH_INITIALIZER("", "", modem_chat_on_manufacturer, false,
						       true),
			  MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false));
/*
 * The response to the CGMM command is:
 * "": model string
 * "OK": end of response to CGMM command
 */
MODEM_CHAT_MATCHES_DEFINE(model_match,
			  MODEM_CHAT_MATCH_INITIALIZER("", "", modem_chat_on_model, false, true),
			  MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false));
/*
 * The response to the CGMR command is:
 * "": revision string
 * "OK": end of response to CGMR command
 */
MODEM_CHAT_MATCHES_DEFINE(revision_match,
			  MODEM_CHAT_MATCH_INITIALIZER("", "", modem_chat_on_revision, false, true),
			  MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false));
/*
 * The response to the XRECV command can be:
 * 1. "ERROR": no data was received
 * 2. Series of responses if some data is received
 *      2.1 "#XRECV": indicating the amount of data received
 *      2.2 "": the actual data
 *      2.3 "OK": end of response to XRECV command
 */
MODEM_CHAT_MATCHES_DEFINE(recv_match, MODEM_CHAT_MATCH_INITIALIZER("ERROR", "", NULL, false, false),
			  MODEM_CHAT_MATCH_INITIALIZER("#XRECV: ", "", modem_chat_on_xrecv, false,
						       true),
			  MODEM_CHAT_MATCH_INITIALIZER("", "", modem_chat_on_xrecvdata, false,
						       true),
			  MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false));
/*
 * The response to the XGPS command is:
 * "OK": to signal correct execution of the command
 * "XGPS": to indicate status and service of GNSS
 */
MODEM_CHAT_MATCHES_DEFINE(xgps_match, MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, true),
			  MODEM_CHAT_MATCH_INITIALIZER("#XGPS: ", ",", modem_chat_on_xgps, false,
						       false));
/*
 * The response to the XCONNECT command is:
 * "XCONNECT": reporting the connection status
 * "OK": to signal correct execution of the command
 */
MODEM_CHAT_MATCHES_DEFINE(xconnect_match,
			  MODEM_CHAT_MATCH_INITIALIZER("#XCONNECT: ", "", modem_chat_on_xconnect,
						       false, true),
			  MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false));
/*
 * The response to the XSOCKET command is:
 * "XSOCKET": reporting the details of the socket that was opened/closed
 * "OK": to signal correct execution of the command
 */
MODEM_CHAT_MATCHES_DEFINE(xsocket_match,
			  MODEM_CHAT_MATCH_INITIALIZER("#XSOCKET: ", "", modem_chat_on_xsocket,
						       false, true),
			  MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false));
/*
 * The response to the XSOCKETSELECT command is:
 * "XSOCKETSELECT": reporting the handle of the selected socket
 * "OK": to signal correct execution of the command
 */
MODEM_CHAT_MATCHES_DEFINE(xsocketselect_match,
			  MODEM_CHAT_MATCH_INITIALIZER("#XSOCKETSELECT: ", ",",
						       modem_chat_on_xsocketselect, false, true),
			  MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false));
/*
 * The response to the XGETADDRINFO command is:
 * "XGETADDRINFO": reporting the resolved IP address
 * "OK": to signal correct execution of the command
 */
MODEM_CHAT_MATCHES_DEFINE(xgetaddrinfo_match,
			  MODEM_CHAT_MATCH_INITIALIZER("#XGETADDRINFO: ", "",
						       modem_chat_on_xgetaddrinfo, false, true),
			  MODEM_CHAT_MATCH_INITIALIZER("OK", "", NULL, false, false));

/* ~~~~ GNSS offload APIs ~~~~ */

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf9160_gnss)
static int offload_gnss(struct modem_data *data, bool enable)
{
	int ret = 0;
	bool cloud_assistance = false; /* Do not use cloud assistance */

	if (enable) {
		/* Set dynamic script name */
		data->dynamic_script.name = "gnss_enable";
		/* Start GNSS */
		if (data->gnss_interval == 1) {
			/* Continous mode, omit timeout param */
			ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
				       "AT#XGPS=%d,%d,%d", enable, cloud_assistance,
				       data->gnss_interval);
		} else {
			/* One-shot or periodic */
			ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
				       "AT#XGPS=%d,%d,%d,%d", enable, cloud_assistance,
				       data->gnss_interval, data->gnss_timeout);
		}
	} else {
		/* Set dynamic script name */
		data->dynamic_script.name = "gnss_disable";
		/* Stop GNSS */
		ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
			       "AT#XGPS=%d", enable);
	}
	/* Set request size */
	data->dynamic_script_chat.request_size = ret;

	/* Set dynamic match
	 * Create statically defined one as multiple responses are expected */
	data->dynamic_script_chat.response_matches = xgps_match;
	data->dynamic_script_chat.response_matches_size = ARRAY_SIZE(xgps_match);

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to send GNSS command, error %d", ret);
	}

	return ret;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf9160_gnss) */

/* ~~~~ PM APIs ~~~~ */

#ifdef CONFIG_PM_DEVICE
static int modem_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;
	struct modem_data *data = (struct modem_data *)dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("Modem PM resume");
		modem_add_event(data, MODEM_EVENT_RESUME);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		LOG_DBG("Modem PM suspend");
		modem_add_event(data, MODEM_EVENT_SUSPEND);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf9160_gnss)
static int gnss_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;
	struct modem_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("GNSS PM resume");
		modem_add_request(data, MODEM_REQ_GNSS_RESUME);
		/* Wait for script execution to be done */
		ret = wait_script_done(__func__, data, MDM_SCRIPT_DONE_TIMEOUT_SEC, 1);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		LOG_DBG("GNSS PM suspend");
		modem_add_request(data, MODEM_REQ_GNSS_SUSPEND);
		/* Wait for script execution to be done */
		ret = wait_script_done(__func__, data, MDM_SCRIPT_DONE_TIMEOUT_SEC, 1);
		break;

	default:
		LOG_WRN("Received unhandled action %d", action);
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf9160_gnss) */
#endif /* CONFIG_PM_DEVICE */

/* ~~~~ Net IF chat scripts ~~~~ */

static int do_iface_enable(struct modem_data *data)
{
	int ret = 0;

	/* Set dynamic script name */
	data->dynamic_script.name = "iface_enable";
	/* Create dynamic request */
	ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf), "AT+CFUN=1");
	data->dynamic_script_chat.request_size = ret;
	/* Make sure the dynamic script uses the static match buffer */
	data->dynamic_script_chat.response_matches = &data->dynamic_match;
	data->dynamic_script_chat.response_matches_size = 1;
	/* Create dynamic match */
	ret = snprintk(data->dynamic_match_buf, sizeof(data->dynamic_match_buf), "OK");
	data->dynamic_match.match_size = ret;
	data->dynamic_match.callback = NULL;

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run iface enable script, error %d", ret);
	}

	return ret;
}

static int do_iface_disable(struct modem_data *data)
{
	int ret = 0;

	/* Set dynamic script name */
	data->dynamic_script.name = "iface_disable";
	/* Create dynamic request */
	ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf), "AT+CFUN=0");
	data->dynamic_script_chat.request_size = ret;
	/* Make sure the dynamic script uses the static match buffer */
	data->dynamic_script_chat.response_matches = &data->dynamic_match;
	data->dynamic_script_chat.response_matches_size = 1;
	/* Create dynamic match */
	ret = snprintk(data->dynamic_match_buf, sizeof(data->dynamic_match_buf), "OK");
	data->dynamic_match.match_size = ret;
	data->dynamic_match.callback = NULL;

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run iface disable script, error %d", ret);
	}

	return ret;
}

/* ~~~~ DNS related chat scripts ~~~~ */

static int do_get_addrinfo(struct modem_data *data)
{
	int ret = 0;

	/* Set dynamic script name */
	data->dynamic_script.name = "get_addrinfo";
	/* Create dynamic request */
	ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
		       "AT#XGETADDRINFO=\"%s\"", data->get_addrinfo.node);
	data->dynamic_script_chat.request_size = ret;
	/* Set dynamic match
	 * Create statically defined one as multiple responses are expected */
	data->dynamic_script_chat.response_matches = xgetaddrinfo_match;
	data->dynamic_script_chat.response_matches_size = ARRAY_SIZE(xgetaddrinfo_match);

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run get_addrinfo script, error %d", ret);
	}

	return ret;
}

/* ~~~~ Sockets related chat scripts ~~~~ */

static int do_socket_open(struct modem_data *data)
{
	int ret = 0;
	int role = 0; /* Default: Client */

	/* Set dynamic script name */
	data->dynamic_script.name = "sock_open";
	/* Create dynamic request */
	ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
		       "AT#XSOCKET=%d,%d,%d", data->open_sock.family, data->open_sock.type, role);
	data->dynamic_script_chat.request_size = ret;
	/* Set dynamic match
	 * Create statically defined one as multiple responses are expected */
	data->dynamic_script_chat.response_matches = xsocket_match;
	data->dynamic_script_chat.response_matches_size = ARRAY_SIZE(xsocket_match);

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run socket open script, error %d", ret);
		errno = -ret;
	}

	return ret;
}

static int do_socket_close(struct modem_data *data)
{
	int ret = 0;

	/* Set dynamic script name */
	data->dynamic_script.name = "sock_close";
	/* Create dynamic request */
	ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
		       "AT#XSOCKET=0");
	data->dynamic_script_chat.request_size = ret;
	/* Set dynamic match
	 * Create statically defined one as multiple responses are expected */
	data->dynamic_script_chat.response_matches = xsocket_match;
	data->dynamic_script_chat.response_matches_size = ARRAY_SIZE(xsocket_match);

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run socket close script, error %d", ret);
	}

	return ret;
}

static int do_socket_connect(struct modem_data *data)
{
	int ret = 0;

	/* Set dynamic script name */
	data->dynamic_script.name = "sock_connect";
	/* Create dynamic request */
	ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
		       "AT#XCONNECT=\"%s\",%d", data->connect_sock.ip_str,
		       data->connect_sock.dst_port);
	data->dynamic_script_chat.request_size = ret;
	/* Create dynamic match
	 * Use statically defined one as multiple responses are expected */
	data->dynamic_script_chat.response_matches = xconnect_match;
	data->dynamic_script_chat.response_matches_size = ARRAY_SIZE(xconnect_match);

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run socket connect script, error %d", ret);
	}

	return ret;
}

static int do_data_mode(struct modem_data *data)
{
	int ret;
	uint16_t dst_port = 0U;

	if (!data->send_sock.sock) {
		return -EINVAL;
	}

	if (!data->send_sock.dst_addr && data->send_sock.sock->ip_proto == IPPROTO_UDP) {
		data->send_sock.dst_addr = &data->send_sock.sock->dst;
	}

	/*
	 * Data mode allows sending MDM_MAX_DATA_LENGTH bytes to
	 * the socket in one command
	 */
	if (data->send_sock.len > MDM_MAX_DATA_LENGTH) {
		data->send_sock.len = MDM_MAX_DATA_LENGTH;
	}

	/* Set dynamic script name */
	data->dynamic_script.name = "data_mode_enter";

	if (data->send_sock.sock->ip_proto == IPPROTO_UDP) {
		char ip_str[NET_IPV6_ADDR_LEN];

		ret = sprint_ip_addr(data->send_sock.dst_addr, ip_str, sizeof(ip_str));
		if (ret != 0) {
			LOG_ERR("Error formatting IP string %d", ret);
			goto error;
		}

		ret = get_addr_port(data->send_sock.dst_addr, &dst_port);
		if (ret != 0) {
			LOG_ERR("Error getting port from IP address %d", ret);
			goto error;
		}

		/* Create dynamic request for UDP send */
		ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
			       "AT#XSENDTO=\"%s\",%d", ip_str, dst_port);
	} else {
		/* Create dynamic request for TCP send */
		ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
			       "AT#XSEND");
	}
	/* Set dynamic request size */
	data->dynamic_script_chat.request_size = ret;

	/* Make sure the dynamic script uses the static match buffer */
	data->dynamic_script_chat.response_matches = &data->dynamic_match;
	data->dynamic_script_chat.response_matches_size = 1;
	/* Create dynamic match */
	ret = snprintk(data->dynamic_match_buf, sizeof(data->dynamic_match_buf), "OK");
	data->dynamic_match.match_size = ret;
	data->dynamic_match.callback = NULL;

	/* Send command that will trigger entering SLM Data Mode */
	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run data mode script, error %d", ret);
	}

error:
	return ret;
}

static int do_socket_send(struct modem_data *data)
{
	int ret = 0;

	/*
	 * Write all data to the UART pipe
	 */
	LOG_HEXDUMP_DBG(data->send_sock.buf, data->send_sock.len, "DATA");
	modem_pipe_transmit(data->uart_pipe, data->send_sock.buf, data->send_sock.len);

	/* Send MDM_DATA_MODE_TERMINATOR to exit SLM Data Mode */
	/* Set dynamic script name */
	data->dynamic_script.name = "data_mode_exit";
	/* Create dynamic request, do not use snprintk as terminator contains special characters */
	strcpy(data->dynamic_request_buf, MDM_DATA_MODE_TERMINATOR);
	data->dynamic_script_chat.request_size = strlen(MDM_DATA_MODE_TERMINATOR);
	/* Make sure the dynamic script uses the static match buffer */
	data->dynamic_script_chat.response_matches = &data->dynamic_match;
	data->dynamic_script_chat.response_matches_size = 1;
	/* Create dynamic match */
	ret = snprintk(data->dynamic_match_buf, sizeof(data->dynamic_match_buf), "#XDATAMODE: ");
	data->dynamic_match.match_size = ret;
	data->dynamic_match.callback = modem_chat_on_xdata;

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run socket send script, error %d", ret);
	}

	return ret;
}

static int do_socket_recv(struct modem_data *data)
{
	int ret = 0;

	/* Set dynamic script name */
	data->dynamic_script.name = "sock_recv";

	if ((data->recv_sock.flags & ZSOCK_MSG_DONTWAIT) ||
	    (data->recv_sock.flags & ZSOCK_MSG_WAITALL) ||
	    (data->recv_sock.flags & ZSOCK_MSG_PEEK)) {
		/* Create message to start receiving data, using provided flags */
		ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
			       "AT#XRECV=%d,%d", MDM_RECV_DATA_TIMEOUT_SEC, data->recv_sock.flags);
	} else {
		/* Create message to start receiving data */
		ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
			       "AT#XRECV=%d", MDM_RECV_DATA_TIMEOUT_SEC);
	}
	/* Set dynamic request size */
	data->dynamic_script_chat.request_size = ret;

	/* Create dynamic match
	 * Use statically defined one as multiple responses are expected */
	data->dynamic_script_chat.response_matches = recv_match;
	data->dynamic_script_chat.response_matches_size = ARRAY_SIZE(recv_match);

	/* Set shorter timeout just for this case */
	data->dynamic_script.timeout = MDM_RECV_DATA_SCRIPT_TIMEOUT_SEC;

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run socket recv script, error %d", ret);
	}

	return ret;
}

static int do_get_active_socket(struct modem_data *data)
{
	int ret = 0;

	/* Set dynamic script name */
	data->dynamic_script.name = "sock_get_active";
	/* Create dynamic request */
	ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
		       "AT#XSOCKETSELECT?");
	data->dynamic_script_chat.request_size = ret;
	/* Set dynamic match
	 * Create statically defined one as multiple responses are expected */
	data->dynamic_script_chat.response_matches = xsocketselect_match;
	data->dynamic_script_chat.response_matches_size = ARRAY_SIZE(xsocketselect_match);

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run get_active_socket script, error %d", ret);
	}

	return ret;
}

static int do_select_socket(struct modem_data *data)
{
	int ret = 0;

	if ((data->select_sock.sock_fd < 0) || (data->select_sock.sock_fd >= MDM_MAX_SOCKETS)) {
		LOG_ERR("Socket id %d out of range", data->select_sock.sock_fd);
		ret = -EINVAL;
		goto error;
	}

	/* Check if the socket is already the active one */
	if (data->select_sock.sock_fd == data->sock_fd) {
		LOG_DBG("Socket %d is already active", data->select_sock.sock_fd);
		/* Give semaphores as no script will be executed */
		k_sem_give(&data->sem_script_done);
		k_sem_give(&data->sem_script_exec);
		goto error;
	}

	struct modem_socket *sock =
		modem_socket_from_fd(&data->socket_config, data->select_sock.sock_fd);

	/* Set dynamic script name */
	data->dynamic_script.name = "sock_select";
	/* Create dynamic request */
	ret = snprintk(data->dynamic_request_buf, sizeof(data->dynamic_request_buf),
		       "AT#XSOCKETSELECT=%d", sock->id);
	data->dynamic_script_chat.request_size = ret;
	/* Set dynamic match
	 * Create statically defined one as multiple responses are expected */
	data->dynamic_script_chat.response_matches = xsocketselect_match;
	data->dynamic_script_chat.response_matches_size = ARRAY_SIZE(xsocketselect_match);

	ret = modem_chat_run_script_async(&data->chat, &data->dynamic_script);
	if (ret < 0) {
		LOG_ERR("Failed to run select_socket script, error %d", ret);
	}

error:
	return ret;
}

/* ~~~~ Socket offload APIs ~~~~ */

/*
 * Func: offload_close
 * Desc: This function closes the connection with the remote client and
 * frees the socket.
 */
static int offload_close(void *obj)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct modem_data *data = sock->data;
	int ret = 0;

	/* Make sure socket is allocated and assigned an id */
	if (modem_socket_id_is_assigned(&data->socket_config, sock) == false) {
		goto error;
	}

	/* Make sure the given socket is the one selected by the modem */
	/* Set data to be used by FSM */
	data->select_sock.sock_fd = sock->sock_fd;

	modem_add_request(data, MODEM_REQ_SELECT_SOCK);

	/* No need to store any data for FSM */
	modem_add_request(data, MODEM_REQ_CLOSE_SOCK);

	/* Wait for scripts execution to be done */
	ret = wait_script_done(__func__, data, MDM_SCRIPT_DONE_TIMEOUT_SEC, 2);
	if (ret) {
		goto error;
	}

	/* Invalidate reference to selected socket */
	data->sock_fd = -1;

	/* Close socket */
	modem_socket_put(&data->socket_config, sock->sock_fd);

error:
	return ret;
}

/*
 * Func: offload_bind
 * Desc: This function binds the provided socket to the provided address.
 */
static int offload_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct modem_data *data = sock->data;

	if (addrlen <= 0) {
		LOG_ERR("Invalid socket len %d", addrlen);
		ret = -EINVAL;
		goto error;
	}

	/* Make sure we've created the socket */
	if (modem_socket_is_allocated(&data->socket_config, sock) == false) {
		LOG_ERR("Need to create a socket first!");
		ret = -ENODEV;
		goto error;
	}

	/* Save bind address information */
	memcpy(&sock->src, addr, addrlen);

error:
	return ret;
}

/*
 * Func: offload_connect
 * Desc: This function will connect with a provided address.
 */
static int offload_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct modem_data *data = sock->data;

	if (!modem_connected_get(data)) {
		LOG_WRN("Modem not registered to the network");
		ret = -ENOTCONN;
		goto error;
	}

	if (!addr) {
		ret = -EINVAL;
		errno = -ret;
		goto error;
	}

	/* Make sure socket has been allocated */
	if (modem_socket_is_allocated(&data->socket_config, sock) == false) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d", sock->id, sock->sock_fd);
		errno = EINVAL;
		ret = -1;
		goto error;
	}

	/* Make sure we've created the socket */
	if (modem_socket_id_is_assigned(&data->socket_config, sock) == false) {
		LOG_ERR("Need to create a socket first!");
		ret = -1;
		goto error;
	}

	memcpy(&sock->dst, addr, sizeof(*addr));
	if (addr->sa_family == AF_INET6) {
		data->connect_sock.dst_port = ntohs(net_sin6(addr)->sin6_port);
	} else if (addr->sa_family == AF_INET) {
		data->connect_sock.dst_port = ntohs(net_sin(addr)->sin_port);
	} else {
		errno = EAFNOSUPPORT;
		ret = -1;
		goto error;
	}

	/* Skip socket connect if UDP */
	if (sock->ip_proto == IPPROTO_UDP) {
		errno = 0;
		ret = 0;
		goto error;
	}

	ret = sprint_ip_addr(addr, data->connect_sock.ip_str, sizeof(data->connect_sock.ip_str));
	if (ret != 0) {
		errno = -ret;
		LOG_ERR("Error formatting IP string %d", ret);
		goto error;
	}

	/* Make sure the given socket is the one selected by the modem */
	/* Set data to be used by FSM */
	data->select_sock.sock_fd = sock->sock_fd;

	modem_add_request(data, MODEM_REQ_SELECT_SOCK);

	/* Data to be used by FSM has alredy been set */
	modem_add_request(data, MODEM_REQ_CONNECT_SOCK);

	/* Wait for scripts execution to be done */
	ret = wait_script_done(__func__, data, MDM_SCRIPT_DONE_TIMEOUT_SEC, 2);
	if (ret) {
		goto error;
	}

	if (!sock->is_connected) {
		LOG_ERR("Socket connection failed");
		ret = -ENOTCONN;
	}

error:
	return ret;
}

/*
 * Func: offload_sendto
 * Desc: This function will send data on the socket object.
 */
static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *to, socklen_t tolen)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct modem_data *data = sock->data;

	if (!modem_connected_get(data)) {
		LOG_WRN("Modem not registered to the network");
		ret = -ENOTCONN;
		goto error;
	}

	/* Ensure that valid parameters are passed. */
	if (!buf || len == 0) {
		LOG_ERR("Invalid buf or len");
		errno = EINVAL;
		ret = -1;
		goto error;
	}

	if (!sock->is_connected && sock->ip_proto != IPPROTO_UDP) {
		LOG_ERR("Socket is not connected");
		errno = ENOTCONN;
		ret = -1;
		goto error;
	}

	/* Make sure the given socket is the one selected by the modem */
	/* Set data to be used by FSM */
	data->select_sock.sock_fd = sock->sock_fd;

	modem_add_request(data, MODEM_REQ_SELECT_SOCK);

	/* Set data to be used by FSM */
	data->send_sock.sock = sock;
	data->send_sock.dst_addr = to;
	data->send_sock.buf = (uint8_t *)buf;
	data->send_sock.len = len;
	/* Reset number of bytes sent, it will be set in the XDATA mode response handler */
	data->send_sock.sent = 0;

	/* Run script to enter data mode */
	modem_add_request(data, MODEM_REQ_DATA_MODE);

	/* Send data in data mode */
	modem_add_request(data, MODEM_REQ_SEND_DATA);

	/* Wait for script execution to be done */
	ret = wait_script_done(__func__, data, MDM_SCRIPT_DONE_TIMEOUT_SEC, 3);
	if (ret) {
		goto error;
	}

	/* Return amount of bytes sent in case of success */
	ret = data->send_sock.sent;
	if (ret < 0) {
		LOG_ERR("Data mode reported error %d", ret);
		goto error;
	}

	LOG_INF("Written %d bytes", ret);

	/* Data was written successfully. */
	errno = 0;

error:
	return ret;
}

/*
 * Func: offload_recvfrom
 * Desc: This function will receive data on the socket object.
 */
static ssize_t offload_recvfrom(void *obj, void *buf, size_t len, int flags, struct sockaddr *from,
				socklen_t *fromlen)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct modem_data *data = sock->data;

	if (!modem_connected_get(data)) {
		LOG_WRN("Modem not registered to the network");
		ret = -ENOTCONN;
		goto error;
	}

	if (!buf || len == 0) {
		errno = EINVAL;
		ret = -1;
		goto error;
	}

	/* Make sure the given socket is the one selected by the modem */
	/* Set data to be used by FSM */
	data->select_sock.sock_fd = sock->sock_fd;

	modem_add_request(data, MODEM_REQ_SELECT_SOCK);

	/* Set data to be used by FSM */
	data->recv_sock.flags = flags;

	modem_add_request(data, MODEM_REQ_RECV_DATA);

	/* Wait for script execution to be done */
	ret = wait_script_done(__func__, data, MDM_SCRIPT_DONE_TIMEOUT_SEC, 2);
	if (ret) {
		goto error;
	}

	/*
	 * Check if ringbuf is empty, not data has been received
	 * If empty we actually do not have any data to return
	 */
	if (ring_buf_is_empty(&rx_ringbuf)) {
		LOG_DBG("No data received");
		ret = -EAGAIN;
		errno = -ret;
		goto error;
	}

	/* Get data from rx_ringbuf and copy it to caller's buf */
	ret = (int)ring_buf_get(&rx_ringbuf, buf, len);
	if (ret != len) {
		LOG_DBG("Received data smaller than buffer, %d < %d", ret, len);
	}

	errno = 0;

error:
	/* Restore timeout for dynamic scripts, it is updated in do_socket_recv() */
	data->dynamic_script.timeout = MDM_DYNAMIC_SCRIPT_TIMEOUT_SEC;

	return ret;
}

/*
 * Func: offload_read
 * Desc: This function reads data from the given socket object.
 */
static ssize_t offload_read(void *obj, void *buffer, size_t count)
{
	return offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

/*
 * Func: offload_write
 * Desc: This function writes data to the given socket object.
 */
static ssize_t offload_write(void *obj, const void *buffer, size_t count)
{
	return offload_sendto(obj, buffer, count, 0, NULL, 0);
}

/*
 * Func: offload_sendmsg
 * Desc: This function sends messages to the modem.
 */
static ssize_t offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	ssize_t sent = 0;
	int rc;

	LOG_DBG("msg_iovlen:%zd flags:%d", msg->msg_iovlen, flags);

	for (int i = 0; i < msg->msg_iovlen; i++) {
		const char *buf = msg->msg_iov[i].iov_base;
		size_t len = msg->msg_iov[i].iov_len;

		while (len > 0) {
			rc = offload_sendto(obj, buf, len, flags, msg->msg_name, msg->msg_namelen);
			if (rc < 0) {
				if (rc == -EAGAIN) {
					k_sleep(MDM_SENDMSG_SLEEP);
				} else {
					sent = rc;
					break;
				}
			} else {
				sent += rc;
				buf += rc;
				len -= rc;
			}
		}
	}

	return (ssize_t)sent;
}

/* Func: offload_ioctl
 * Desc: Function call to handle various misc requests.
 */
static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct modem_data *data = sock->data;

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return modem_socket_poll_prepare(&data->socket_config, obj, pfd, pev, pev_end);
	}
	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return modem_socket_poll_update(obj, pfd, pev);
	}

	default:
		errno = EINVAL;
		return -1;
	}
}

/*
 * Socket vtable.
 */
static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable =
		{
			.read = offload_read,
			.write = offload_write,
			.close = offload_close,
			.ioctl = offload_ioctl,
		},
	.shutdown = NULL,
	.bind = offload_bind,
	.connect = offload_connect,
	.listen = NULL,
	.accept = NULL,
	.sendto = offload_sendto,
	.recvfrom = offload_recvfrom,
	.getsockopt = NULL,
	.setsockopt = NULL,
	.sendmsg = offload_sendmsg,
	.getpeername = NULL,
	.getsockname = NULL,
};

/*
 * Reserve a socket FD and request a socket from the modem.
 * Socket handle received from modem is used as socket ID.
 */
static int offload_socket(struct modem_data *data, int family, int type, int proto)
{
	int ret = 0;
	int sock_fd;

	sock_fd = modem_socket_get(&data->socket_config, family, type, proto);
	if (sock_fd < 0) {
		ret = sock_fd;
		errno = -ret;
		goto error;
	}

	/* Set data to be used by FSM */
	data->open_sock.family = family;
	data->open_sock.type = type;

	modem_add_request(data, MODEM_REQ_OPEN_SOCK);

	/* Wait for script execution to be done */
	ret = wait_script_done(__func__, data, MDM_SCRIPT_DONE_TIMEOUT_SEC, 1);
	if (ret) {
		goto error;
	}

	/* Use received handle as socket ID if valid, on error give up modem socket */
	if (data->sock_fd >= 0) {
		struct modem_socket *sock = modem_socket_from_fd(&data->socket_config, sock_fd);
		ret = modem_socket_id_assign(&data->socket_config, sock, data->sock_fd);
		if (ret < 0) {
			LOG_ERR("Failed to assign socket ID %d", ret);
			modem_socket_put(&data->socket_config, sock->sock_fd);
			goto error;
		}
		/* Store context data into sock */
		sock->data = data;
	}
	/* Return socket fd */
	ret = data->sock_fd;
	errno = 0;

error:
	return ret;
}

/*
 * Function to check if offload is supported
 */
static bool offload_is_supported(int family, int type, int proto)
{
	if (family != AF_INET && family != AF_INET6) {
		LOG_DBG("Offload not supported, family %d", family);
		return false;
	}

	if (type != SOCK_STREAM && type != SOCK_DGRAM) {
		LOG_DBG("Offload not supported, type %d", type);
		return false;
	}

	if (proto != IPPROTO_TCP && proto != IPPROTO_UDP) {
		LOG_DBG("Offload not supported, proto %d", proto);
		return false;
	}

	return true;
}

/* ~~~~ DNS offload APIs  ~~~~*/

/*
 * Perform a dns lookup.
 */
static int offload_getaddrinfo(struct modem_data *data, const char *node, const char *service,
			       const struct zsock_addrinfo *hints, struct zsock_addrinfo **res)
{
	uint32_t port = 0;
	int ret = 0;

	if (!modem_connected_get(data)) {
		LOG_WRN("Modem not registered to the network");
		ret = -ENOTCONN;
		goto error;
	}

	/* init result */
	(void)memset(&dns_result, 0, sizeof(dns_result));
	(void)memset(&dns_result_addr, 0, sizeof(dns_result_addr));

	dns_result.ai_addr = &dns_result_addr;
	dns_result.ai_addrlen = sizeof(dns_result_addr);
	dns_result.ai_canonname = dns_result_canonname;
	dns_result_canonname[0] = '\0';

	if (service) {
		port = ATOI(service, -1, "port");
		if (port < 1 || port > USHRT_MAX) {
			LOG_ERR("Port number is out of range %d", port);
			ret = -DNS_EAI_SERVICE;
			goto error;
		}
	}

	if (port > 0U) {
		if (dns_result.ai_family == AF_INET) {
			net_sin(&dns_result_addr)->sin_port = htons(port);
		}
	}

	/* Check if node is an IP address */
	if (net_addr_pton(dns_result.ai_family, node,
			  &((struct sockaddr_in *)&dns_result_addr)->sin_addr) == 0) {
		*res = &dns_result;
		LOG_DBG("Already an IP address, returning");
		ret = 0;
		goto error;
	}

	/* user flagged node as numeric host, but we failed net_addr_pton */
	if (hints && hints->ai_flags & AI_NUMERICHOST) {
		LOG_ERR("Numeric host flag, but failed to convert address");
		ret = -DNS_EAI_NONAME;
		goto error;
	}

	/* Set data to be used by FSM */
	data->get_addrinfo.node = node;

	modem_add_request(data, MODEM_REQ_GET_ADDRINFO);

	/* Wait for script execution to be done */
	ret = wait_script_done(__func__, data, MDM_SCRIPT_DONE_TIMEOUT_SEC, 1);
	if (ret) {
		goto error;
	}

	*res = (struct zsock_addrinfo *)&dns_result;

error:
	return ret;
}

/*
 * Free addrinfo structure.
 */
static void offload_freeaddrinfo(struct modem_data *data, struct zsock_addrinfo *res)
{
	ARG_UNUSED(data);
	/* No need to free static memory. */
	res = NULL;
}

/* ~~~~ Network interface offload APIs ~~~~ */

/*
 * Enable or disable modem using AT+CFUN
 * when net_if_up/down() is called
 */
static int modem_net_iface_enable(const struct net_if *iface, bool state)
{
	int ret = 0;

	/* Get device associated to net IF */
	const struct device *dev = net_if_get_device((struct net_if *)iface);
	/* Get net IF context */
	struct net_if_data *if_data = dev->data;
	/* Get modem context */
	struct modem_data *data = if_data->modem_dev->data;

	LOG_DBG("Received iface %s", state ? "enable" : "disable");

	if (modem_state_get(data) >= MODEM_STATE_READY) {
		if (state) {
			modem_add_request(data, MODEM_REQ_IFACE_ENABLE);
		} else {
			modem_add_request(data, MODEM_REQ_IFACE_DISABLE);
		}

		/* Wait for script execution to be done */
		ret = wait_script_done(__func__, data, MDM_SCRIPT_DONE_TIMEOUT_SEC, 1);
	}

	return ret;
}

/*
 * Setup the Modem NET Interface
 */
static void modem_net_iface_init(struct net_if *iface)
{
	/* Get device associated to net IF */
	const struct device *dev = net_if_get_device(iface);
	/* Get modem context */
	struct modem_data *data = ((struct net_if_data *)dev->data)->modem_dev->data;
	/* Get modem config */
	const struct modem_config *config = ((struct net_if_data *)dev->data)->modem_dev->config;

	/* Set link addr */
	net_if_set_link_addr(iface, modem_get_mac(dev), sizeof(data->iface.mac_addr),
			     NET_LINK_ETHERNET);
	/* Register offloaded DNS APIs*/
	socket_offload_dns_register(&config->dns_ops);

	net_if_socket_offload_set(iface, config->sock_create);
}

/*
 * Offloaded API funcs
 */
static struct offloaded_if_api api_funcs = {
	.iface_api.init = modem_net_iface_init,
	.enable = modem_net_iface_enable,
};

/* ~~~~ GNSS driver APIs ~~~~ */

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf9160_gnss)
int get_supported_systems(const struct device *dev, gnss_systems_t *systems)
{
	*systems = GNSS_SYSTEM_GPS | GNSS_SYSTEM_QZSS;
	return 0;
}

int set_periodic_config(const struct device *dev,
			const struct gnss_periodic_config *periodic_config)
{
	int ret = 0;
	struct modem_data *data = dev->data;

	data->gnss_interval = periodic_config->inactive_time_ms;
	data->gnss_timeout = periodic_config->active_time_ms;

	return ret;
}

int get_periodic_config(const struct device *dev, struct gnss_periodic_config *periodic_config)
{
	int ret = 0;
	struct modem_data *data = dev->data;

	periodic_config->inactive_time_ms = data->gnss_interval;
	periodic_config->active_time_ms = data->gnss_timeout;

	return ret;
}

static struct gnss_driver_api gnss_api = {
	.set_fix_rate = NULL,
	.get_fix_rate = NULL,
	.set_periodic_config = set_periodic_config,
	.get_periodic_config = get_periodic_config,
	.set_navigation_mode = NULL,
	.get_navigation_mode = NULL,
	.set_enabled_systems = NULL,
	.get_enabled_systems = NULL,
	.get_supported_systems = get_supported_systems,
};
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf9160_gnss) */

/* ~~~~ Modem driver init functions ~~~~ */

static void init_dynamic_scripts(const struct device *dev)
{
	struct modem_data *data = dev->data;

	/* Dynamic script used for offloaded operations */

	/* Dynamic match that is updated at runtime */
	data->dynamic_match.match = data->dynamic_match_buf;
	data->dynamic_match.separators = data->dynamic_separators_buf;
	data->dynamic_match.separators_size = sizeof(data->dynamic_separators_buf);
	data->dynamic_match.wildcards = false;
	data->dynamic_match.partial = false;
	/* Dynamic request that is updated at runtime */
	data->dynamic_script_chat.request = data->dynamic_request_buf;
	data->dynamic_script_chat.response_matches = &data->dynamic_match;
	data->dynamic_script_chat.response_matches_size = 1;
	data->dynamic_script_chat.timeout = 0;

	data->dynamic_script.name = "dynamic";
	data->dynamic_script.script_chats = &data->dynamic_script_chat;
	data->dynamic_script.script_chats_size = 1;
	data->dynamic_script.abort_matches = abort_matches;
	data->dynamic_script.abort_matches_size = ARRAY_SIZE(abort_matches);
	data->dynamic_script.callback = modem_chat_handler;
	data->dynamic_script.timeout = MDM_DYNAMIC_SCRIPT_TIMEOUT_SEC;
}

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf9160_gnss)
static int gnss_init(const struct device *dev)
{
	int ret = 0;
	struct modem_data *data = (struct modem_data *)dev->data;

	/* Store reference to GNSS device in modem data */
	data->gnss_dev = dev;

	/* Init GNSS as suspended */
	pm_device_init_suspended(dev);

	return ret;
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf9160_gnss) */

static int modem_init(const struct device *dev)
{
	int ret = 0;
	struct modem_data *data = (struct modem_data *)dev->data;
	struct modem_config *config = (struct modem_config *)dev->config;

	/* Store reference to device itself in modem data */
	data->dev = dev;
	/* Get device associated to net IF */
	const struct device *if_dev = net_if_get_device(data->iface.net_iface);
	/* Store a reference to modem device into net_if data */
	((struct net_if_data *)if_dev->data)->modem_dev = dev;

	/* Initialize FSM worker */
	k_work_init(&data->event_dispatch_work, modem_event_dispatch_handler);
	/* Initialize event's ring buffer */
	ring_buf_init(&data->event_rb, sizeof(data->event_buf), data->event_buf);
	/* Initialize request worker */
	k_work_init_delayable(&data->request_dispatch_work, modem_request_dispatch_handler);
	/* Initialize request's ring buffer */
	ring_buf_init(&data->request_rb, sizeof(data->request_buf), data->request_buf);

	/* Socket config */
	ret = modem_socket_init(&data->socket_config, data->sockets, ARRAY_SIZE(data->sockets),
				MDM_BASE_SOCKET_NUM, false, &offload_socket_fd_op_vtable);
	if (ret < 0) {
		LOG_ERR("Socket init error %d", ret);
		goto error;
	}

	/* Semaphores */
	k_sem_init(&data->sem_state, 1, 1);
	k_sem_init(&data->sem_script_exec, 1, 1);
	k_sem_init(&data->sem_script_done, 0, 1);
	k_sem_init(&data->sem_script_sync, 1, 1);

	k_work_queue_init(&_modem_workq);
	struct k_work_queue_config cfg = {
		.name = "modem_workq",
		.no_yield = false,
	};
	k_work_queue_start(&_modem_workq, _modem_workq_stack_area,
			   K_THREAD_STACK_SIZEOF(_modem_workq_stack_area),
			   CONFIG_MODEM_NORDIC_NRF9160_WORKQ_PRV_INIT_PRIORITY, &cfg);

	if (config->power_gpio.port != NULL) {
		gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_INACTIVE);
	}

	if (config->reset_gpio.port != NULL) {
		gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
	}

	const struct modem_backend_uart_config uart_backend_config = {
		.uart = config->uart,
		.receive_buf = data->uart_backend_receive_buf,
		.receive_buf_size = ARRAY_SIZE(data->uart_backend_receive_buf),
		.transmit_buf = data->uart_backend_transmit_buf,
		.transmit_buf_size = ARRAY_SIZE(data->uart_backend_transmit_buf),
	};

	data->uart_pipe = modem_backend_uart_init(&data->uart_backend, &uart_backend_config);
	if (data->uart_pipe == NULL) {
		LOG_ERR("Failed to init UART backend");
		goto error;
	}

	const struct modem_chat_config chat_config = {
		.user_data = data,
		.receive_buf = data->chat_receive_buf,
		.receive_buf_size = ARRAY_SIZE(data->chat_receive_buf),
		.delimiter = data->chat_delimiter,
		.delimiter_size = ARRAY_SIZE(data->chat_delimiter),
		.filter = NULL,
		.filter_size = 0,
		.argv = data->chat_argv,
		.argv_size = ARRAY_SIZE(data->chat_argv),
		.unsol_matches = unsol_matches,
		.unsol_matches_size = ARRAY_SIZE(unsol_matches),
		.process_timeout = K_MSEC(2),
	};

	ret = modem_chat_init(&data->chat, &chat_config);
	if (ret < 0) {
		LOG_ERR("Modem chat init error %d", ret);
		goto error;
	}

	/* Initialize structs used for dynamic char scripts */
	init_dynamic_scripts(dev);

#if IS_ENABLED(CONFIG_MODEM_NORDIC_NRF9160_AUTOINIT)
	/* Push RESUME event to initialize modem */
	modem_add_event(data, MODEM_EVENT_RESUME);

	/* Wait until the modem is initialized */
	ret = wait_script_done(__func__, data, MDM_INIT_TIMEOUT_SEC, 1);
	if (ret < 0) {
		LOG_ERR("Modem init error %d", ret);
		ret = -ETIMEDOUT;
		goto error;
	}
#elif IS_ENABLED(CONFIG_PM_DEVICE)
	pm_device_init_suspended(dev);
#endif

	LOG_INF("Modem initialized!");
error:
	return ret;
}

/* ~~~~ Public APIs exposed in zephyr/drivers/modem/nordic_nrf9160.h ~~~~ */

int mdm_nrf9160_reset(const struct device *dev)
{
	int ret = 0;
	struct modem_data *data = dev->data;

	modem_add_request(data, MODEM_REQ_RESET);

	/* Wait for semaphore to signal init done */
	ret = wait_script_done(__func__, data, MDM_RESET_TIMEOUT_SEC, 1);
	if (ret < 0) {
		LOG_ERR("Reset operation timed out");
		ret = -ETIMEDOUT;
	} else {
		LOG_INF("Modem reset success!");
	}

#if IS_ENABLED(CONFIG_MODEM_NORDIC_NRF9160_AUTOINIT)
	modem_net_iface_enable(data->iface.net_iface, true);
#endif

	return ret;
}

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	init_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 1000),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 1000), MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 1000),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 1000),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP(MDM_SETUP_CMD_SYSTEM_MODE, ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT%XBANDLOCK=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+COPS=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP(MDM_SETUP_CMD_PDP_CTX, ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPSMS=1,\"\",\"\",\"10101010\",\"00100001\"", ok_match),

	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGSN", imei_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGMI", manufacturer_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGMM", model_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_MULT("AT+CGMR", revision_match));

MODEM_CHAT_SCRIPT_DEFINE(init_chat_script, init_chat_script_cmds, abort_matches, modem_chat_handler,
			 MDM_INIT_SCRIPT_TIMEOUT_SECONDS);

MODEM_CHAT_SCRIPT_CMDS_DEFINE(
	reset_chat_script_cmds, MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT#XRESET", ready_match),
	MODEM_CHAT_SCRIPT_CMD_RESP_NONE("AT", 100),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CFUN=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP(MDM_SETUP_CMD_SYSTEM_MODE, ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT%XBANDLOCK=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+COPS=0", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP(MDM_SETUP_CMD_PDP_CTX, ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CEREG=1", ok_match),
	MODEM_CHAT_SCRIPT_CMD_RESP("AT+CPSMS=1,\"\",\"\",\"10101010\",\"00100001\"", ok_match));

MODEM_CHAT_SCRIPT_DEFINE(reset_chat_script, reset_chat_script_cmds, abort_matches,
			 modem_chat_handler, MDM_RESET_SCRIPT_TIMEOUT_SEC);

#define NRF9160_DEVICE_INST_NAME(name, inst) _CONCAT(_CONCAT(_CONCAT(name, _), DT_DRV_COMPAT), inst)

#define NRF9160_DEVICE(inst)                                                                       \
	static struct net_if_data net_if_data##inst;                                               \
                                                                                                   \
	NET_DEVICE_INIT(NRF9160_DEVICE_INST_NAME(net_if, inst), "net_if_nrf9160" #inst, NULL,      \
			NULL, &net_if_data##inst, NULL,                                            \
			CONFIG_MODEM_NORDIC_NRF9160_NET_IF_INIT_PRIORITY, &api_funcs,              \
			OFFLOADED_NETDEV_L2, NET_L2_GET_CTX_TYPE(OFFLOADED_NETDEV_L2), 1500);      \
                                                                                                   \
	static struct modem_data NRF9160_DEVICE_INST_NAME(data, inst) = {                          \
		.chat_delimiter = {'\r', '\n'},                                                    \
		.dynamic_separators_buf = {','},                                                   \
		.iface =                                                                           \
			{                                                                          \
				.net_iface =                                                       \
					NET_IF_GET(NRF9160_DEVICE_INST_NAME(net_if, inst), 0),     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	int offload_getaddrinfo##inst(const char *node, const char *service,                       \
				      const struct zsock_addrinfo *hints,                          \
				      struct zsock_addrinfo **res)                                 \
	{                                                                                          \
		return offload_getaddrinfo(&NRF9160_DEVICE_INST_NAME(data, inst), node, service,   \
					   hints, res);                                            \
	}                                                                                          \
                                                                                                   \
	void offload_freeaddrinfo##inst(struct zsock_addrinfo *res)                                \
	{                                                                                          \
		offload_freeaddrinfo(&NRF9160_DEVICE_INST_NAME(data, inst), res);                  \
	}                                                                                          \
                                                                                                   \
	static int offload_socket##inst(int family, int type, int proto)                           \
	{                                                                                          \
		return offload_socket(&NRF9160_DEVICE_INST_NAME(data, inst), family, type, proto); \
	}                                                                                          \
                                                                                                   \
	NET_SOCKET_OFFLOAD_REGISTER(inst, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,          \
				    offload_is_supported, offload_socket##inst);                   \
                                                                                                   \
	static struct modem_config NRF9160_DEVICE_INST_NAME(config, inst) = {                      \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {}),                 \
		.init_chat_script = &init_chat_script,                                             \
		.reset_chat_script = &reset_chat_script,                                           \
		.dns_ops =                                                                         \
			{                                                                          \
				.getaddrinfo = offload_getaddrinfo##inst,                          \
				.freeaddrinfo = offload_freeaddrinfo##inst,                        \
			},                                                                         \
		.sock_create = offload_socket##inst,                                               \
	};                                                                                         \
                                                                                                   \
	IF_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf9160_gnss),                                 \
		   (PM_DEVICE_DT_DEFINE(DT_INST(inst, nordic_nrf9160_gnss), gnss_pm_action);       \
                                                                                                   \
		    DEVICE_DT_DEFINE(DT_INST(inst, nordic_nrf9160_gnss), gnss_init,                \
				     PM_DEVICE_DT_GET(DT_INST(inst, nordic_nrf9160_gnss)),         \
				     &NRF9160_DEVICE_INST_NAME(data, inst),                        \
				     &NRF9160_DEVICE_INST_NAME(config, inst), POST_KERNEL,         \
				     CONFIG_MODEM_NORDIC_NRF9160_INIT_PRIORITY, &gnss_api)));      \
                                                                                                   \
	PM_DEVICE_DT_DEFINE(DT_INST(inst, nordic_nrf9160), modem_pm_action);                       \
                                                                                                   \
	DEVICE_DT_DEFINE(DT_INST(inst, nordic_nrf9160), modem_init,                                \
			 PM_DEVICE_DT_GET(DT_INST(inst, nordic_nrf9160)),                          \
			 &NRF9160_DEVICE_INST_NAME(data, inst),                                    \
			 &NRF9160_DEVICE_INST_NAME(config, inst), POST_KERNEL,                     \
			 CONFIG_MODEM_NORDIC_NRF9160_INIT_PRIORITY, NULL);

#define DT_DRV_COMPAT nordic_nrf9160
DT_INST_FOREACH_STATUS_OKAY(NRF9160_DEVICE)
#undef DT_DRV_COMPAT
