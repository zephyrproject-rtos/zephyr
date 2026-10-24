/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/net/rfc2217.h>
#include <zephyr/ztest.h>

#define IAC      0xff
#define DO_NOT   0xfe
#define DO       0xfd
#define WILL_NOT 0xfc
#define WILL     0xfb
#define SB       0xfa
#define SE       0xf0

#define COM_PORT 44

static struct rfc2217_session session;

static uint8_t data[64];
static size_t data_len;
static uint8_t sent[64];
static size_t sent_len;

static uint32_t baud_rate = 9600;
static uint8_t data_size = 8;
static uint8_t parity = RFC2217_PARITY_NONE;
static uint8_t stop_size = RFC2217_STOP_SIZE_1;
static uint32_t last_baud_rate;
static uint8_t last_control;
static uint8_t last_purge;
static int suspended;

static void on_data(struct rfc2217_session *s, const uint8_t *buf, size_t len)
{
	zassert_true(data_len + len <= sizeof(data));
	memcpy(&data[data_len], buf, len);
	data_len += len;
}

static void on_send(struct rfc2217_session *s, const uint8_t *buf, size_t len)
{
	zassert_true(sent_len + len <= sizeof(sent));
	memcpy(&sent[sent_len], buf, len);
	sent_len += len;
}

static uint32_t on_baud_rate(struct rfc2217_session *s, uint32_t value)
{
	last_baud_rate = value;
	if (value != 0 && value <= 1000000) {
		baud_rate = value;
	}

	return baud_rate;
}

static uint8_t on_data_size(struct rfc2217_session *s, uint8_t value)
{
	if (value >= 5 && value <= 8) {
		data_size = value;
	}

	return data_size;
}

static uint8_t on_parity(struct rfc2217_session *s, uint8_t value)
{
	if (value == RFC2217_PARITY_NONE || value == RFC2217_PARITY_EVEN) {
		parity = value;
	}

	return parity;
}

static uint8_t on_stop_size(struct rfc2217_session *s, uint8_t value)
{
	if (value == RFC2217_STOP_SIZE_1 || value == RFC2217_STOP_SIZE_2) {
		stop_size = value;
	}

	return stop_size;
}

static uint8_t on_control(struct rfc2217_session *s, uint8_t value)
{
	last_control = value;

	return value == RFC2217_CONTROL_BREAK_ON ? RFC2217_CONTROL_BREAK_OFF : value;
}

static void on_purge(struct rfc2217_session *s, uint8_t value)
{
	last_purge = value;
}

static void on_suspend(struct rfc2217_session *s, bool suspend)
{
	suspended = suspend ? 1 : 0;
}

static uint8_t on_modem_state(struct rfc2217_session *s)
{
	return RFC2217_MODEM_CD | RFC2217_MODEM_DSR | RFC2217_MODEM_CTS;
}

static const struct rfc2217_session_ops ops = {
	.data = on_data,
	.send = on_send,
	.baud_rate = on_baud_rate,
	.data_size = on_data_size,
	.parity = on_parity,
	.stop_size = on_stop_size,
	.control = on_control,
	.purge = on_purge,
	.suspend = on_suspend,
	.modem_state = on_modem_state,
};

static void input(const uint8_t *buf, size_t len)
{
	data_len = 0;
	sent_len = 0;
	rfc2217_session_input(&session, buf, len);
}

#define INPUT(...)                                                                                 \
	do {                                                                                       \
		const uint8_t _in[] = {__VA_ARGS__};                                               \
		input(_in, sizeof(_in));                                                           \
	} while (false)

#define ASSERT_SENT(...)                                                                           \
	do {                                                                                       \
		const uint8_t _exp[] = {__VA_ARGS__};                                              \
		zassert_equal(sent_len, sizeof(_exp), "sent %zu bytes", sent_len);                 \
		zassert_mem_equal(sent, _exp, sizeof(_exp));                                       \
	} while (false)

