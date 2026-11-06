/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_HELPERS_H_
#define TEST_HELPERS_H_

#include "test_backend.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

/* APN set by advance_to(), and the command the driver configures it with */
#define TEST_APN     "internet"
#define TEST_APN_CMD "AT+CGDCONT=1,\"IP\",\"" TEST_APN "\""

/* Modem info strings reported by modems with the modem-info-script property. TEST_IMEI_SN is
 * reported first as both IMEI and serial number, then replaced by TEST_IMEI and
 * TEST_SERIAL_NUMBER.
 */
#define TEST_IMEI_SN       "490154203237518"
#define TEST_IMEI          "352656100367872"
#define TEST_SERIAL_NUMBER "SN0123456789"
#define TEST_MODEL_ID      "ZT-1000"
#define TEST_MANUFACTURER  "Zephyr Test Modems"
#define TEST_FW_VERSION    "ZT-1000_v1.2.3"
#define TEST_IMSI          "001010123456789"
#define TEST_ICCID         "89882390000123456789"

/*
 * Points advance_to() can stop a boot at, each defined by what the modem has seen.
 * Steps before BOOT_SET_BAUDRATE are timed, and only last long enough to catch on a
 * modem with slow timings.
 */
enum boot_step {
	/* Reset line asserted */
	BOOT_RESET_PULSE,
	/* Reset line released again, power key not yet pressed */
	BOOT_RESET_RELEASED,
	/* Power key asserted */
	BOOT_POWER_KEY,
	/* UART opened, waiting for the modem to start */
	BOOT_UART_OPENED,
	/* Set baudrate command sent, unanswered */
	BOOT_SET_BAUDRATE,
	/* Init command sent, unanswered */
	BOOT_INIT,
	/* Init command answered, first modem info command sent and unanswered. Only reached
	 * by modems with the modem-info-script property.
	 */
	BOOT_MODEM_INFO,
	/* Init command answered with an error, waiting out the recovery backoff */
	BOOT_RECOVERY,
	/* Init command answered, CMUX connect started and held until
	 * test_backend_cmux_connect_finish()
	 */
	BOOT_CMUX_PENDING,
	/* DLCI setup command sent on DLCI1, unanswered */
	BOOT_DLCI1_SETUP,
	/* DLCI setup command sent on DLCI2, unanswered */
	BOOT_DLCI2_SETUP,
	/* Channels open and set up, nothing sent until an APN is configured */
	BOOT_AWAIT_APN,
	/* APN command sent, unanswered */
	BOOT_APN,
	/* Network command sent, unanswered */
	BOOT_NETWORK,
	/* Waiting for the modem to report registration */
	BOOT_AWAIT_REGISTRATION,
	/* Dial command sent, unanswered */
	BOOT_DIAL,
	/* Registered, with the network interface up */
	BOOT_LINK_UP,
	/* Registration lost, network interface dormant again */
	BOOT_DEREGISTERED,
};

/* Poll until the link to @p dev reaches @p state, returning false on timeout */
bool wait_for_state(const struct device *dev, enum test_backend_state state,
		    k_timeout_t timeout);

/* Assert that the link to @p dev reaches @p state */
void expect_state(const struct device *dev, enum test_backend_state state);

/* Assert that the driver drives @p spec to @p value */
void expect_gpio(const struct gpio_dt_spec *spec, int value);

/* Assert that the next line sent by the driver is @p expected on @p pipe */
void expect_line(enum test_backend_pipe_id pipe, const char *expected);

/* Assert that the driver sends nothing within @p timeout */
void expect_no_line(k_timeout_t timeout);

/* Assert that the driver reads every response injected on @p pipe */
void expect_read(enum test_backend_pipe_id pipe);

/* Respond "OK" on @p pipe */
void ok(enum test_backend_pipe_id pipe);

/* Respond "ERROR" on @p pipe */
void respond_error(enum test_backend_pipe_id pipe);

/* Report LTE registration on @p pipe */
void registered(enum test_backend_pipe_id pipe);

/* Baudrate @p dev powers up at, before the driver switches it */
uint32_t modem_power_up_baudrate(const struct device *dev);

/* Current baudrate of the UART @p dev is attached to */
uint32_t uart_baudrate(const struct device *dev);

/* Wait for the driver to handle every event delegated so far */
void flush_driver_events(void);

/* Halt @p dev and return its driver and backend to a clean idle state */
void modem_reset_state(const struct device *dev);

/* Reset @p dev and initialise the driver again, starting a fresh boot */
void modem_restart(const struct device *dev);

/*
 * Drive a fresh boot of @p dev forward until the driver sits in @p target, answering
 * each command along the way. Script steps the modem does not have are skipped.
 */
void advance_to(const struct device *dev, enum boot_step target);

/* Suspend @p dev from a separate thread, as it blocks until the modem is powered down */
void start_suspend(const struct device *dev);

/* Assert that the suspend started by start_suspend() completed into idle */
void finish_suspend(const struct device *dev);

/* Suspend @p dev, answering the shutdown script */
void suspend_and_shutdown(const struct device *dev);

/* Suite hooks shared by every suite in this application */
void *modem_test_setup(void);
void modem_test_before(void *fixture);
void modem_test_after(void *fixture);

#endif /* TEST_HELPERS_H_ */
