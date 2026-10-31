/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_backend.h"

#define DT_DRV_COMPAT zephyr_modem_cellular_test

#include <zephyr/modem/backend/uart.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/modem/ppp.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ppp.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/ztest.h>

#include <string.h>

LOG_MODULE_REGISTER(modem_cellular_test_backend, LOG_LEVEL_INF);

/*
 * This file supplies the non-chat pieces that modem_cellular.c normally gets
 * from the modem module stack. The test intentionally enables the real
 * MODEM_CHAT and MODEM_PIPELINK modules only; UART backend, CMUX, and PPP are
 * replaced with small synchronous shims so tests can drive the state machine one
 * AT line at a time.
 */

/* ------------------------------------------------------------------------- */
/* Scriptable modem pipes                                                    */
/* ------------------------------------------------------------------------- */

struct test_pipe {
	struct modem_pipe pipe;
	struct ring_buf rx_rb;
	uint8_t rx_buf[512];
	enum test_backend_pipe_id id;
	bool open;
	char line[128];
	size_t line_len;
};

struct test_backend {
	const struct device *modem;
	const struct device *uart;
	struct gpio_dt_spec power_gpio;
	struct gpio_dt_spec reset_gpio;
	bool set_baudrate_script;
	bool network_script;
	bool dlci_setup_script;
	bool periodic_script;
	bool modem_info_script;
	struct modem_cmux *cmux;
	bool cmux_connecting;
	bool cmux_connected;
	struct modem_ppp *ppp;
	struct test_pipe uart_pipe;
	struct test_pipe dlci1_pipe;
	struct test_pipe dlci2_pipe;
	struct test_pipe user3_pipe;
	struct test_pipe user4_pipe;
};

static const char *test_backend_pipe_name(enum test_backend_pipe_id pipe)
{
	switch (pipe) {
	case TEST_BACKEND_PIPE_UART:
		return "UART";
	case TEST_BACKEND_PIPE_DLCI1:
		return "DLCI1";
	case TEST_BACKEND_PIPE_DLCI2:
		return "DLCI2";
	case TEST_BACKEND_PIPE_USER3:
		return "USER3";
	case TEST_BACKEND_PIPE_USER4:
		return "USER4";
	default:
		return "unknown";
	}
}

K_MSGQ_DEFINE(line_msgq, sizeof(struct test_backend_line), 16, 4);

#define TEST_BACKEND_DEFINE(inst)                                                                  \
	{                                                                                          \
		.modem = DEVICE_DT_INST_GET(inst),                                                 \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {0}),                \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_reset_gpios, {0}),                \
		.set_baudrate_script = !DT_INST_PROP(inst, no_set_baudrate_script),                \
		.network_script = !DT_INST_PROP(inst, no_network_script),                          \
		.dlci_setup_script = !DT_INST_PROP(inst, no_dlci_setup_script),                    \
		.periodic_script = !DT_INST_PROP(inst, no_periodic_script),                        \
		.modem_info_script = DT_INST_PROP(inst, modem_info_script),                        \
		.uart_pipe = {.id = TEST_BACKEND_PIPE_UART},                                       \
		.dlci1_pipe = {.id = TEST_BACKEND_PIPE_DLCI1},                                     \
		.dlci2_pipe = {.id = TEST_BACKEND_PIPE_DLCI2},                                     \
		.user3_pipe = {.id = TEST_BACKEND_PIPE_USER3},                                     \
		.user4_pipe = {.id = TEST_BACKEND_PIPE_USER4},                                     \
	},

static struct test_backend test_backends[] = {DT_INST_FOREACH_STATUS_OKAY(TEST_BACKEND_DEFINE)};
static struct test_backend *active_backend;
static bool cmux_connect_hold;
/* Set when a driver line was truncated or did not fit in the queue */
static bool lines_lost;

static struct test_backend *test_backend_from_modem(const struct device *modem)
{
	for (size_t i = 0; i < ARRAY_SIZE(test_backends); i++) {
		if (test_backends[i].modem == modem) {
			return &test_backends[i];
		}
	}

	__ASSERT(false, "%s is not a test modem", modem->name);
	return NULL;
}