#define ASSERT_DATA(...)                                                                           \
	do {                                                                                       \
		const uint8_t _exp[] = {__VA_ARGS__};                                              \
		zassert_equal(data_len, sizeof(_exp), "got %zu bytes", data_len);                  \
		zassert_mem_equal(data, _exp, sizeof(_exp));                                       \
	} while (false)

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	data_len = 0;
	sent_len = 0;
	baud_rate = 9600;
	data_size = 8;
	parity = RFC2217_PARITY_NONE;
	stop_size = RFC2217_STOP_SIZE_1;
	last_baud_rate = 0xffffffff;
	last_control = 0xff;
	last_purge = 0;
	suspended = -1;
	rfc2217_session_init(&session, &ops, NULL);
}

ZTEST(rfc2217, test_opening)
{
	ASSERT_SENT(IAC, WILL, 1, IAC, WILL, 3, IAC, DO, 0, IAC, WILL, COM_PORT);
	zassert_false(rfc2217_session_is_active(&session));
}

ZTEST(rfc2217, test_negotiation)
{
	/* Answers to the server's own requests are not answered again */
	INPUT(IAC, DO, 1, IAC, DO, 3, IAC, WILL, 0, IAC, DO, COM_PORT);
	zassert_equal(sent_len, 0);
	zassert_true(rfc2217_session_is_active(&session));

	/* A client asking first gets an agreement */
	INPUT(IAC, WILL, COM_PORT, IAC, DO, 0);
	ASSERT_SENT(IAC, DO, COM_PORT, IAC, WILL, 0);

	/* Other options are refused */
	INPUT(IAC, DO, 24, IAC, WILL, 31);
	ASSERT_SENT(IAC, WILL_NOT, 24, IAC, DO_NOT, 31);

	/* Disabling an option is acknowledged once */
	INPUT(IAC, DO_NOT, 1, IAC, DO_NOT, 1);
	ASSERT_SENT(IAC, WILL_NOT, 1);
}

ZTEST(rfc2217, test_line_settings)
{
	INPUT(IAC, SB, COM_PORT, 1, 0x00, 0x01, 0xc2, 0x00, IAC, SE);
	zassert_equal(last_baud_rate, 115200);
	ASSERT_SENT(IAC, SB, COM_PORT, 101, 0x00, 0x01, 0xc2, 0x00, IAC, SE);

	/* A refused value is answered with the one in effect */
	INPUT(IAC, SB, COM_PORT, 1, 0x00, 0x3d, 0x09, 0x00, IAC, SE);
	ASSERT_SENT(IAC, SB, COM_PORT, 101, 0x00, 0x01, 0xc2, 0x00, IAC, SE);

	/* Zero asks for the current value */
	INPUT(IAC, SB, COM_PORT, 1, 0, 0, 0, 0, IAC, SE);
	zassert_equal(last_baud_rate, 0);
	ASSERT_SENT(IAC, SB, COM_PORT, 101, 0x00, 0x01, 0xc2, 0x00, IAC, SE);

	INPUT(IAC, SB, COM_PORT, 2, 7, IAC, SE);
	ASSERT_SENT(IAC, SB, COM_PORT, 102, 7, IAC, SE);
	INPUT(IAC, SB, COM_PORT, 3, RFC2217_PARITY_MARK, IAC, SE);
	ASSERT_SENT(IAC, SB, COM_PORT, 103, RFC2217_PARITY_NONE, IAC, SE);
	INPUT(IAC, SB, COM_PORT, 3, RFC2217_PARITY_EVEN, IAC, SE);
	ASSERT_SENT(IAC, SB, COM_PORT, 103, RFC2217_PARITY_EVEN, IAC, SE);
	INPUT(IAC, SB, COM_PORT, 4, RFC2217_STOP_SIZE_2, IAC, SE);
	ASSERT_SENT(IAC, SB, COM_PORT, 104, RFC2217_STOP_SIZE_2, IAC, SE);
}

ZTEST(rfc2217, test_escaped_values)
{
	/* 0xff in a value is doubled both ways */
	INPUT(IAC, SB, COM_PORT, 1, 0x00, 0x00, IAC, IAC, 0x01, IAC, SE);
	zassert_equal(last_baud_rate, 0xff01);
	ASSERT_SENT(IAC, SB, COM_PORT, 101, 0x00, 0x00, IAC, IAC, 0x01, IAC, SE);
}

