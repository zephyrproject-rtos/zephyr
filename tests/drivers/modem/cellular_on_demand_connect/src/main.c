/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FSM tests for the modem_cellular driver's on-demand-connect feature
 * (CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT). A Quectel EG25-G modem is
 * instantiated on an emulated UART (zephyr,uart-emul). The tests play the modem
 * (DCE) side of the dialogue so the driver walks its state machine, and assert
 * the driver's FSM state directly instead of inferring it from the AT traffic,
 * so each test pins down which transition it exercises.
 *
 * The emulator has two phases. Before CMUX it answers the raw init AT script
 * directly over the emulated UART. Once the driver sends AT+CMUX it switches to
 * a modem_cmux instance acting as the DCE peer (as in
 * tests/subsys/modem/modem_cmux_pair), bridged to the emulated UART through the
 * modem backend mock, so all CMUX framing is handled for us and the per-DLCI AT
 * dialogue (APN, periodic, dial) is answered on the decoded channels.
 *
 * Three Twister scenarios share this file:
 *
 *   - the default one walks the full dial / hang-up cycle;
 *   - CONFIG_TEST_ON_DEMAND_EARLY_ADMIN_UP=y admits the PPP interface before the
 *     modem finishes booting, so the ADMIN_UP edge arrives while the FSM cannot
 *     act on it;
 *   - CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT=n guards the documented default,
 *     where the data call is dialled unprompted as part of power-up.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/pm/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>

#include <modem_backend_mock.h>

#include <string.h>

static const struct device *const modem = DEVICE_DT_GET(DT_ALIAS(modem));
static const struct device *const modem_uart = DEVICE_DT_GET(DT_NODELABEL(modem_uart));

/* Observation shared with the emulator. */
static atomic_t atd_count;    /* number of ATD dial attempts seen on DLCI2 */
static atomic_t csq_count;    /* number of AT+CSQ polls seen on DLCI1 (periodic script) */
static struct k_event emu_events;
#define EV_APN_DONE BIT(0)    /* APN script answered => boot got past RUN_APN_SCRIPT */

/* Knobs the tests flip to steer the emulated modem. */
static atomic_t emu_registered;    /* report registration on the next AT+CEREG? poll */
static atomic_t emu_hold_connect;  /* count ATD but withhold CONNECT */

/* ------------------------------------------------------------------------- */
/* Modem (DCE) emulator                                                      */
/* ------------------------------------------------------------------------- */

enum emu_phase {
	EMU_PHASE_AT,   /* raw init AT script, straight over the emulated UART */
	EMU_PHASE_CMUX, /* driver switched to CMUX; route through the DCE peer */
};

static enum emu_phase emu_phase;
static struct k_mutex pump_lock;

/* CMUX DCE peer + its two DLCI channels. */
static struct modem_cmux dce_cmux;
static uint8_t dce_cmux_rx[512];
static uint8_t dce_cmux_tx[512];
static struct modem_cmux_dlci dce_dlci1;
static struct modem_cmux_dlci dce_dlci2;
static struct modem_pipe *dce_dlci1_pipe;
static struct modem_pipe *dce_dlci2_pipe;
static uint8_t dce_dlci1_rx[256];
static uint8_t dce_dlci2_rx[256];

/* Bridge: wire_mock <-> dce_bus_mock. The DCE CMUX attaches to dce_bus; the
 * test owns wire and shuttles bytes between it and the emulated UART.
 */
static struct modem_backend_mock wire_mock;
static struct modem_backend_mock dce_bus_mock;
static uint8_t wire_rx[2048];
static uint8_t wire_tx[2048];
static uint8_t dce_bus_rx[2048];
static uint8_t dce_bus_tx[2048];
static struct modem_pipe *wire_pipe;
static struct modem_pipe *dce_bus_pipe;

/* Per-channel line assembly for the AT responders. */
static uint8_t at_line[256];
static size_t at_line_len;
static uint8_t dlci1_line[256];
static size_t dlci1_line_len;
static uint8_t dlci2_line[256];
static size_t dlci2_line_len;

static void to_driver(const char *resp)
{
	uart_emul_put_rx_data(modem_uart, (const uint8_t *)resp, strlen(resp));
}

