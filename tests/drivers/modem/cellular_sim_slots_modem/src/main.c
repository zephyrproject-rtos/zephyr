/*
 * Copyright (c) 2026 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SIM slot selection and switch of the modem_cellular driver against an
 * emulated modem. The tests play the modem (DCE) side over an emulated UART:
 * the raw init AT script first, then a modem_cmux peer bridged through the
 * modem backend mock once the driver switches to CMUX, as in tests/drivers/
 * modem/cellular_on_demand_connect.
 *
 * Two scenarios share this file:
 *
 *   - a SIMCom A76xx with two SIM interfaces, which it selects with
 *     AT*SELECTSIMSLOT and keeps across power cycles, as in non-volatile memory.
 *     No APN is configured, so the driver parks in WAIT_FOR_APN until a test
 *     sets one;
 *   - a Quectel BG95 whose single SIM interface is switched between two SIM
 *     slots by a gpio-mux, with an APN, so the driver dials at boot.
 *
 * The emulated modem logs every command it receives in CMUX mode, and the
 * level of the SIM switch GPIO when it deactivates and activates the SIM
 * interface. The tests run in order, each one from the state the previous one
 * left.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>

#include <modem_backend_mock.h>

#define SIM_BY_MUX DT_NODE_EXISTS(DT_NODELABEL(sim_mux))

#if SIM_BY_MUX
#define SIM_MUX_GPIO_NODE DT_GPIO_CTLR(DT_NODELABEL(sim_mux), mux_gpios)
#define SIM_MUX_GPIO_PIN  DT_GPIO_PIN(DT_NODELABEL(sim_mux), mux_gpios)
#define DEACTIVATE_CMD    "AT+CFUN=0"
#define ACTIVATE_CMD      "AT+CFUN=4"
#else
#define DEACTIVATE_CMD "AT+CFUN=4"
#endif

#define SELECT_CMD "AT*SELECTSIMSLOT="
#define QUERY_CMD  "AT*SELECTSIMSLOT?"
#define DETACH_CMD "AT+CGATT=0"

static const struct device *const modem = DEVICE_DT_GET(DT_NODELABEL(modem));
static const struct device *const modem_uart = DEVICE_DT_GET(DT_NODELABEL(modem_uart));

/* SIM interface the A76xx has selected, kept across power cycles */
static atomic_t emu_slot;
static atomic_t emu_queries;
static atomic_t emu_writes;
static char emu_last_write[32];
static char emu_last_apn[64];
static atomic_t emu_cmux_connects;
static atomic_t emu_dials;

#if SIM_BY_MUX
/* SIM switch GPIO level when the SIM interface was deactivated and activated */
static atomic_t emu_mux_at_deactivate;
static atomic_t emu_mux_at_activate;

/* As on a BG95: once reactivated, the SIM is ready some time after AT+CFUN=4 answers,
 * which +CPIN: READY reports, and AT+CPIN?, AT+CIMI and AT+QCCID answer ERROR until
 * then. A SIM that was not deactivated stays ready, without a new +CPIN: READY.
 */
static atomic_t emu_sim_not_ready;
static struct k_work_delayable emu_cpin_work;
#endif

/* Commands received in CMUX mode, one per line */
static char emu_log[2048];
static size_t emu_log_len;
static struct k_spinlock emu_log_lock;

/* Prefix of the command the emulated modem refuses, empty for none */
static char emu_fail_prefix[32];

/* ------------------------------------------------------------------------- */
/* Modem (DCE) emulator                                                      */
/* ------------------------------------------------------------------------- */

enum emu_phase {
	EMU_PHASE_AT,   /* raw init AT script, straight over the emulated UART */
	EMU_PHASE_CMUX, /* driver switched to CMUX; route through the DCE peer */
};

static enum emu_phase emu_phase;
static struct k_mutex pump_lock;

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

static uint8_t at_line[256];
static size_t at_line_len;
static uint8_t dlci1_line[256];
static size_t dlci1_line_len;
static uint8_t dlci2_line[256];
static size_t dlci2_line_len;

/* SIM slot the modem is routed to */
static int emu_current_slot(void)
{
#if SIM_BY_MUX
	return gpio_emul_output_get(DEVICE_DT_GET(SIM_MUX_GPIO_NODE), SIM_MUX_GPIO_PIN);
#else
	return (int)atomic_get(&emu_slot);
#endif
}