static struct test_backend *test_backend_from_uart(const struct device *uart)
{
	for (size_t i = 0; i < ARRAY_SIZE(test_backends); i++) {
		if (test_backends[i].uart == uart) {
			return &test_backends[i];
		}
	}

	return NULL;
}

static struct test_backend *test_backend_from_cmux(const struct modem_cmux *cmux)
{
	for (size_t i = 0; i < ARRAY_SIZE(test_backends); i++) {
		if (test_backends[i].cmux == cmux) {
			return &test_backends[i];
		}
	}

	return NULL;
}

static void test_backend_record_byte(struct test_pipe *tpipe, uint8_t byte)
{
	struct test_backend_line line = {
		.pipe = tpipe->id,
	};

	if (byte == '\n') {
		return;
	}

	if (byte == '\r') {
		tpipe->line[tpipe->line_len] = '\0';
		LOG_INF("%s <- modem_cellular: %s", test_backend_pipe_name(tpipe->id), tpipe->line);
		strncpy(line.data, tpipe->line, sizeof(line.data) - 1);
		/* Runs on the driver's thread, so leave asserting to the test thread */
		if (k_msgq_put(&line_msgq, &line, K_NO_WAIT) != 0) {
			lines_lost = true;
		}
		tpipe->line_len = 0;
		return;
	}

	if (tpipe->line_len < sizeof(tpipe->line) - 1) {
		tpipe->line[tpipe->line_len++] = byte;
	} else {
		lines_lost = true;
	}
}

static int test_pipe_open(void *data)
{
	struct test_pipe *tpipe = data;

	tpipe->open = true;
	modem_pipe_notify_opened(&tpipe->pipe);
	return 0;
}

static int test_pipe_transmit_chain(void *data, const struct modem_pipe_data_fragment *frags,
				    size_t num_frags)
{
	struct test_pipe *tpipe = data;
	size_t total = 0;

	for (size_t i = 0; i < num_frags; i++) {
		for (size_t j = 0; j < frags[i].size; j++) {
			test_backend_record_byte(tpipe, frags[i].data[j]);
		}
		total += frags[i].size;
	}

	modem_pipe_notify_transmit_idle(&tpipe->pipe);
	return total;
}

static int test_pipe_receive(void *data, uint8_t *buf, size_t size)
{
	struct test_pipe *tpipe = data;

	return ring_buf_get(&tpipe->rx_rb, buf, size);
}

static int test_pipe_close(void *data)
{
	struct test_pipe *tpipe = data;

	tpipe->open = false;
	modem_pipe_notify_closed(&tpipe->pipe);
	return 0;
}

static const struct modem_pipe_api test_pipe_api = {
	.open = test_pipe_open,
	.transmit_chain = test_pipe_transmit_chain,
	.receive = test_pipe_receive,
	.close = test_pipe_close,
};

static void test_pipe_init_once(struct test_pipe *tpipe)
{
	ring_buf_init(&tpipe->rx_rb, sizeof(tpipe->rx_buf), tpipe->rx_buf);
	modem_pipe_init(&tpipe->pipe, tpipe, &test_pipe_api);
}

void test_backend_reset(const struct device *modem)
{
	active_backend = test_backend_from_modem(modem);

	k_msgq_purge(&line_msgq);
	lines_lost = false;
	cmux_connect_hold = false;
	active_backend->cmux_connecting = false;
	active_backend->cmux_connected = false;

	active_backend->uart_pipe.open = false;
	active_backend->dlci1_pipe.open = false;
	active_backend->dlci2_pipe.open = false;
	active_backend->user3_pipe.open = false;
	active_backend->user4_pipe.open = false;

	active_backend->uart_pipe.line_len = 0;
	active_backend->dlci1_pipe.line_len = 0;
	active_backend->dlci2_pipe.line_len = 0;
	active_backend->user3_pipe.line_len = 0;
	active_backend->user4_pipe.line_len = 0;

	ring_buf_reset(&active_backend->uart_pipe.rx_rb);
	ring_buf_reset(&active_backend->dlci1_pipe.rx_rb);
	ring_buf_reset(&active_backend->dlci2_pipe.rx_rb);
	ring_buf_reset(&active_backend->user3_pipe.rx_rb);
	ring_buf_reset(&active_backend->user4_pipe.rx_rb);
}