static void dce_send(struct modem_pipe *pipe, const char *resp)
{
	modem_pipe_transmit(pipe, (const uint8_t *)resp, strlen(resp));
}

/* Answer one raw init-script line (pre-CMUX), or trigger the CMUX switch. */
static void at_phase_respond(const char *line)
{
	if (line[0] == '\0') {
		/* The script's empty "" step probes for the trailing OK after a
		 * value query; that OK is already bundled with the value below,
		 * so there is nothing to send here.
		 */
	} else if (strncmp(line, "AT+CMUX", 7) == 0) {
		/* CMD_RESP_NONE: the driver expects no reply and then brings up
		 * CMUX. From here on route bytes through the DCE peer.
		 */
		emu_phase = EMU_PHASE_CMUX;
	} else if (strcmp(line, "AT+CGSN") == 0) {
		to_driver("359123456789012\r\nOK\r\n");
	} else if (strcmp(line, "AT+CGMM") == 0) {
		to_driver("EG25\r\nOK\r\n");
	} else if (strcmp(line, "AT+CGMI") == 0) {
		to_driver("Quectel\r\nOK\r\n");
	} else if (strcmp(line, "AT+CGMR") == 0) {
		to_driver("EG25GGBR07A08M2G\r\nOK\r\n");
	} else if (strcmp(line, "AT+CIMI") == 0) {
		to_driver("460001234567890\r\nOK\r\n");
	} else {
		/* ATE0, AT+CFUN=4, AT+CMEE=1, AT+C*REG[=?], ... */
		to_driver("OK\r\n");
	}
}

/* Answer AT commands the driver runs on DLCI1: APN + periodic script. */
static void dlci1_respond(const char *line)
{
	if (line[0] == '\0') {
		dce_send(dce_dlci1_pipe, "OK\r\n");
	} else if (strncmp(line, "AT+CGDCONT", 10) == 0) {
		dce_send(dce_dlci1_pipe, "OK\r\n");
		k_event_post(&emu_events, EV_APN_DONE);
	} else if (strcmp(line, "AT+CSQ") == 0) {
		atomic_inc(&csq_count);
		dce_send(dce_dlci1_pipe, "+CSQ: 20,99\r\nOK\r\n");
	} else if (strcmp(line, "AT+CEREG?") == 0 && atomic_get(&emu_registered)) {
		/* <n>,<stat> with stat 1: registered, home LTE network. The driver
		 * picks this up through its unsolicited +CEREG match and raises
		 * MODEM_CELLULAR_EVENT_REGISTERED.
		 */
		dce_send(dce_dlci1_pipe, "+CEREG: 1,1\r\nOK\r\n");
	} else {
		/* AT+CREG?, AT+CEREG?, AT+CGREG?, AT+QENG="servingcell" */
		dce_send(dce_dlci1_pipe, "OK\r\n");
	}
}

/* Answer the dial script the driver runs on DLCI2. */
static void dlci2_respond(const char *line)
{
	if (strncmp(line, "ATD", 3) == 0) {
		atomic_inc(&atd_count);
		if (atomic_get(&emu_hold_connect)) {
			/* Leave the dial script waiting for CONNECT, so a hang-up
			 * can catch it in flight.
			 */
			return;
		}
		dce_send(dce_dlci2_pipe, "CONNECT\r\n");
	} else if (strncmp(line, "AT", 2) == 0) {
		/* AT+CGACT=0,1 (allow OK/ERROR), AT+CFUN=1, ... */
		dce_send(dce_dlci2_pipe, "OK\r\n");
	}
	/* Anything else on DLCI2 (PPP HDLC frames after CONNECT) is ignored. */
}

/* Assemble '\r'-delimited lines, dropping '\n', and dispatch complete lines.
 *
 * Every channel that reaches here carries AT commands only, so nothing but a
 * line starting with "AT" is worth accumulating. That guard matters on DLCI2:
 * once the data call is up the channel also carries PPP HDLC frames, and without
 * it their bytes would sit in the buffer and prefix the next command, hiding it
 * from the responder.
 */
