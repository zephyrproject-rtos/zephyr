/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_BACKEND_H_
#define TEST_BACKEND_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/*
 * Test-side view of the modem_cellular transport.
 *
 * The modem_cellular state machine writes AT commands to either the initial
 * UART pipe or one of the CMUX DLCI pipes. The backend records each
 * '\r'-terminated command as a line tagged with the pipe that carried it, so a
 * test can wait for the next driver action and then inject the modem response
 * that should drive the next state transition.
 */
enum test_backend_pipe_id {
	TEST_BACKEND_PIPE_UART,
	TEST_BACKEND_PIPE_DLCI1,
	TEST_BACKEND_PIPE_DLCI2,
	TEST_BACKEND_PIPE_USER3,
	TEST_BACKEND_PIPE_USER4,
};

struct test_backend_line {
	enum test_backend_pipe_id pipe;
	char data[128];
};

/*
 * How far the driver has brought up its link to the modem, as seen from the modem
 * side of the transport. Power and reset lines are observed separately.
 */
enum test_backend_state {
	/* UART closed */
	TEST_BACKEND_STATE_OFF,
	/* UART open for AT commands */
	TEST_BACKEND_STATE_AT,
	/* CMUX connected over the UART */
	TEST_BACKEND_STATE_CMUX,
	/* PPP attached to the data channel, network interface dormant */
	TEST_BACKEND_STATE_DIALED,
	/* PPP attached to the data channel, network interface up */
	TEST_BACKEND_STATE_LINK_UP,
};

/* Clear queued driver lines and pending injected responses before a suite/test. */
void test_backend_reset(const struct device *modem);

/* Current state of the link between @p modem and its driver */
enum test_backend_state test_backend_state(const struct device *modem);

/* UART @p modem is attached to */
const struct device *test_backend_uart(const struct device *modem);

/* Power and reset lines of @p modem, with a NULL port when the line is absent */
const struct gpio_dt_spec *test_backend_power_gpio(const struct device *modem);
const struct gpio_dt_spec *test_backend_reset_gpio(const struct device *modem);

/* Whether @p modem runs the optional set baudrate, network, DLCI setup and periodic scripts */
bool test_backend_has_set_baudrate_script(const struct device *modem);
bool test_backend_has_network_script(const struct device *modem);
bool test_backend_has_dlci_setup_script(const struct device *modem);
bool test_backend_has_periodic_script(const struct device *modem);

/* Whether the init script of @p modem queries every modem info string */
bool test_backend_has_modem_info_script(const struct device *modem);

/* Wait for the next complete line sent by the driver. */
bool test_backend_wait_for_line(struct test_backend_line *line, k_timeout_t timeout);

/* Inject bytes from the emulated modem back into the selected driver pipe. */
void test_backend_respond(enum test_backend_pipe_id pipe, const char *response);

/* Wait for the driver to read every byte injected into the selected pipe. */
bool test_backend_wait_for_read(enum test_backend_pipe_id pipe, k_timeout_t timeout);

/* Leave CMUX connect attempts pending until test_backend_cmux_connect_finish(). */
void test_backend_cmux_connect_hold(bool hold);

/* Wait for the driver to start a CMUX connect attempt. */
bool test_backend_wait_for_cmux_connect(k_timeout_t timeout);

/*
 * Complete the pending CMUX connect attempt, failing it as if the modem never
 * answered when @p connected is false.
 */
void test_backend_cmux_connect_finish(bool connected);

/* Close the UART under the CMUX, as its power saving does with close_pipe_on_power_save */
void test_backend_cmux_power_save(void);

/* Whether the driver has @p pipe open. */
bool test_backend_pipe_is_open(enum test_backend_pipe_id pipe);

#endif /* TEST_BACKEND_H_ */