enum test_backend_state test_backend_state(const struct device *modem)
{
	const struct test_backend *backend = test_backend_from_modem(modem);
	const struct modem_ppp *ppp = backend->ppp;

	if ((ppp != NULL) && (ppp->pipe != NULL)) {
		return net_if_is_dormant(ppp->iface) ? TEST_BACKEND_STATE_DIALED
						     : TEST_BACKEND_STATE_LINK_UP;
	}

	if (backend->cmux_connected) {
		return TEST_BACKEND_STATE_CMUX;
	}

	return backend->uart_pipe.open ? TEST_BACKEND_STATE_AT : TEST_BACKEND_STATE_OFF;
}

const struct device *test_backend_uart(const struct device *modem)
{
	return test_backend_from_modem(modem)->uart;
}

const struct gpio_dt_spec *test_backend_power_gpio(const struct device *modem)
{
	return &test_backend_from_modem(modem)->power_gpio;
}

const struct gpio_dt_spec *test_backend_reset_gpio(const struct device *modem)
{
	return &test_backend_from_modem(modem)->reset_gpio;
}

bool test_backend_has_set_baudrate_script(const struct device *modem)
{
	return test_backend_from_modem(modem)->set_baudrate_script;
}

bool test_backend_has_network_script(const struct device *modem)
{
	return test_backend_from_modem(modem)->network_script;
}

bool test_backend_has_dlci_setup_script(const struct device *modem)
{
	return test_backend_from_modem(modem)->dlci_setup_script;
}

bool test_backend_has_periodic_script(const struct device *modem)
{
	return test_backend_from_modem(modem)->periodic_script;
}

bool test_backend_has_modem_info_script(const struct device *modem)
{
	return test_backend_from_modem(modem)->modem_info_script;
}

bool test_backend_wait_for_line(struct test_backend_line *line, k_timeout_t timeout)
{
	bool received = k_msgq_get(&line_msgq, line, timeout) == 0;

	zassert_false(lines_lost, "driver lines were truncated or dropped");
	return received;
}

static struct test_pipe *test_backend_pipe(enum test_backend_pipe_id pipe)
{
	switch (pipe) {
	case TEST_BACKEND_PIPE_UART:
		return &active_backend->uart_pipe;
	case TEST_BACKEND_PIPE_DLCI1:
		return &active_backend->dlci1_pipe;
	case TEST_BACKEND_PIPE_DLCI2:
		return &active_backend->dlci2_pipe;
	case TEST_BACKEND_PIPE_USER3:
		return &active_backend->user3_pipe;
	case TEST_BACKEND_PIPE_USER4:
		return &active_backend->user4_pipe;
	default:
		zassert_unreachable("unknown pipe %d", pipe);
		return NULL;
	}
}

void test_backend_respond(enum test_backend_pipe_id pipe, const char *response)
{
	struct test_pipe *tpipe = test_backend_pipe(pipe);
	const char *line;

	line = response;
	while (*line != '\0') {
		size_t line_len = strcspn(line, "\r\n");

		if (line_len > 0) {
			LOG_INF("%s -> modem_cellular: %.*s", test_backend_pipe_name(pipe),
				(int)line_len, line);
		}
		line += line_len;
		line += strspn(line, "\r\n");
	}

	zassert_equal(strlen(response),
		      ring_buf_put(&tpipe->rx_rb, (const uint8_t *)response, strlen(response)),
		      "%s receive buffer full", test_backend_pipe_name(pipe));
	modem_pipe_notify_receive_ready(&tpipe->pipe);
}

bool test_backend_pipe_is_open(enum test_backend_pipe_id pipe)
{
	return test_backend_pipe(pipe)->open;
}

bool test_backend_wait_for_read(enum test_backend_pipe_id pipe, k_timeout_t timeout)
{
	struct test_pipe *tpipe = test_backend_pipe(pipe);
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (!ring_buf_is_empty(&tpipe->rx_rb)) {
		if (sys_timepoint_expired(end)) {
			return false;
		}
		k_msleep(1);
	}

	return true;
}

/* ------------------------------------------------------------------------- */
/* UART backend shim                                                         */
/* ------------------------------------------------------------------------- */