static void feed_lines(const uint8_t *buf, size_t size, uint8_t *line, size_t *len,
			size_t cap, void (*respond)(const char *cmd))
{
	for (size_t i = 0; i < size; i++) {
		uint8_t c = buf[i];

		if (c == '\n') {
			continue;
		}
		if (c == '\r') {
			line[*len] = '\0';
			respond((const char *)line);
			*len = 0;
			continue;
		}
		if (*len == 1 && c != 'T') {
			/* Not "AT" after all; the byte may still start a command. */
			*len = 0;
		}
		if (*len == 0 && c != 'A') {
			continue;
		}
		if (*len < cap - 1) {
			line[(*len)++] = c;
		} else {
			/* Overflow: drop and resync. */
			*len = 0;
		}
	}
}

/* ------------------------------------------------------------------------- */
/* Minimal PPP peer: answer the LCP terminate handshake                      */
/* ------------------------------------------------------------------------- */

/* Once the data call is up the driver hands DLCI2 to PPP, and net_if_down()
 * blocks in the PPP L2's ppp_lcp_close() until the peer acknowledges the LCP
 * Terminate-Request. Unanswered it returns -EAGAIN, leaving the interface
 * admin-up so no NET_EVENT_IF_ADMIN_DOWN is emitted and the driver never hangs
 * up. Answering the terminate handshake is all these tests need from a PPP peer:
 * LCP is otherwise left unnegotiated, so no user traffic ever flows.
 */
#define PPP_FLAG              0x7E
#define PPP_ESCAPE            0x7D
#define PPP_ESCAPE_MASK       0x20
#define PPP_ADDRESS           0xFF
#define PPP_CONTROL           0x03
#define PPP_PROTO_LCP         0xC021U
#define PPP_LCP_TERMINATE_REQ 5
#define PPP_LCP_TERMINATE_ACK 6
/* Address + control + protocol + the 4-byte LCP header. */
#define PPP_LCP_MIN_LEN       6

static uint8_t ppp_frame[256];
static size_t ppp_frame_len;
static bool ppp_frame_escaped;

static void dce_ppp_send(const uint8_t *payload, size_t len)
{
	uint8_t out[64];
	size_t pos = 0;

	out[pos++] = PPP_FLAG;
	out[pos++] = PPP_ADDRESS;
	out[pos++] = PPP_ESCAPE;
	out[pos++] = PPP_CONTROL ^ PPP_ESCAPE_MASK;

	for (size_t i = 0; i < len; i++) {
		if (payload[i] == PPP_FLAG || payload[i] == PPP_ESCAPE || payload[i] < 0x20) {
			out[pos++] = PPP_ESCAPE;
			out[pos++] = payload[i] ^ PPP_ESCAPE_MASK;
		} else {
			out[pos++] = payload[i];
		}
	}

	/* modem_ppp strips the frame tail without checking it, so any two bytes
	 * that need no escaping will do for the FCS.
	 */
	out[pos++] = 0xAA;
	out[pos++] = 0xBB;
	out[pos++] = PPP_FLAG;

	__ASSERT_NO_MSG(pos <= sizeof(out));
	modem_pipe_transmit(dce_dlci2_pipe, out, pos);
}

/* Handle one deframed frame: [FF 03] <protocol:2> <code> <id> <length:2> ... */
static void dce_ppp_on_frame(const uint8_t *frame, size_t len)
{
	uint8_t ack[] = {PPP_PROTO_LCP >> 8, PPP_PROTO_LCP & 0xFF, PPP_LCP_TERMINATE_ACK, 0x00,
			 0x00, 0x04};
	size_t hdr = 0;

	/* Address and control are absent once ACFC is negotiated. */
	if (len >= 2 && frame[0] == PPP_ADDRESS && frame[1] == PPP_CONTROL) {
		hdr = 2;
	}
	if ((len - hdr) < PPP_LCP_MIN_LEN - 2) {
		return;
	}
	if (sys_get_be16(&frame[hdr]) != PPP_PROTO_LCP ||
	    frame[hdr + 2] != PPP_LCP_TERMINATE_REQ) {
		return;
	}

	/* Terminate-Ack echoing the request's identifier; the length covers the
	 * 4-byte LCP header only.
	 */
	ack[3] = frame[hdr + 3];
	dce_ppp_send(ack, sizeof(ack));
}

