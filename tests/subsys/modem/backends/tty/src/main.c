/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*************************************************************************************************/
/*                                        Dependencies                                           */
/*************************************************************************************************/
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/modem/backend/tty.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#define TEST_MODEM_BACKEND_TTY_PIPE_EVENT_OPENED_BIT (0)
#define TEST_MODEM_BACKEND_TTY_PIPE_EVENT_RRDY_BIT   (1)
#define TEST_MODEM_BACKEND_TTY_PIPE_EVENT_TIDLE_BIT  (2)
#define TEST_MODEM_BACKEND_TTY_PIPE_EVENT_CLOSED_BIT (3)

#define TEST_MODEM_BACKEND_TTY_OP_DELAY (K_MSEC(1000))

/*************************************************************************************************/
/*                                          Mock pipe                                            */
/*************************************************************************************************/
static struct modem_backend_tty tty_backend;
static struct modem_pipe *tty_pipe;

/*************************************************************************************************/
/*                                          Mock PTY                                             */
/*************************************************************************************************/
static int primary_fd;

/*************************************************************************************************/
/*                                          Buffers                                              */
/*************************************************************************************************/
static uint8_t buffer1[1024];
K_KERNEL_STACK_DEFINE(tty_stack, 4096);

/*************************************************************************************************/
/*                                          Helpers                                              */
/*************************************************************************************************/
static void test_modem_backend_tty_cfmakeraw(struct termios *termios_p)
{
	termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	termios_p->c_oflag &= ~OPOST;
	termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	termios_p->c_cflag &= ~(CSIZE | PARENB);
	termios_p->c_cflag |= CS8;
}

/*************************************************************************************************/
/*                                     Modem pipe callback                                       */
/*************************************************************************************************/
static atomic_t tty_pipe_events;

static void modem_pipe_callback_handler(struct modem_pipe *pipe, enum modem_pipe_event event,
					void *user_data)
{
	switch (event) {
	case MODEM_PIPE_EVENT_OPENED:
		atomic_set_bit(&tty_pipe_events, TEST_MODEM_BACKEND_TTY_PIPE_EVENT_OPENED_BIT);
		break;

	case MODEM_PIPE_EVENT_RECEIVE_READY:
		atomic_set_bit(&tty_pipe_events, TEST_MODEM_BACKEND_TTY_PIPE_EVENT_RRDY_BIT);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		atomic_set_bit(&tty_pipe_events, TEST_MODEM_BACKEND_TTY_PIPE_EVENT_TIDLE_BIT);
		break;

	case MODEM_PIPE_EVENT_CLOSED:
		atomic_set_bit(&tty_pipe_events, TEST_MODEM_BACKEND_TTY_PIPE_EVENT_CLOSED_BIT);
		break;
	}
}

/*************************************************************************************************/
/*                                         Test setup                                            */
/*************************************************************************************************/
static void *test_modem_backend_tty_setup(void)
{
	const char *secondary_name;
	struct termios tio;

	primary_fd = posix_openpt(O_RDWR | O_NOCTTY);
	__ASSERT_NO_MSG(primary_fd > -1);
	__ASSERT_NO_MSG(grantpt(primary_fd) > -1);
	__ASSERT_NO_MSG(unlockpt(primary_fd) > -1);
	__ASSERT_NO_MSG(tcgetattr(primary_fd, &tio) > -1);
	test_modem_backend_tty_cfmakeraw(&tio);
	__ASSERT_NO_MSG(tcsetattr(primary_fd, TCSAFLUSH, &tio) > -1);
	secondary_name = ptsname(primary_fd);

	struct modem_backend_tty_config config = {
		.tty_path = secondary_name,
		.stack = tty_stack,
		.stack_size = K_KERNEL_STACK_SIZEOF(tty_stack),
	};

	tty_pipe = modem_backend_tty_init(&tty_backend, &config);
	modem_pipe_attach(tty_pipe, modem_pipe_callback_handler, NULL);
	__ASSERT_NO_MSG(modem_pipe_open(tty_pipe) == 0);
	return NULL;
}

