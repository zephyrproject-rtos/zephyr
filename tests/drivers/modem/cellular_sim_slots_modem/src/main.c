/*
 * Copyright (c) 2026 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SIM slot selection by the modem, for a SIMCom A76xx with two SIM interfaces.
 * The tests play the modem (DCE) side over an emulated UART: the raw init AT
 * script first, then a modem_cmux peer bridged through the modem backend mock
 * once the driver switches to CMUX, as in tests/drivers/modem/
 * cellular_on_demand_connect. The emulated modem keeps the selected SIM
 * interface as if in its non-volatile memory, and counts the commands that
 * read and write it.
 *
 * No APN is configured, so the driver parks in WAIT_FOR_APN right after the
 * SIM slot selection.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/drivers/modem/modem_cellular.h>
#include <zephyr/modem/cmux.h>
#include <zephyr/modem/pipe.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>

#include <modem_backend_mock.h>

#define SELECT_CMD "AT*SELECTSIMSLOT="
#define QUERY_CMD  "AT*SELECTSIMSLOT?"

static const struct device *const modem = DEVICE_DT_GET(DT_NODELABEL(modem));
static const struct device *const modem_uart = DEVICE_DT_GET(DT_NODELABEL(modem_uart));

/* State of the emulated modem: the selected SIM interface, kept across power cycles */
static atomic_t emu_slot;
static atomic_t emu_queries;
static atomic_t emu_writes;
static char emu_last_write[32];

/* Knobs the tests flip to make the emulated modem refuse a command */
static atomic_t emu_fail_query;
static atomic_t emu_fail_select;

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

static void to_driver(const char *resp)
{
	uart_emul_put_rx_data(modem_uart, (const uint8_t *)resp, strlen(resp));
}

static void dce_send(struct modem_pipe *pipe, const char *resp)
{
	modem_pipe_transmit(pipe, (const uint8_t *)resp, strlen(resp));
}

/* Answer one raw init script line, or switch to CMUX */
static void at_phase_respond(const char *line)
{
	if (strncmp(line, "AT+CMUX", 7) == 0) {
		/* No reply expected: the driver brings up CMUX */
		emu_phase = EMU_PHASE_CMUX;
	} else if (strcmp(line, "AT+CGSN") == 0) {
		to_driver("359123456789012\r\nOK\r\n");
	} else if (strcmp(line, "AT+CGMM") == 0) {
		to_driver("A7663E\r\nOK\r\n");
	} else if (line[0] != '\0') {
		to_driver("OK\r\n");
	}
}

/* Answer the SIM slot query and selection, and any other command, on DLCI1 */
static void dlci1_respond(const char *line)
{
	char resp[48];

	if (strcmp(line, QUERY_CMD) == 0) {
		atomic_inc(&emu_queries);
		if (atomic_get(&emu_fail_query)) {
			dce_send(dce_dlci1_pipe, "ERROR\r\n");
			return;
		}

		snprintf(resp, sizeof(resp), "*SELECTUSIM: %d\r\nOK\r\n",
			 (int)atomic_get(&emu_slot));
		dce_send(dce_dlci1_pipe, resp);
	} else if (strncmp(line, SELECT_CMD, strlen(SELECT_CMD)) == 0) {
		atomic_inc(&emu_writes);
		strncpy(emu_last_write, line, sizeof(emu_last_write) - 1);
		if (atomic_get(&emu_fail_select)) {
			dce_send(dce_dlci1_pipe, "ERROR\r\n");
			return;
		}

		atomic_set(&emu_slot, atoi(line + strlen(SELECT_CMD)));
		dce_send(dce_dlci1_pipe, "OK\r\n");
	} else {
		dce_send(dce_dlci1_pipe, "OK\r\n");
	}
}

/* Answer the commands on DLCI2, the shutdown script among them */
static void dlci2_respond(const char *line)
{
	ARG_UNUSED(line);

	dce_send(dce_dlci2_pipe, "OK\r\n");
}

/* Assemble '\r'-delimited lines starting with "AT", dropping '\n' */
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
static void emulator_rewind(void)
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

/* native_sim runs on virtual time, so the wait costs no wall-clock time */
static void wait_for_apn_state(void)
{
	zassert_true(WAIT_FOR(modem_state() == MODEM_CELLULAR_STATE_WAIT_FOR_APN, 60000000,
			      k_msleep(20)),
		     "Modem did not park in WAIT_FOR_APN, state is %d", modem_state());
}

static void modem_suspend(void)
{
	zassert_ok(pm_device_action_run(modem, PM_DEVICE_ACTION_SUSPEND), "Suspend failed");
	zassert_equal(modem_state(), MODEM_CELLULAR_STATE_IDLE, "Modem not idle once suspended");
	emulator_rewind();
}