static void dce_ppp_feed(const uint8_t *buf, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		uint8_t c = buf[i];

		if (c == PPP_FLAG) {
			if (ppp_frame_len > 0) {
				dce_ppp_on_frame(ppp_frame, ppp_frame_len);
			}
			ppp_frame_len = 0;
			ppp_frame_escaped = false;
			continue;
		}
		if (c == PPP_ESCAPE) {
			ppp_frame_escaped = true;
			continue;
		}
		if (ppp_frame_escaped) {
			c ^= PPP_ESCAPE_MASK;
			ppp_frame_escaped = false;
		}
		if (ppp_frame_len < sizeof(ppp_frame)) {
			ppp_frame[ppp_frame_len++] = c;
		}
	}
}

/* Drain everything the driver transmitted and route it by phase. */
static void emu_pump(void)
{
	uint8_t buf[128];
	uint32_t n;

	k_mutex_lock(&pump_lock, K_FOREVER);
	while ((n = uart_emul_get_tx_data(modem_uart, buf, sizeof(buf))) > 0) {
		if (emu_phase == EMU_PHASE_AT) {
			feed_lines(buf, n, at_line, &at_line_len, sizeof(at_line),
				   at_phase_respond);
		} else {
			modem_pipe_transmit(wire_pipe, buf, n);
		}
	}
	k_mutex_unlock(&pump_lock);
}

static void tx_ready_cb(const struct device *dev, size_t size, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(size);
	ARG_UNUSED(user_data);
	emu_pump();
}

/* DCE bus -> emulated UART: frames the DCE CMUX produced go back to the driver. */
static void wire_pipe_cb(struct modem_pipe *pipe, enum modem_pipe_event event, void *user_data)
{
	uint8_t buf[128];
	int n;

	ARG_UNUSED(user_data);

	if (event != MODEM_PIPE_EVENT_RECEIVE_READY) {
		return;
	}
	while ((n = modem_pipe_receive(pipe, buf, sizeof(buf))) > 0) {
		uart_emul_put_rx_data(modem_uart, buf, n);
	}
}

static void dce_dlci1_cb(struct modem_pipe *pipe, enum modem_pipe_event event, void *user_data)
{
	uint8_t buf[128];
	int n;

	ARG_UNUSED(user_data);

	if (event != MODEM_PIPE_EVENT_RECEIVE_READY) {
		return;
	}
	while ((n = modem_pipe_receive(pipe, buf, sizeof(buf))) > 0) {
		feed_lines(buf, n, dlci1_line, &dlci1_line_len, sizeof(dlci1_line),
			   dlci1_respond);
	}
}

static void dce_dlci2_cb(struct modem_pipe *pipe, enum modem_pipe_event event, void *user_data)
{
	uint8_t buf[128];
	int n;

	ARG_UNUSED(user_data);

	if (event != MODEM_PIPE_EVENT_RECEIVE_READY) {
		return;
	}
	/* DLCI2 carries the dial script's AT dialogue before CONNECT and PPP
	 * frames after it. Both readers see every byte: neither can be fooled by
	 * the other's traffic, so there is no mode to switch.
	 */
	while ((n = modem_pipe_receive(pipe, buf, sizeof(buf))) > 0) {
		feed_lines(buf, n, dlci2_line, &dlci2_line_len, sizeof(dlci2_line),
			   dlci2_respond);
		dce_ppp_feed(buf, n);
	}
}

/* Bring up the CMUX DCE peer on the bus mock. Also used to rebuild it for a
 * second power-up.
 */
static void dce_cmux_attach_peer(void)
{
	struct modem_cmux_dlci_config dlci1_config = {
		.dlci_address = 1,
		.receive_buf = dce_dlci1_rx,
		.receive_buf_size = sizeof(dce_dlci1_rx),
	};
	struct modem_cmux_dlci_config dlci2_config = {
		.dlci_address = 2,
		.receive_buf = dce_dlci2_rx,
		.receive_buf_size = sizeof(dce_dlci2_rx),
	};
	struct modem_cmux_config cmux_config = {
		.receive_buf = dce_cmux_rx,
		.receive_buf_size = sizeof(dce_cmux_rx),
		.transmit_buf = dce_cmux_tx,
		.transmit_buf_size = sizeof(dce_cmux_tx),
	};

	modem_cmux_init(&dce_cmux, &cmux_config);
	dce_dlci1_pipe = modem_cmux_dlci_init(&dce_cmux, &dce_dlci1, &dlci1_config);
	dce_dlci2_pipe = modem_cmux_dlci_init(&dce_cmux, &dce_dlci2, &dlci2_config);

	/* The DCE CMUX answers the driver's connect and DLCI opens automatically. */
	zassert_ok(modem_cmux_attach(&dce_cmux, dce_bus_pipe), "failed to attach the DCE CMUX");
	modem_pipe_attach(dce_dlci1_pipe, dce_dlci1_cb, NULL);
	modem_pipe_attach(dce_dlci2_pipe, dce_dlci2_cb, NULL);
}