ZTEST(rfc2217, test_control)
{
	INPUT(IAC, SB, COM_PORT, 5, RFC2217_CONTROL_DTR_ON, IAC, SE);
	zassert_equal(last_control, RFC2217_CONTROL_DTR_ON);
	ASSERT_SENT(IAC, SB, COM_PORT, 105, RFC2217_CONTROL_DTR_ON, IAC, SE);

	INPUT(IAC, SB, COM_PORT, 5, RFC2217_CONTROL_BREAK_ON, IAC, SE);
	ASSERT_SENT(IAC, SB, COM_PORT, 105, RFC2217_CONTROL_BREAK_OFF, IAC, SE);
}

ZTEST(rfc2217, test_other_commands)
{
	INPUT(IAC, SB, COM_PORT, 0, IAC, SE);
	ASSERT_SENT(IAC, SB, COM_PORT, 100, 'Z', 'e', 'p', 'h', 'y', 'r', IAC, SE);

	/* The client's own signature is not answered */
	INPUT(IAC, SB, COM_PORT, 0, 'p', 'y', IAC, SE);
	zassert_equal(sent_len, 0);

	INPUT(IAC, SB, COM_PORT, 12, RFC2217_PURGE_BOTH, IAC, SE);
	zassert_equal(last_purge, RFC2217_PURGE_BOTH);
	ASSERT_SENT(IAC, SB, COM_PORT, 112, RFC2217_PURGE_BOTH, IAC, SE);

	INPUT(IAC, SB, COM_PORT, 8, IAC, SE);
	zassert_equal(suspended, 1);
	zassert_equal(sent_len, 0);
	INPUT(IAC, SB, COM_PORT, 9, IAC, SE);
	zassert_equal(suspended, 0);

	INPUT(IAC, SB, COM_PORT, 7, IAC, SE);
	ASSERT_SENT(IAC, SB, COM_PORT, 107, 0xb0, IAC, SE);

	INPUT(IAC, SB, COM_PORT, 11, 0xf0, IAC, SE);
	ASSERT_SENT(IAC, SB, COM_PORT, 111, 0xf0, IAC, SE);

	/* A subnegotiation longer than the buffer is dropped */
	INPUT(IAC, SB, COM_PORT, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	      'n', 'o', 'p', 'q', IAC, SE);
	zassert_equal(sent_len, 0);
}

ZTEST(rfc2217, test_data)
{
	INPUT('a', IAC, IAC, 'b', IAC, 241, 'c');
	ASSERT_DATA('a', IAC, 'b', 'c');

	/* Sequences split across reads */
	INPUT('d', IAC);
	ASSERT_DATA('d');
	INPUT(IAC, 'e', IAC, SB, COM_PORT);
	ASSERT_DATA(IAC, 'e');
	INPUT(2, 6, IAC);
	zassert_equal(data_len, 0);
	INPUT(SE, 'f');
	ASSERT_SENT(IAC, SB, COM_PORT, 102, 6, IAC, SE);
	ASSERT_DATA('f');
}

ZTEST(rfc2217, test_cr_nul)
{
	/* Without the binary option, CR NUL is a CR */
	INPUT('a', '\r', '\0', 'b');
	ASSERT_DATA('a', '\r', 'b');

	INPUT(IAC, WILL, 0);
	INPUT('a', '\r', '\0', 'b');
	ASSERT_DATA('a', '\r', '\0', 'b');
}

ZTEST(rfc2217, test_escape)
{
	const uint8_t in[] = {'a', IAC, 'b', IAC};
	uint8_t out[8];
	size_t consumed;
	size_t n;

	n = rfc2217_escape(in, sizeof(in), out, sizeof(out), &consumed);
	zassert_equal(n, 6);
	zassert_equal(consumed, 4);
	zassert_mem_equal(out, ((uint8_t[]){'a', IAC, IAC, 'b', IAC, IAC}), 6);

	/* An escaped byte is never split */
	n = rfc2217_escape(in, sizeof(in), out, 2, &consumed);
	zassert_equal(n, 1);
	zassert_equal(consumed, 1);
	n = rfc2217_escape(in, sizeof(in), out, 5, &consumed);
	zassert_equal(n, 4);
	zassert_equal(consumed, 3);
}

ZTEST_SUITE(rfc2217, NULL, NULL, before, NULL, NULL);