static void emu_log_add(const char *line)
{
	k_spinlock_key_t key = k_spin_lock(&emu_log_lock);
	size_t len = strlen(line);

	if (emu_log_len + len + 2 < sizeof(emu_log)) {
		memcpy(&emu_log[emu_log_len], line, len);
		emu_log_len += len;
		emu_log[emu_log_len++] = '\n';
		emu_log[emu_log_len] = '\0';
	}

	k_spin_unlock(&emu_log_lock, key);
}

static void emu_log_reset(void)
{
	k_spinlock_key_t key = k_spin_lock(&emu_log_lock);

	emu_log_len = 0;
	emu_log[0] = '\0';
	k_spin_unlock(&emu_log_lock, key);
}

/* Number of times a command was received */
static __maybe_unused int emu_log_count(const char *cmd)
{
	char needle[48];
	const char *p = emu_log;
	int count = 0;

	snprintf(needle, sizeof(needle), "%s\n", cmd);
	while ((p = strstr(p, needle)) != NULL) {
		count++;
		p += strlen(needle);
	}

	return count;
}

/* Position of a command in the log, -1 if it was not received */
static int emu_log_pos(const char *cmd)
{
	char needle[48];
	const char *p;

	snprintf(needle, sizeof(needle), "%s\n", cmd);
	p = strstr(emu_log, needle);

	return (p == NULL) ? -1 : (int)(p - emu_log);
}

static bool emu_fails(const char *line)
{
	return (emu_fail_prefix[0] != '\0') &&
	       (strncmp(line, emu_fail_prefix, strlen(emu_fail_prefix)) == 0);
}

static void to_driver(const char *resp)
{
	uart_emul_put_rx_data(modem_uart, (const uint8_t *)resp, strlen(resp));
}

static void dce_send(struct modem_pipe *pipe, const char *resp)
{
	modem_pipe_transmit(pipe, (const uint8_t *)resp, strlen(resp));
}

/* Answer the identity queries, the same on the raw UART and in CMUX mode */
static bool identity_respond(const char *line, char *resp, size_t size)
{
	if (strcmp(line, "AT+CGSN") == 0) {
		snprintf(resp, size, "359123456789012\r\nOK\r\n");
	} else if (strcmp(line, "AT+CGMM") == 0) {
		snprintf(resp, size, "MODEM\r\nOK\r\n");
	} else if (strcmp(line, "AT+CIMI") == 0) {
		snprintf(resp, size, "72400000000000%d\r\nOK\r\n", emu_current_slot());
	} else if (strcmp(line, "AT+QCCID") == 0) {
		snprintf(resp, size, "+QCCID: 895500000000000000%d\r\nOK\r\n", emu_current_slot());
	} else {
		return false;
	}

	return true;
}

/* Answer one raw init script line, or switch to CMUX */
static void at_phase_respond(const char *line)
{
	char resp[64];

	if (strncmp(line, "AT+CMUX", 7) == 0) {
		/* No reply expected: the driver brings up CMUX */
		atomic_inc(&emu_cmux_connects);
		emu_phase = EMU_PHASE_CMUX;
	} else if (identity_respond(line, resp, sizeof(resp))) {
		to_driver(resp);
	} else if (line[0] != '\0') {
		to_driver("OK\r\n");
	}
}