struct modem_pipe *modem_backend_uart_init(struct modem_backend_uart *backend,
					   const struct modem_backend_uart_config *config)
{
	struct test_backend *tbackend = test_backend_from_uart(config->uart);

	ARG_UNUSED(backend);
	__ASSERT_NO_MSG(tbackend != NULL);

	test_pipe_init_once(&tbackend->uart_pipe);
	return &tbackend->uart_pipe.pipe;
}

/* ------------------------------------------------------------------------- */
/* CMUX shim                                                                 */
/* ------------------------------------------------------------------------- */

/* Find the backend of the driver instance initialising a CMUX with @p config */
static struct test_backend *test_backend_from_cmux_config(const struct modem_cmux_config *config)
{
#ifdef CONFIG_MODEM_CELLULAR
	const struct modem_cellular_data *data = config->user_data;

	for (size_t i = 0; i < ARRAY_SIZE(test_backends); i++) {
		if (&test_backends[i].uart_pipe.pipe == data->uart_pipe) {
			return &test_backends[i];
		}
	}

	return NULL;
#else
#error "No cellular modem implementation to find the CMUX test backend of"
#endif
}

void modem_cmux_init(struct modem_cmux *cmux, const struct modem_cmux_config *config)
{
	struct test_backend *tbackend = test_backend_from_cmux_config(config);

	__ASSERT_NO_MSG(tbackend != NULL);
	tbackend->cmux = cmux;

	memset(cmux, 0, sizeof(*cmux));
	cmux->config = *config;

	test_pipe_init_once(&tbackend->dlci1_pipe);
	test_pipe_init_once(&tbackend->dlci2_pipe);
	test_pipe_init_once(&tbackend->user3_pipe);
	test_pipe_init_once(&tbackend->user4_pipe);
}

struct modem_pipe *modem_cmux_dlci_init(struct modem_cmux *cmux, struct modem_cmux_dlci *dlci,
					const struct modem_cmux_dlci_config *config)
{
	struct test_backend *tbackend = test_backend_from_cmux(cmux);

	ARG_UNUSED(dlci);
	__ASSERT_NO_MSG(tbackend != NULL);

	switch (config->dlci_address) {
	case 1:
		return &tbackend->dlci1_pipe.pipe;
	case 2:
		return &tbackend->dlci2_pipe.pipe;
	case 3:
		return &tbackend->user3_pipe.pipe;
	case 4:
		return &tbackend->user4_pipe.pipe;
	default:
		__ASSERT(false, "no test pipe for DLCI %u", config->dlci_address);
		return NULL;
	}
}

/* Takes over the bus pipe as the real CMUX does, so the driver no longer sees its events */
static void test_cmux_bus_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				   void *user_data)
{
	ARG_UNUSED(pipe);
	ARG_UNUSED(event);
	ARG_UNUSED(user_data);
}

int modem_cmux_attach(struct modem_cmux *cmux, struct modem_pipe *pipe)
{
	if (cmux->pipe != NULL) {
		return -EALREADY;
	}

	cmux->pipe = pipe;
	modem_pipe_attach(pipe, test_cmux_bus_callback, cmux);
	return 0;
}

void test_backend_cmux_power_save(void)
{
	zassert_not_null(active_backend->cmux->pipe, "CMUX is not attached");
	zassert_ok(modem_pipe_close_async(active_backend->cmux->pipe));
}

static void test_backend_cmux_settle(struct test_backend *tbackend, bool connected)
{
	struct modem_cmux *cmux = tbackend->cmux;
	/* A failed connect reports DISCONNECTED once the retries run out */
	enum modem_cmux_event event =
		connected ? MODEM_CMUX_EVENT_CONNECTED : MODEM_CMUX_EVENT_DISCONNECTED;

	tbackend->cmux_connecting = false;
	tbackend->cmux_connected = connected;

	if (cmux->config.callback != NULL) {
		cmux->config.callback(cmux, event, cmux->config.user_data);
	}
}

void test_backend_cmux_connect_hold(bool hold)
{
	cmux_connect_hold = hold;
}

bool test_backend_wait_for_cmux_connect(k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (!active_backend->cmux_connecting) {
		if (sys_timepoint_expired(end)) {
			return false;
		}
		k_msleep(1);
	}

	return true;
}

void test_backend_cmux_connect_finish(bool connected)
{
	zassert_true(active_backend->cmux_connecting, "no CMUX connect is pending");
	test_backend_cmux_settle(active_backend, connected);
}

