/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/avdtp.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/classic/avdtp_internal.h"

/* Receive handler of the AVDTP signaling channel, not declared in a header */
int bt_avdtp_l2cap_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);

#define INT_SEID 1U

/* Indications each session received */
struct ind_count {
	size_t set_config;
	size_t get_config;
	size_t open;
	size_t abort;
	size_t delay_report;
};

struct test_session {
	struct bt_avdtp avdtp;
	struct ind_count ind;
};

static struct test_session session_a;
static struct test_session session_b;

/* One SEP per test, SEIDs are never released */
static struct bt_avdtp_sep seps[8];
static size_t seps_used;

NET_BUF_POOL_FIXED_DEFINE(cmd_pool, 1, 32, 0, NULL);

static struct ind_count *ind_of(struct bt_avdtp *avdtp)
{
	return &CONTAINER_OF(avdtp, struct test_session, avdtp)->ind;
}

static int set_config_ind(struct bt_avdtp *avdtp, struct bt_avdtp_sep *sep, uint8_t int_seid,
			  struct net_buf *buf, uint8_t *errcode)
{
	ind_of(avdtp)->set_config++;
	return 0;
}

static int get_config_ind(struct bt_avdtp *avdtp, struct bt_avdtp_sep *sep, struct net_buf *rsp_buf,
			  uint8_t *errcode)
{
	ind_of(avdtp)->get_config++;
	return 0;
}

static int open_ind(struct bt_avdtp *avdtp, struct bt_avdtp_sep *sep, uint8_t *errcode)
{
	ind_of(avdtp)->open++;
	return 0;
}

static int abort_ind(struct bt_avdtp *avdtp, struct bt_avdtp_sep *sep, uint8_t *errcode)
{
	ind_of(avdtp)->abort++;
	return 0;
}

static int delay_report_ind(struct bt_avdtp *avdtp, struct bt_avdtp_sep *sep, struct net_buf *buf,
			    uint8_t *errcode)
{
	ind_of(avdtp)->delay_report++;
	return 0;
}

static const struct bt_avdtp_ops_cb ops = {
	.set_configuration_ind = set_config_ind,
	.get_configuration_ind = get_config_ind,
	.open_ind = open_ind,
	.abort_ind = abort_ind,
	.delay_report_ind = delay_report_ind,
};

/* Feed one single-packet command to the session's signaling channel */
static void send_cmd(struct test_session *s, uint8_t sig_id, const uint8_t *params, size_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&cmd_pool, K_NO_WAIT);
	zassert_not_null(buf);

	net_buf_add_u8(buf, (BT_AVDTP_PACKET_TYPE_SINGLE << 2) | BT_AVDTP_CMD);
	net_buf_add_u8(buf, sig_id);
	net_buf_add_mem(buf, params, len);

	zassert_ok(bt_avdtp_l2cap_recv(&s->avdtp.br_chan.chan, buf));
	net_buf_unref(buf);
}

static void send_seid_cmd(struct test_session *s, uint8_t sig_id, struct bt_avdtp_sep *sep)
{
	const uint8_t params[] = {sep->sep_info.id << 2};

	send_cmd(s, sig_id, params, sizeof(params));
}

/* Register a new SEP and have session A configure it */
static struct bt_avdtp_sep *configured_sep(void)
{
	struct bt_avdtp_sep *sep;
	uint8_t params[4];

	zassert_true(seps_used < ARRAY_SIZE(seps));
	sep = &seps[seps_used++];
	zassert_ok(bt_avdtp_register_sep(BT_AVDTP_AUDIO, BT_AVDTP_SINK, sep));

	params[0] = sep->sep_info.id << 2;
	params[1] = INT_SEID << 2;
	params[2] = BT_AVDTP_SERVICE_MEDIA_TRANSPORT;
	params[3] = 0U;
	send_cmd(&session_a, BT_AVDTP_SET_CONFIGURATION, params, sizeof(params));

	zassert_equal(session_a.ind.set_config, 1U);
	zassert_equal_ptr(sep->session, &session_a.avdtp);

	return sep;
}

ZTEST(avdtp_sep_session, test_open_from_other_session)
{
	struct bt_avdtp_sep *sep = configured_sep();
	uint8_t state = sep->state;

	send_seid_cmd(&session_b, BT_AVDTP_OPEN, sep);
	zassert_equal(session_b.ind.open, 0U);
	zassert_equal(sep->state, state);
	zassert_is_null(session_b.avdtp.current_sep);

	send_seid_cmd(&session_a, BT_AVDTP_OPEN, sep);
	zassert_equal(session_a.ind.open, 1U);
	zassert_not_equal(sep->state, state);
	zassert_equal_ptr(session_a.avdtp.current_sep, sep);
}

ZTEST(avdtp_sep_session, test_get_configuration_from_other_session)
{
	struct bt_avdtp_sep *sep = configured_sep();

	send_seid_cmd(&session_b, BT_AVDTP_GET_CONFIGURATION, sep);
	zassert_equal(session_b.ind.get_config, 0U);

	send_seid_cmd(&session_a, BT_AVDTP_GET_CONFIGURATION, sep);
	zassert_equal(session_a.ind.get_config, 1U);
}

ZTEST(avdtp_sep_session, test_delay_report_from_other_session)
{
	struct bt_avdtp_sep *sep = configured_sep();
	const uint8_t params[] = {sep->sep_info.id << 2, 0x00, 0x10};

	send_cmd(&session_b, BT_AVDTP_DELAYREPORT, params, sizeof(params));
	zassert_equal(session_b.ind.delay_report, 0U);

	send_cmd(&session_a, BT_AVDTP_DELAYREPORT, params, sizeof(params));
	zassert_equal(session_a.ind.delay_report, 1U);
}

ZTEST(avdtp_sep_session, test_abort_from_other_session)
{
	struct bt_avdtp_sep *sep = configured_sep();
	uint8_t state = sep->state;

	send_seid_cmd(&session_b, BT_AVDTP_ABORT, sep);
	zassert_equal(session_b.ind.abort, 0U);
	zassert_equal(sep->state, state);

	send_seid_cmd(&session_a, BT_AVDTP_ABORT, sep);
	zassert_equal(session_a.ind.abort, 1U);
}

static void init_session(struct test_session *s)
{
	memset(s, 0, sizeof(*s));
	s->avdtp.ops = &ops;
	k_sem_init(&s->avdtp.sem_lock, 1, 1);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	init_session(&session_a);
	init_session(&session_b);
}

ZTEST_SUITE(avdtp_sep_session, NULL, NULL, before, NULL, NULL);