static void emulator_init(void)
{
	const struct modem_backend_mock_config wire_config = {
		.rx_buf = wire_rx,
		.rx_buf_size = sizeof(wire_rx),
		.tx_buf = wire_tx,
		.tx_buf_size = sizeof(wire_tx),
		.limit = 256,
	};
	const struct modem_backend_mock_config dce_bus_config = {
		.rx_buf = dce_bus_rx,
		.rx_buf_size = sizeof(dce_bus_rx),
		.tx_buf = dce_bus_tx,
		.tx_buf_size = sizeof(dce_bus_tx),
		.limit = 256,
	};

	wire_pipe = modem_backend_mock_init(&wire_mock, &wire_config);
	dce_bus_pipe = modem_backend_mock_init(&dce_bus_mock, &dce_bus_config);
	zassert_ok(modem_pipe_open(wire_pipe, K_SECONDS(1)));
	zassert_ok(modem_pipe_open(dce_bus_pipe, K_SECONDS(1)));
	modem_backend_mock_bridge(&wire_mock, &dce_bus_mock);
	modem_pipe_attach(wire_pipe, wire_pipe_cb, NULL);

	dce_cmux_attach_peer();

	emu_phase = EMU_PHASE_AT;
	uart_emul_callback_tx_data_ready_set(modem_uart, tx_ready_cb, NULL);
}

/* ------------------------------------------------------------------------- */
/* Test helpers                                                              */
/* ------------------------------------------------------------------------- */

static struct net_if *ppp_iface;

static enum modem_cellular_state modem_fsm_state(void)
{
	const struct modem_cellular_data *data = modem->data;

	return data->state;
}

/* The driver exposes no state-observer API, so poll instead. native_sim runs on
 * virtual time, so these sleeps cost no wall-clock time.
 */
static bool wait_for_atd(int target, int timeout_ms)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (atomic_get(&atd_count) < target) {
		if (k_uptime_get() > deadline) {
			return false;
		}
		k_msleep(20);
	}
	return true;
}

/* Wait for the data call to be up: the dial script completed and the FSM is
 * resting in one of the two states that hold a live call.
 */
static bool wait_for_call_up(int timeout_ms)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (modem_fsm_state() != MODEM_CELLULAR_STATE_AWAIT_REGISTERED &&
	       modem_fsm_state() != MODEM_CELLULAR_STATE_REGISTERED) {
		if (k_uptime_get() > deadline) {
			return false;
		}
		k_msleep(20);
	}
	return true;
}