static void modem_resume(void)
{
	zassert_ok(pm_device_action_run(modem, PM_DEVICE_ACTION_RESUME), "Resume failed");
	wait_for_apn_state();
}

static uint8_t active_sim_slot(void)
{
	uint8_t count;
	uint8_t active;

	zassert_ok(cellular_get_sim_slots(modem, &count, &active));
	zassert_equal(count, 2, "Unexpected SIM slot count %u", count);

	return active;
}

/* ------------------------------------------------------------------------- */
/* Tests, run in order: each one starts from the state the previous one left */
/* ------------------------------------------------------------------------- */

ZTEST(cellular_sim_slots_modem, test_01_boot_selects_default_slot)
{
	wait_for_apn_state();

	zassert_true(atomic_get(&emu_queries) > 0, "Selected SIM interface not queried");
	zassert_equal(atomic_get(&emu_writes), 1, "Unexpected number of SIM selections");
	zassert_str_equal(emu_last_write, "AT*SELECTSIMSLOT=1,0", "Unexpected select command");
	zassert_equal(atomic_get(&emu_slot), 1, "Modem not on the default-sim slot");
	zassert_equal(active_sim_slot(), 1, "default-sim slot not active");
}

ZTEST(cellular_sim_slots_modem, test_02_selected_slot_not_written_again)
{
	atomic_val_t queries = atomic_get(&emu_queries);

	modem_suspend();
	modem_resume();

	zassert_true(atomic_get(&emu_queries) > queries, "Selected SIM interface not queried");
	zassert_equal(atomic_get(&emu_writes), 1, "Slot the modem had selected written again");
	zassert_equal(active_sim_slot(), 1, "Active SIM slot changed");
}

ZTEST(cellular_sim_slots_modem, test_03_select_rejected_while_running)
{
	zassert_equal(cellular_select_sim(modem, 0), -EBUSY,
		      "SIM slot switched while the modem is running");
	zassert_equal(active_sim_slot(), 1, "Active SIM slot changed");
}

ZTEST(cellular_sim_slots_modem, test_04_select_applied_on_power_up)
{
	modem_suspend();

	zassert_ok(cellular_select_sim(modem, 0));
	zassert_equal(active_sim_slot(), 0, "SIM slot 0 not active");
	zassert_equal(atomic_get(&emu_writes), 1, "Powered off modem received a command");

	modem_resume();

	zassert_equal(atomic_get(&emu_writes), 2, "SIM slot not selected on power-up");
	zassert_str_equal(emu_last_write, "AT*SELECTSIMSLOT=0,0", "Unexpected select command");
	zassert_equal(atomic_get(&emu_slot), 0, "Modem not on SIM slot 0");
}

ZTEST(cellular_sim_slots_modem, test_05_failed_select_keeps_modem_slot)
{
	modem_suspend();
	zassert_ok(cellular_select_sim(modem, 1));

	atomic_set(&emu_fail_select, 1);
	modem_resume();
	atomic_set(&emu_fail_select, 0);

	zassert_equal(atomic_get(&emu_writes), 3, "SIM slot selection not attempted");
	zassert_equal(atomic_get(&emu_slot), 0, "Modem changed SIM interface");
	zassert_equal(active_sim_slot(), 0, "Active SIM slot not the one the modem kept");
}

ZTEST(cellular_sim_slots_modem, test_06_failed_query_selects_anyway)
{
	modem_suspend();
	zassert_ok(cellular_select_sim(modem, 1));

	atomic_set(&emu_fail_query, 1);
	modem_resume();
	atomic_set(&emu_fail_query, 0);

	zassert_equal(atomic_get(&emu_writes), 4, "SIM slot not selected without a query");
	zassert_equal(atomic_get(&emu_slot), 1, "Modem not on SIM slot 1");
	zassert_equal(active_sim_slot(), 1, "SIM slot 1 not active");
}

ZTEST(cellular_sim_slots_modem, test_07_invalid_query_response_ignored)
{
	struct modem_cellular_data *data = modem->data;
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
}

static void *test_setup(void)
{
	k_mutex_init(&pump_lock);

	zassert_true(device_is_ready(modem), "Modem not ready");
	zassert_true(device_is_ready(modem_uart), "Emulated UART not ready");

	/* The driver started at boot; answer it from now on, from SIM interface 0 */
	atomic_set(&emu_slot, 0);
	emulator_init();
	emu_pump();

	return NULL;
}

ZTEST_SUITE(cellular_sim_slots_modem, NULL, test_setup, NULL, NULL, NULL);