/* Answer the commands on DLCI1: SIM slot, SIM interface, APN and registration */
static void dlci1_respond(const char *line)
{
	char resp[64];

	emu_log_add(line);

	if (emu_fails(line)) {
		dce_send(dce_dlci1_pipe, "ERROR\r\n");
		return;
	}

#if SIM_BY_MUX
	if (atomic_get(&emu_sim_not_ready) &&
	    ((strcmp(line, "AT+CPIN?") == 0) || (strcmp(line, "AT+CIMI") == 0) ||
	     (strcmp(line, "AT+QCCID") == 0))) {
		/* SIM busy */
		dce_send(dce_dlci1_pipe, "+CME ERROR: 14\r\n");
		return;
	}
#endif

	if (strcmp(line, QUERY_CMD) == 0) {
		atomic_inc(&emu_queries);
		snprintf(resp, sizeof(resp), "*SELECTUSIM: %d\r\nOK\r\n",
			 (int)atomic_get(&emu_slot));
		dce_send(dce_dlci1_pipe, resp);
	} else if (strncmp(line, SELECT_CMD, strlen(SELECT_CMD)) == 0) {
		atomic_inc(&emu_writes);
		strncpy(emu_last_write, line, sizeof(emu_last_write) - 1);
		atomic_set(&emu_slot, atoi(line + strlen(SELECT_CMD)));
		dce_send(dce_dlci1_pipe, "OK\r\n");
	} else if (strncmp(line, "AT+CGDCONT", 10) == 0) {
		strncpy(emu_last_apn, line, sizeof(emu_last_apn) - 1);
		dce_send(dce_dlci1_pipe, "OK\r\n");
	} else if (strcmp(line, "AT+CPIN?") == 0) {
		dce_send(dce_dlci1_pipe, "+CPIN: READY\r\nOK\r\n");
	} else if (strcmp(line, "AT+CEREG?") == 0) {
		/* Registered on the home network */
		dce_send(dce_dlci1_pipe, "+CEREG: 1,1\r\nOK\r\n");
	} else if (identity_respond(line, resp, sizeof(resp))) {
		dce_send(dce_dlci1_pipe, resp);
	} else {
#if SIM_BY_MUX
		if (strcmp(line, DEACTIVATE_CMD) == 0) {
			atomic_set(&emu_mux_at_deactivate, emu_current_slot());
			atomic_set(&emu_sim_not_ready, 1);
		} else if ((strcmp(line, ACTIVATE_CMD) == 0) && atomic_get(&emu_sim_not_ready)) {
			atomic_set(&emu_mux_at_activate, emu_current_slot());
			k_work_schedule(&emu_cpin_work, K_MSEC(300));
		}
#endif
		dce_send(dce_dlci1_pipe, "OK\r\n");
	}
}

#if SIM_BY_MUX
static void emu_cpin_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	atomic_set(&emu_sim_not_ready, 0);
	dce_send(dce_dlci1_pipe, "+CPIN: READY\r\n");
}
#endif

/* Answer the dial and shutdown scripts on DLCI2 */
static void dlci2_respond(const char *line)
{
	emu_log_add(line);

	if (strncmp(line, "ATD", 3) == 0) {
		atomic_inc(&emu_dials);
		dce_send(dce_dlci2_pipe, "CONNECT\r\n");
	} else if (strcmp(line, "AT+QPOWD=1") == 0) {
		dce_send(dce_dlci2_pipe, "OK\r\n\r\nPOWERED DOWN\r\n");
	} else {
		dce_send(dce_dlci2_pipe, "OK\r\n");
	}
}

/* Assemble '\r'-delimited lines starting with "AT", dropping '\n'. Lines not
 * starting with "AT" are dropped, which keeps the PPP frames on DLCI2 out.
 */
static void feed_lines(const uint8_t *buf, size_t size, uint8_t *line, size_t *len, size_t cap,
		       void (*respond)(const char *cmd))
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

		if ((*len == 1) && (c != 'T')) {
			*len = 0;
		}

		if ((*len == 0) && (c != 'A')) {
			continue;
		}

		if (*len < cap - 1) {
			line[(*len)++] = c;
		} else {
			*len = 0;
		}
	}
}

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

/* DCE bus -> emulated UART: frames of the DCE CMUX go back to the driver */
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
		feed_lines(buf, n, dlci1_line, &dlci1_line_len, sizeof(dlci1_line), dlci1_respond);
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

	while ((n = modem_pipe_receive(pipe, buf, sizeof(buf))) > 0) {
		feed_lines(buf, n, dlci2_line, &dlci2_line_len, sizeof(dlci2_line), dlci2_respond);
	}
}

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

	zassert_ok(modem_cmux_attach(&dce_cmux, dce_bus_pipe), "Failed to attach the DCE CMUX");
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

/* Rewind the emulator for the next power-up; only while the driver is idle */
static __maybe_unused void emulator_rewind(void)
{
	modem_cmux_release(&dce_cmux);
	modem_backend_mock_reset(&wire_mock);
	modem_backend_mock_reset(&dce_bus_mock);

	k_mutex_lock(&pump_lock, K_FOREVER);
	at_line_len = 0;
	dlci1_line_len = 0;
	dlci2_line_len = 0;
	emu_phase = EMU_PHASE_AT;
	k_mutex_unlock(&pump_lock);

	dce_cmux_attach_peer();
}