int modem_cmux_connect(struct modem_cmux *cmux)
{
	struct test_backend *tbackend = test_backend_from_cmux(cmux);

	tbackend->cmux_connecting = true;
	if (!cmux_connect_hold) {
		test_backend_cmux_settle(tbackend, true);
	}

	return 0;
}

int modem_cmux_connect_async(struct modem_cmux *cmux)
{
	return modem_cmux_connect(cmux);
}

int modem_cmux_disconnect(struct modem_cmux *cmux)
{
	test_backend_from_cmux(cmux)->cmux_connected = false;
	if (cmux->config.callback != NULL) {
		cmux->config.callback(cmux, MODEM_CMUX_EVENT_DISCONNECTED, cmux->config.user_data);
	}

	return 0;
}

int modem_cmux_disconnect_async(struct modem_cmux *cmux)
{
	return modem_cmux_disconnect(cmux);
}

void modem_cmux_release(struct modem_cmux *cmux)
{
	struct test_backend *tbackend = test_backend_from_cmux(cmux);

	/* The driver may release a CMUX it never initialised */
	if (tbackend != NULL) {
		tbackend->cmux_connecting = false;
		tbackend->cmux_connected = false;
	}
	if (cmux->pipe != NULL) {
		modem_pipe_release(cmux->pipe);
	}
	cmux->pipe = NULL;
}

/* ------------------------------------------------------------------------- */
/* PPP shim                                                                  */
/* ------------------------------------------------------------------------- */

int modem_ppp_attach(struct modem_ppp *ppp, struct modem_pipe *pipe)
{
	for (size_t i = 0; i < ARRAY_SIZE(test_backends); i++) {
		if (&test_backends[i].dlci2_pipe.pipe == pipe) {
			test_backends[i].ppp = ppp;
		}
	}

	ppp->pipe = pipe;
	return 0;
}

struct net_if *modem_ppp_get_iface(struct modem_ppp *ppp)
{
	return ppp->iface;
}

void modem_ppp_release(struct modem_ppp *ppp)
{
	ppp->pipe = NULL;
}

int modem_ppp_init_internal(const struct device *dev)
{
	struct modem_ppp *ppp = dev->data;

	ppp->iface = net_if_lookup_by_dev(dev);
	return 0;
}

static void test_ppp_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct modem_ppp *ppp = dev->data;

	net_ppp_init(iface);
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_carrier_off(iface);
	net_if_dormant_on(iface);

	ppp->iface = iface;
}

static int test_ppp_start(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int test_ppp_stop(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

/* Answer an LCP Terminate-Request as the peer would, so that net_if_down() completes */
static void test_ppp_ack_terminate(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_pkt *ack;
	uint16_t protocol;
	uint8_t code;
	uint8_t id;

	net_pkt_cursor_init(pkt);
	if ((net_pkt_read_be16(pkt, &protocol) < 0) || (protocol != PPP_LCP) ||
	    (net_pkt_read_u8(pkt, &code) < 0) || (code != PPP_TERMINATE_REQ) ||
	    (net_pkt_read_u8(pkt, &id) < 0)) {
		return;
	}

	ack = net_pkt_rx_alloc_with_buffer(iface, 6, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (ack == NULL) {
		LOG_ERR("no buffer for the LCP Terminate-Ack");
		return;
	}

	(void)net_pkt_write_be16(ack, PPP_LCP);
	(void)net_pkt_write_u8(ack, PPP_TERMINATE_ACK);
	(void)net_pkt_write_u8(ack, id);
	(void)net_pkt_write_be16(ack, 4);
	net_pkt_cursor_init(ack);
	net_pkt_set_ppp(ack, true);

	if (net_recv_data(iface, ack) < 0) {
		net_pkt_unref(ack);
	}
}

static int test_ppp_send(const struct device *dev, struct net_pkt *pkt)
{
	struct modem_ppp *ppp = dev->data;

	test_ppp_ack_terminate(ppp->iface, pkt);
	return 0;
}

const struct ppp_api modem_ppp_ppp_api = {
	.iface_api.init = test_ppp_iface_init,
	.start = test_ppp_start,
	.stop = test_ppp_stop,
	.send = test_ppp_send,
};