static void *common_setup(void)
{
	k_mutex_init(&pump_lock);
	k_event_init(&emu_events);

	zassert_true(device_is_ready(modem), "modem device not ready");
	zassert_true(device_is_ready(modem_uart), "emulated UART not ready");

	ppp_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
	zassert_not_null(ppp_iface, "no PPP interface found");

	return NULL;
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */

#if !defined(CONFIG_MODEM_CELLULAR_ON_DEMAND_CONNECT)

/* Guards the documented default: with the option off, power-up dials the data
 * call unprompted, and no consumer ever admits the PPP interface.
 */
ZTEST(cellular_on_demand_disabled, test_dials_unprompted)
{
	zassert_true(wait_for_atd(1, 20000), "modem did not dial as part of power-up");
	zassert_false(net_if_is_admin_up(ppp_iface),
		      "the unprompted dial must not depend on the PPP interface admin state");
	zassert_true(wait_for_call_up(10000), "dial script did not complete");
}

static void *disabled_suite_setup(void)
{
	common_setup();
	emulator_init();
	/* The FSM already started at boot; process whatever it has sent so far. */
	emu_pump();

	return NULL;
}

ZTEST_SUITE(cellular_on_demand_disabled, NULL, disabled_suite_setup, NULL, NULL, NULL);

#elif defined(CONFIG_TEST_ON_DEMAND_EARLY_ADMIN_UP)

/* The ADMIN_UP edge is delivered while the FSM is still powering up, where DIAL
 * is dropped on the floor. Reaching AWAIT_DIAL must dial anyway, because that
 * state re-derives the admin level on entry.
 */
ZTEST(cellular_on_demand_early_admin_up, test_early_admin_up_still_dials)
{
	zassert_true(wait_for_atd(1, 20000), "an ADMIN_UP edge from before the boot was lost");
	zassert_true(wait_for_call_up(10000), "dial script did not complete");
}

static void *early_suite_setup(void)
{
	common_setup();

	/* The driver auto-boots at device init but stalls on the first init-script
	 * command, because nothing answers the emulated UART until the emulator is
	 * attached below. Admitting the interface now therefore lands the event
	 * well before the FSM can act on it.
	 */
	zassert_not_equal(MODEM_CELLULAR_STATE_AWAIT_DIAL, modem_fsm_state(),
			  "modem booted before the interface was admitted");
	zassert_ok(net_if_up(ppp_iface), "net_if_up failed");

	emulator_init();
	emu_pump();

	return NULL;
}

ZTEST_SUITE(cellular_on_demand_early_admin_up, NULL, early_suite_setup, NULL, NULL, NULL);

#else /* on-demand connect, default scenario */

static bool wait_for_state(enum modem_cellular_state want, int timeout_ms)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (modem_fsm_state() != want) {
		if (k_uptime_get() > deadline) {
			return false;
		}
		k_msleep(20);
	}
	return true;
}

/* Wait until the periodic script has polled AT+CSQ at least `target` times. */
static bool wait_for_csq(int target, int timeout_ms)
{
	int64_t deadline = k_uptime_get() + timeout_ms;

	while (atomic_get(&csq_count) < target) {
		if (k_uptime_get() > deadline) {
			return false;
		}
		k_msleep(20);
	}
	return true;
}

/* Drop the PPP interface and wait for the FSM to come to rest in AWAIT_DIAL, so
 * a test starts from a known place regardless of what ran before it.
 */
static void park_in_await_dial(void)
{
	if (net_if_is_admin_up(ppp_iface)) {
		zassert_ok(net_if_down(ppp_iface), "net_if_down failed");
	}
	zassert_true(wait_for_state(MODEM_CELLULAR_STATE_AWAIT_DIAL, 15000),
		     "FSM did not park in AWAIT_DIAL, state is %d", modem_fsm_state());
}

static void admit_iface(void)
{
	if (!net_if_is_admin_up(ppp_iface)) {
		zassert_ok(net_if_up(ppp_iface), "net_if_up failed");
	}
}

/* Rewind the emulator for a second power-up: back to the raw AT phase with a
 * freshly built DCE CMUX peer. Only safe while the driver's FSM is quiescent,
 * i.e. suspended in IDLE, since nothing is in flight then.
 */
static void emulator_rewind(void)
{
	modem_cmux_release(&dce_cmux);
	modem_backend_mock_reset(&wire_mock);
	modem_backend_mock_reset(&dce_bus_mock);

	k_mutex_lock(&pump_lock, K_FOREVER);
	at_line_len = 0;
	dlci1_line_len = 0;
	dlci2_line_len = 0;
	ppp_frame_len = 0;
	ppp_frame_escaped = false;
	emu_phase = EMU_PHASE_AT;
	k_mutex_unlock(&pump_lock);

	dce_cmux_attach_peer();
}