static void test_modem_backend_tty_before(void *f)
{
	atomic_set(&tty_pipe_events, 0);
}

static void test_modem_backend_tty_teardown(void *f)
{
	modem_pipe_close(tty_pipe);
}

/*************************************************************************************************/
/*                                             Tests                                             */
/*************************************************************************************************/
ZTEST(modem_backend_tty_suite, test_close_open)
{
	bool result;

	zassert_ok(modem_pipe_close(tty_pipe), "Failed to close pipe");
	zassert_ok(modem_pipe_close(tty_pipe), "Pipe should already be closed");
	zassert_ok(modem_pipe_open(tty_pipe), "Failed to open pipe");
	result = atomic_test_bit(&tty_pipe_events, TEST_MODEM_BACKEND_TTY_PIPE_EVENT_TIDLE_BIT);
	zassert_true(result, "Transmit idle event should be set");
	zassert_ok(modem_pipe_open(tty_pipe), "Pipe should already be open");
}

ZTEST(modem_backend_tty_suite, test_receive_ready_event_not_raised)
{
	bool result;

	k_sleep(TEST_MODEM_BACKEND_TTY_OP_DELAY);

	result = atomic_test_bit(&tty_pipe_events, TEST_MODEM_BACKEND_TTY_PIPE_EVENT_RRDY_BIT);
	zassert_false(result, "Receive ready event should not be set");
}

ZTEST(modem_backend_tty_suite, test_receive_ready_event_raised)
{
	int ret;
	bool result;
	char msg[] = "Test me buddy";

	ret = write(primary_fd, msg, sizeof(msg));
	zassert_true(ret == sizeof(msg), "Failed to write to primary FD");

	k_sleep(TEST_MODEM_BACKEND_TTY_OP_DELAY);

	result = atomic_test_bit(&tty_pipe_events, TEST_MODEM_BACKEND_TTY_PIPE_EVENT_RRDY_BIT);
	zassert_true(result == true, "Receive ready evennt not set");
}

ZTEST(modem_backend_tty_suite, test_receive)
{
	int ret;
	char msg[] = "Test me buddy";

	ret = write(primary_fd, msg, sizeof(msg));
	zassert_true(ret == sizeof(msg), "Failed to write to primary FD");
	k_sleep(TEST_MODEM_BACKEND_TTY_OP_DELAY);

	ret = modem_pipe_receive(tty_pipe, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(msg), "Received incorrect number of bytes");
	ret = memcmp(msg, buffer1, sizeof(msg));
	zassert_true(ret == 0, "Received incorrect bytes");
}

ZTEST(modem_backend_tty_suite, test_transmit)
{
	int ret;
	char msg[] = "Test me buddy 2";
	bool result;

	result = atomic_test_bit(&tty_pipe_events, TEST_MODEM_BACKEND_TTY_PIPE_EVENT_TIDLE_BIT);
	zassert_false(result, "Transmit idle event should not be set");

	ret = modem_pipe_transmit(tty_pipe, msg, sizeof(msg));
	zassert_true(ret == sizeof(msg), "Failed to transmit using pipe");

	k_sleep(TEST_MODEM_BACKEND_TTY_OP_DELAY);

	result = atomic_test_bit(&tty_pipe_events, TEST_MODEM_BACKEND_TTY_PIPE_EVENT_TIDLE_BIT);
	zassert_true(result, "Transmit idle event should be set");

	ret = read(primary_fd, buffer1, sizeof(buffer1));
	zassert_true(ret == sizeof(msg), "Read incorrect number of bytes");
	ret = memcmp(msg, buffer1, sizeof(msg));
	zassert_true(ret == 0, "Read incorrect bytes");
}

ZTEST_SUITE(modem_backend_tty_suite, NULL, test_modem_backend_tty_setup,
	    test_modem_backend_tty_before, NULL, test_modem_backend_tty_teardown);