/* ------------------------------------------------------------------------- */
/* Test helpers                                                              */
/* ------------------------------------------------------------------------- */

static enum modem_cellular_state modem_state(void)
{
	const struct modem_cellular_data *data = modem->data;

	return data->state;
}

static bool modem_switching(void)
{
	const struct modem_cellular_data *data = modem->data;

	return data->sim_switching || (data->sim_slot_requested != NULL);
}

/* native_sim runs on virtual time, so the waits cost no wall-clock time */
static void wait_for_state(enum modem_cellular_state state)
{
	zassert_true(
		WAIT_FOR((modem_state() == state) && !modem_switching(), 60000000, k_msleep(20)),
		"Modem did not reach state %d, state is %d", state, modem_state());
}

static uint8_t active_sim_slot(void)
{
	uint8_t count;
	uint8_t active;

	zassert_ok(cellular_get_sim_slots(modem, &count, &active));
	zassert_equal(count, 2, "Unexpected SIM slot count %u", count);

	return active;
}

/* Request a switch of the running modem, and wait for it to connect again */
static void switch_while_running(uint8_t slot, enum modem_cellular_state state)
{
	zassert_ok(cellular_select_sim(modem, slot), "SIM slot switch refused");
	wait_for_state(state);
}

static void assert_in_order(const char *first, const char *second)
{
	int pos_first = emu_log_pos(first);
	int pos_second = emu_log_pos(second);

	zassert_true(pos_first >= 0, "%s not received", first);
	zassert_true(pos_second >= 0, "%s not received", second);
	zassert_true(pos_first < pos_second, "%s received after %s", first, second);
}

static void *test_setup(void)
{
	k_mutex_init(&pump_lock);
#if SIM_BY_MUX
	k_work_init_delayable(&emu_cpin_work, emu_cpin_handler);
#endif

	zassert_true(device_is_ready(modem), "Modem not ready");
	zassert_true(device_is_ready(modem_uart), "Emulated UART not ready");

	/* The driver started at boot; answer it from now on, from SIM interface 0 */
	atomic_set(&emu_slot, 0);
	emulator_init();
	emu_pump();

	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	emu_fail_prefix[0] = '\0';
	emu_log_reset();
}

#if !SIM_BY_MUX

/* ------------------------------------------------------------------------- */
/* SIMCom A76xx: SIM interfaces of the modem                                 */
/* ------------------------------------------------------------------------- */

static void modem_suspend(void)
{
	zassert_ok(pm_device_action_run(modem, PM_DEVICE_ACTION_SUSPEND), "Suspend failed");
	zassert_equal(modem_state(), MODEM_CELLULAR_STATE_IDLE, "Modem not idle once suspended");
	emulator_rewind();
}

static void modem_resume(void)
{
	zassert_ok(pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME), "Resume failed");
	wait_for_state(MODEM_CELLULAR_STATE_WAIT_FOR_APN);
}

ZTEST(cellular_sim_slots_modem, test_01_boot_selects_default_slot)
{
	wait_for_state(MODEM_CELLULAR_STATE_WAIT_FOR_APN);

	zassert_true(atomic_get(&emu_queries) > 0, "Selected SIM interface not queried");
	zassert_equal(atomic_get(&emu_writes), 1, "Unexpected number of SIM selections");
	zassert_str_equal(emu_last_write, "AT*SELECTSIMSLOT=1,0", "Unexpected select command");
	zassert_equal(atomic_get(&emu_slot), 1, "Modem not on the default-sim slot");
	zassert_equal(active_sim_slot(), 1, "default-sim slot not active");
}

ZTEST(cellular_sim_slots_modem, test_02_selected_slot_not_written_again)
{
	atomic_val_t queries = atomic_get(&emu_queries);
	atomic_val_t writes = atomic_get(&emu_writes);

	modem_suspend();
	modem_resume();

	zassert_true(atomic_get(&emu_queries) > queries, "Selected SIM interface not queried");
	zassert_equal(atomic_get(&emu_writes), writes, "Slot the modem had selected written");
	zassert_equal(active_sim_slot(), 1, "Active SIM slot changed");
}