ZTEST(cellular_on_demand_connect, test_01_boot_parks_without_dialling)
{
	/* The driver auto-boots on power-up. Wait until it has walked init + CMUX
	 * + APN, which lands it in AWAIT_DIAL.
	 */
	zassert_true(k_event_wait(&emu_events, EV_APN_DONE, false, K_SECONDS(10)),
		     "modem did not boot through the APN script");
	zassert_true(wait_for_state(MODEM_CELLULAR_STATE_AWAIT_DIAL, 5000),
		     "modem did not park in AWAIT_DIAL after power-up, state is %d",
		     modem_fsm_state());
	zassert_false(net_if_is_admin_up(ppp_iface),
		      "PPP interface should be admin-down at rest");

	/* Parked means parked: it does not wander off or dial on its own. */
	k_msleep(3 * CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS);
	zassert_equal(MODEM_CELLULAR_STATE_AWAIT_DIAL, modem_fsm_state(),
		      "modem left AWAIT_DIAL without net_if_up(), state is %d",
		      modem_fsm_state());
	zassert_equal(0, atomic_get(&atd_count), "modem dialled without net_if_up()");
}

ZTEST(cellular_on_demand_connect, test_02_admin_up_dials)
{
	int dials;

	park_in_await_dial();
	dials = atomic_get(&atd_count);

	zassert_ok(net_if_up(ppp_iface), "net_if_up failed");
	zassert_true(wait_for_atd(dials + 1, 5000), "net_if_up() did not trigger a dial");
	zassert_true(wait_for_call_up(10000), "dial script did not complete, state is %d",
		     modem_fsm_state());
}

ZTEST(cellular_on_demand_connect, test_03_hangup_from_registered_parks)
{
	int dials;

	/* Report registration on the next periodic poll, so the FSM advances from
	 * AWAIT_REGISTERED to REGISTERED: the state a live data call rests in, and
	 * the one a consumer disconnects from in practice.
	 */
	admit_iface();
	atomic_set(&emu_registered, 1);
	zassert_true(wait_for_state(MODEM_CELLULAR_STATE_REGISTERED, 20000),
		     "modem did not reach REGISTERED, state is %d", modem_fsm_state());
	dials = atomic_get(&atd_count);

	/* Hanging up from REGISTERED must tear the call down and park again. */
	zassert_ok(net_if_down(ppp_iface), "net_if_down failed");
	zassert_true(wait_for_state(MODEM_CELLULAR_STATE_AWAIT_DIAL, 15000),
		     "hang-up from REGISTERED did not park in AWAIT_DIAL, state is %d",
		     modem_fsm_state());
	zassert_equal(dials, atomic_get(&atd_count), "modem re-dialled while admin-down");
}

ZTEST(cellular_on_demand_connect, test_04_redial_from_park)
{
	int dials;

	park_in_await_dial();
	dials = atomic_get(&atd_count);

	zassert_ok(net_if_up(ppp_iface), "net_if_up failed");
	zassert_true(wait_for_atd(dials + 1, 5000), "re-connect from AWAIT_DIAL did not re-dial");
	zassert_true(wait_for_call_up(10000), "dial script did not complete, state is %d",
		     modem_fsm_state());
}

ZTEST(cellular_on_demand_connect, test_05_interface_bounce_redials)
{
	int dials;

	/* Bring a call up, then bounce the interface without giving the FSM time
	 * to reach AWAIT_DIAL: the DIAL event lands in AWAIT_PPP_DEAD, which
	 * ignores it, so the re-dial can only come from the admin level being
	 * re-derived afterwards.
	 */
	admit_iface();
	zassert_true(wait_for_call_up(20000), "no call to bounce, state is %d", modem_fsm_state());
	dials = atomic_get(&atd_count);

	zassert_ok(net_if_down(ppp_iface), "net_if_down failed");
	zassert_ok(net_if_up(ppp_iface), "net_if_up failed");
	zassert_not_equal(MODEM_CELLULAR_STATE_AWAIT_DIAL, modem_fsm_state(),
			  "the bounce was not fast enough to skip AWAIT_DIAL");

	zassert_true(wait_for_atd(dials + 1, 15000), "interface bounce did not re-dial");
	zassert_true(wait_for_call_up(10000), "dial script did not complete, state is %d",
		     modem_fsm_state());
}