ZTEST(cellular_sim_slots_modem, test_03_switch_while_running)
{
	atomic_val_t connects = atomic_get(&emu_cmux_connects);

	switch_while_running(0, MODEM_CELLULAR_STATE_WAIT_FOR_APN);

	assert_in_order(DETACH_CMD, DEACTIVATE_CMD);
	assert_in_order(DEACTIVATE_CMD, "AT*SELECTSIMSLOT=0,0");
	zassert_equal(atomic_get(&emu_slot), 0, "Modem not on SIM slot 0");
	zassert_equal(active_sim_slot(), 0, "SIM slot 0 not active");
	zassert_equal(atomic_get(&emu_cmux_connects), connects, "Modem restarted for the switch");
}

ZTEST(cellular_sim_slots_modem, test_04_select_applied_on_power_up)
{
	atomic_val_t writes = atomic_get(&emu_writes);

	modem_suspend();

	zassert_ok(cellular_select_sim(modem, 1));
	zassert_equal(active_sim_slot(), 1, "SIM slot 1 not active");
	zassert_equal(atomic_get(&emu_writes), writes, "Powered off modem received a command");

	modem_resume();

	zassert_equal(atomic_get(&emu_writes), writes + 1, "SIM slot not selected on power-up");
	zassert_str_equal(emu_last_write, "AT*SELECTSIMSLOT=1,0", "Unexpected select command");
	zassert_equal(atomic_get(&emu_slot), 1, "Modem not on SIM slot 1");
}

ZTEST(cellular_sim_slots_modem, test_05_failed_select_keeps_modem_slot)
{
	modem_suspend();
	zassert_ok(cellular_select_sim(modem, 0));

	strcpy(emu_fail_prefix, SELECT_CMD);
	modem_resume();

	zassert_true(emu_log_pos("AT*SELECTSIMSLOT=0,0") >= 0, "SIM slot selection not tried");
	zassert_equal(atomic_get(&emu_slot), 1, "Modem changed SIM interface");
	zassert_equal(active_sim_slot(), 1, "Active SIM slot not the one the modem kept");
}

ZTEST(cellular_sim_slots_modem, test_06_failed_query_selects_anyway)
{
	modem_suspend();
	zassert_ok(cellular_select_sim(modem, 0));

	strcpy(emu_fail_prefix, QUERY_CMD);
	modem_resume();

	zassert_true(emu_log_pos("AT*SELECTSIMSLOT=0,0") >= 0, "SIM slot not selected");
	zassert_equal(atomic_get(&emu_slot), 0, "Modem not on SIM slot 0");
	zassert_equal(active_sim_slot(), 0, "SIM slot 0 not active");
}

ZTEST(cellular_sim_slots_modem, test_07_invalid_query_response_ignored)
{
	struct modem_cellular_data *data = modem->data;
	int16_t saved = data->sim_slot_modem;
	char *argv[2] = {"*SELECTUSIM: ", NULL};

	data->sim_slot_modem = -1;

	argv[1] = "x";
	modem_cellular_chat_on_sim_slot(NULL, argv, ARRAY_SIZE(argv), data);
	zassert_equal(data->sim_slot_modem, -1, "Non numeric slot accepted");

	argv[1] = "256";
	modem_cellular_chat_on_sim_slot(NULL, argv, ARRAY_SIZE(argv), data);
	zassert_equal(data->sim_slot_modem, -1, "Out of range slot accepted");

	argv[1] = "1";
	modem_cellular_chat_on_sim_slot(NULL, argv, ARRAY_SIZE(argv), data);
	zassert_equal(data->sim_slot_modem, 1, "Valid slot not accepted");

	data->sim_slot_modem = saved;
}

ZTEST(cellular_sim_slots_modem, test_08_switch_with_data_call)
{
	atomic_val_t connects;
	atomic_val_t dials;

	/* Set an APN, so that the modem dials */
	zassert_ok(cellular_set_apn(modem, "internet"));
	wait_for_state(MODEM_CELLULAR_STATE_REGISTERED);

	connects = atomic_get(&emu_cmux_connects);
	dials = atomic_get(&emu_dials);
	emu_log_reset();

	/* The new SIM gets its own APN, set along with the switch */
	zassert_ok(cellular_select_sim(modem, 1));
	zassert_ok(cellular_set_apn(modem, "apn.sim1"), "APN refused along with the switch");
	wait_for_state(MODEM_CELLULAR_STATE_REGISTERED);

	assert_in_order(DETACH_CMD, DEACTIVATE_CMD);
	assert_in_order(DEACTIVATE_CMD, "AT*SELECTSIMSLOT=1,0");
	assert_in_order("AT*SELECTSIMSLOT=1,0", "AT+CGDCONT=1,\"IP\",\"apn.sim1\"");
	zassert_equal(atomic_get(&emu_slot), 1, "Modem not on SIM slot 1");
	zassert_equal(active_sim_slot(), 1, "SIM slot 1 not active");
	zassert_equal(atomic_get(&emu_dials), dials + 1, "Data call not dialed again");
	zassert_equal(atomic_get(&emu_cmux_connects), connects, "Modem restarted for the switch");
}

ZTEST(cellular_sim_slots_modem, test_09_failed_deactivate_keeps_slot)
{
	atomic_val_t dials = atomic_get(&emu_dials);

	strcpy(emu_fail_prefix, DEACTIVATE_CMD);
	switch_while_running(0, MODEM_CELLULAR_STATE_REGISTERED);

	zassert_true(emu_log_pos(DEACTIVATE_CMD) >= 0, "SIM interface not deactivated");
	zassert_equal(emu_log_pos("AT*SELECTSIMSLOT=0,0"), -1, "SIM slot selected anyway");
	zassert_equal(atomic_get(&emu_slot), 1, "Modem changed SIM interface");
	zassert_equal(active_sim_slot(), 1, "Active SIM slot not kept");
	zassert_equal(atomic_get(&emu_dials), dials + 1, "Data call not dialed again");
}

ZTEST(cellular_sim_slots_modem, test_10_refused_select_keeps_previous_slot)
{
	/* The modem refuses the query and the selection, without telling its slot */
	strcpy(emu_fail_prefix, "AT*SELECTSIMSLOT");
	switch_while_running(0, MODEM_CELLULAR_STATE_REGISTERED);

	zassert_true(emu_log_pos("AT*SELECTSIMSLOT=0,0") >= 0, "SIM slot selection not tried");
	zassert_equal(atomic_get(&emu_slot), 1, "Modem changed SIM interface");
	zassert_equal(active_sim_slot(), 1, "Active SIM slot not the previous one");

	/* The slot can be requested again */
	emu_fail_prefix[0] = '\0';
	switch_while_running(0, MODEM_CELLULAR_STATE_REGISTERED);
	zassert_equal(atomic_get(&emu_slot), 0, "Modem not on SIM slot 0");
	zassert_equal(active_sim_slot(), 0, "SIM slot 0 not active");
}

ZTEST(cellular_sim_slots_modem, test_11_request_dropped_with_full_event_queue)
{
	struct modem_cellular_data *data = modem->data;
	const struct modem_cellular_sim_slot *requested = NULL;
	const struct modem_cellular_sim_slot *kept;
	uint8_t slot = 1;
	int ret = 0;

	/* Without the state machine running, each request queues an event until the queue
	 * is full
	 */
	k_sched_lock();
	for (int i = 0; (i < 32) && (ret == 0); i++) {
		requested = data->sim_slot_requested;
		ret = cellular_select_sim(modem, slot);
		slot = (slot == 1U) ? 0U : 1U;
	}
	kept = data->sim_slot_requested;
	k_sched_unlock();

	zassert_equal(ret, -EAGAIN, "Request accepted with a full event queue (%d)", ret);
	zassert_equal_ptr(kept, requested, "Request without its event kept");
	zassert_not_null(kept, "No request queued");

	/* The requests that were queued are run */
	wait_for_state(MODEM_CELLULAR_STATE_REGISTERED);
	zassert_equal(active_sim_slot(), kept->index, "Last queued request not run");
	zassert_equal(atomic_get(&emu_slot), kept->index, "Modem not on the requested slot");
}

#else /* SIM_BY_MUX */

/* ------------------------------------------------------------------------- */
/* Quectel BG95: SIM slots behind a gpio-mux                                 */
/* ------------------------------------------------------------------------- */