ZTEST(cellular_on_demand_connect, test_06_hangup_during_dial_parks)
{
	int dials;

	park_in_await_dial();
	dials = atomic_get(&atd_count);

	/* Withhold CONNECT so the dial script is still in flight when we hang up. */
	atomic_set(&emu_hold_connect, 1);
	zassert_ok(net_if_up(ppp_iface), "net_if_up failed");
	zassert_true(wait_for_atd(dials + 1, 5000), "net_if_up() did not trigger a dial");
	zassert_equal(MODEM_CELLULAR_STATE_RUN_DIAL_SCRIPT, modem_fsm_state(),
		      "dial script should still be waiting for CONNECT, state is %d",
		      modem_fsm_state());

	zassert_ok(net_if_down(ppp_iface), "net_if_down failed");
	zassert_true(wait_for_state(MODEM_CELLULAR_STATE_AWAIT_DIAL, 5000),
		     "hang-up during the dial script did not park in AWAIT_DIAL, state is %d",
		     modem_fsm_state());

	/* The aborted script left the FSM usable: the next connect dials through. */
	atomic_set(&emu_hold_connect, 0);
	dials = atomic_get(&atd_count);
	zassert_ok(net_if_up(ppp_iface), "net_if_up failed");
	zassert_true(wait_for_atd(dials + 1, 5000), "modem did not re-dial after an aborted dial");
	zassert_true(wait_for_call_up(10000), "dial script did not complete, state is %d",
		     modem_fsm_state());
}

ZTEST(cellular_on_demand_connect, test_07_suspend_resume_stays_parked)
{
	int dials;

	park_in_await_dial();
	dials = atomic_get(&atd_count);

	/* Suspending from AWAIT_DIAL must power the modem down. pm_device_action_run()
	 * only returns once the FSM has reached IDLE.
	 */
	zassert_ok(pm_device_action_run(modem, PM_DEVICE_ACTION_SUSPEND), "suspend failed");
	zassert_equal(MODEM_CELLULAR_STATE_IDLE, modem_fsm_state(),
		      "modem did not reach IDLE on suspend, state is %d", modem_fsm_state());

	emulator_rewind();

	/* Resuming re-runs the whole power-up, which must park again rather than
	 * dial: the interface is still admin-down.
	 */
	zassert_ok(pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME), "resume failed");
	zassert_true(wait_for_state(MODEM_CELLULAR_STATE_AWAIT_DIAL, 30000),
		     "modem did not park in AWAIT_DIAL after resume, state is %d",
		     modem_fsm_state());
	zassert_equal(dials, atomic_get(&atd_count), "modem dialled on resume without net_if_up()");
}

/* Parked in AWAIT_DIAL the periodic script must keep running, so signal and the
 * serving/neighbour caches stay fresh for an app deciding whether to dial, and
 * pause()/resume() must behave there. The resume kick used to land on
 * default: break in AWAIT_DIAL, silently killing the periodic script for good
 * while still reporting success.
 */
ZTEST(cellular_on_demand_connect, test_08_periodic_survives_pause_resume_while_parked)
{
	int dials;
	int csq;

	park_in_await_dial();
	dials = atomic_get(&atd_count);

	csq = atomic_get(&csq_count);
	zassert_true(wait_for_csq(csq + 1, 4 * CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS),
		     "periodic script did not run while parked in AWAIT_DIAL");

	zassert_ok(cellular_modem_pause_periodic_script(modem), "pause failed");
	zassert_ok(cellular_modem_resume_periodic_script(modem), "resume failed");

	csq = atomic_get(&csq_count);
	zassert_true(wait_for_csq(csq + 1, 4 * CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_MS),
		     "periodic script did not resume after pause/resume while parked");

	/* None of this may have dialled or left AWAIT_DIAL: the interface is still
	 * admin-down.
	 */
	zassert_equal(dials, atomic_get(&atd_count),
		      "periodic activity dialled without net_if_up()");
	zassert_equal(MODEM_CELLULAR_STATE_AWAIT_DIAL, modem_fsm_state(),
		      "periodic activity moved the FSM out of AWAIT_DIAL, state is %d",
		      modem_fsm_state());
}

static void *suite_setup(void)
{
	common_setup();
	emulator_init();
	/* The FSM already started at boot; process whatever it has sent so far. */
	emu_pump();

	return NULL;
}

ZTEST_SUITE(cellular_on_demand_connect, NULL, suite_setup, NULL, NULL, NULL);

#endif /* scenario selection */