static void assert_iccid(int slot)
{
	char iccid[MODEM_CELLULAR_DATA_ICCID_LEN];
	char expected[MODEM_CELLULAR_DATA_ICCID_LEN];

	snprintf(expected, sizeof(expected), "895500000000000000%d", slot);
	zassert_ok(cellular_get_modem_info(modem, CELLULAR_MODEM_INFO_SIM_ICCID, iccid,
					   sizeof(iccid)));
	zassert_str_equal(iccid, expected, "ICCID not read from SIM slot %d", slot);
}

ZTEST(cellular_sim_slots_modem, test_01_boot_on_default_slot)
{
	wait_for_state(MODEM_CELLULAR_STATE_REGISTERED);

	zassert_equal(emu_current_slot(), 1, "SIM switch not routed to the default-sim slot");
	zassert_equal(active_sim_slot(), 1, "default-sim slot not active");
	assert_iccid(1);
}

ZTEST(cellular_sim_slots_modem, test_02_switch_with_data_call)
{
	atomic_val_t connects = atomic_get(&emu_cmux_connects);
	atomic_val_t dials = atomic_get(&emu_dials);

	switch_while_running(0, MODEM_CELLULAR_STATE_REGISTERED);

	assert_in_order(DETACH_CMD, DEACTIVATE_CMD);
	assert_in_order(DEACTIVATE_CMD, ACTIVATE_CMD);
	assert_in_order(ACTIVATE_CMD, "AT+QCCID");
	zassert_true(emu_log_count("AT+CPIN?") > 1, "SIM not polled until ready");
	zassert_equal(emu_log_pos("AT+CIMI") >= 0, true, "IMSI not read again");
	zassert_equal(atomic_get(&emu_mux_at_deactivate), 1,
		      "SIM switch moved before the SIM interface was deactivated");
	zassert_equal(atomic_get(&emu_mux_at_activate), 0,
		      "SIM switch not moved before the SIM interface was activated");
	zassert_equal(active_sim_slot(), 0, "SIM slot 0 not active");
	assert_iccid(0);
	zassert_equal(atomic_get(&emu_dials), dials + 1, "Data call not dialed again");
	zassert_equal(atomic_get(&emu_cmux_connects), connects, "Modem restarted for the switch");
}

ZTEST(cellular_sim_slots_modem, test_03_apn_set_along_with_switch)
{
	zassert_ok(cellular_select_sim(modem, 1));
	zassert_ok(cellular_set_apn(modem, "apn.sim1"), "APN refused along with the switch");
	wait_for_state(MODEM_CELLULAR_STATE_REGISTERED);

	assert_in_order(ACTIVATE_CMD, "AT+CGDCONT=1,\"IP\",\"apn.sim1\"");
	zassert_equal(emu_current_slot(), 1, "SIM switch not routed to slot 1");
	zassert_equal(active_sim_slot(), 1, "SIM slot 1 not active");
}

ZTEST(cellular_sim_slots_modem, test_04_failed_deactivate_keeps_slot)
{
	atomic_val_t dials = atomic_get(&emu_dials);

	strcpy(emu_fail_prefix, DEACTIVATE_CMD);
	switch_while_running(0, MODEM_CELLULAR_STATE_REGISTERED);

	zassert_equal(emu_current_slot(), 1, "SIM switch moved with the SIM interface active");
	zassert_equal(active_sim_slot(), 1, "Active SIM slot not kept");
	assert_in_order(DEACTIVATE_CMD, ACTIVATE_CMD);

	/* The SIM stayed ready, without a new +CPIN: READY: its identity is read again */
	assert_in_order(ACTIVATE_CMD, "AT+QCCID");
	assert_iccid(1);
	zassert_equal(atomic_get(&emu_dials), dials + 1, "Data call not dialed again");
}

ZTEST(cellular_sim_slots_modem, test_05_select_active_slot)
{
	zassert_equal(cellular_select_sim(modem, 1), -EALREADY, "Active SIM slot selected again");
	zassert_equal(cellular_select_sim(modem, 2), -EINVAL, "Unknown SIM slot accepted");
}

#endif /* SIM_BY_MUX */

ZTEST_SUITE(cellular_sim_slots_modem, NULL, test_setup, test_before, NULL, NULL);
