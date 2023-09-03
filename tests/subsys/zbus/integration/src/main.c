/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DECLARE(version_chan, sensor_data_chan, net_pkt_chan, net_log_chan,
		  start_measurement_chan, busy_chan);

static int count_callback;

static void urgent_callback(const struct zbus_channel *chan)
{
	LOG_INF(" *** LISTENER activated for channel %s ***\n", zbus_chan_name(chan));

	++count_callback;
}

ZBUS_LISTENER_DEFINE(critical_lis, urgent_callback);

static int count_core;

ZBUS_SUBSCRIBER_DEFINE(core_sub, 1);

static void core_thread(void)
{
	const struct zbus_channel *chan = NULL;

	while (1) {
		if (!zbus_sub_wait(&core_sub, &chan, K_FOREVER)) {
			count_core++;

			struct sensor_data_msg data;

			zbus_chan_read(&sensor_data_chan, &data, K_NO_WAIT);

			struct net_pkt_msg pkt = {.total = data.a + data.b};

			LOG_DBG("Sensor {a = %d, b = %d}. Sending pkt {total=%d}", data.a, data.b,
				pkt.total);

			zbus_chan_pub(&net_pkt_chan, &pkt, K_MSEC(200));
		}
	}
}

K_THREAD_DEFINE(core_thread_id, 1024, core_thread, NULL, NULL, NULL, 3, 0, 0);

static int count_net;
static struct net_pkt_msg pkt = {0};

ZBUS_SUBSCRIBER_DEFINE(net_sub, 4);

static void net_thread(void)
{
	const struct zbus_channel *chan;

	while (1) {
		if (!zbus_sub_wait(&net_sub, &chan, K_FOREVER)) {
			count_net++;

			zbus_chan_read(&net_pkt_chan, &pkt, K_NO_WAIT);

			LOG_DBG("[Net] Total %d", pkt.total);

			struct net_log_msg log_msg = {.count_net = count_net,
						      .pkt_total = pkt.total};

			zbus_chan_pub(&net_log_chan, &log_msg, K_MSEC(500));
		}
	}
}

K_THREAD_DEFINE(net_thread_id, 1024, net_thread, NULL, NULL, NULL, 3, 0, 0);

static int count_net_log;

ZBUS_MSG_SUBSCRIBER_DEFINE(net_log_sub);

static void net_log_thread(void)
{
	const struct zbus_channel *chan;
	struct net_log_msg log_msg;

	while (1) {
		if (!zbus_sub_wait_msg(&net_log_sub, &chan, &log_msg, K_FOREVER)) {
			count_net_log++;

			LOG_DBG("[Net log]: count_net = %d, pkt.total = %d", log_msg.count_net,
				log_msg.pkt_total);
		}
	}
}

K_THREAD_DEFINE(net_log_thread_id, 1024, net_log_thread, NULL, NULL, NULL, 3, 0, 0);

static int a;
static int b;
static int count_peripheral;

ZBUS_SUBSCRIBER_DEFINE(peripheral_sub, 1);

static void peripheral_thread(void)
{
	struct sensor_data_msg sd = {0, 0};

	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&peripheral_sub, &chan, K_FOREVER)) {
		LOG_DBG("[Peripheral] starting measurement");

		++count_peripheral;
		++a;
		++b;

		sd.a = a;
		sd.b = b;

		LOG_DBG("[Peripheral] sending sensor data");

		zbus_chan_pub(&sensor_data_chan, &sd, K_MSEC(250));

		k_msleep(150);
	}
}

K_THREAD_DEFINE(peripheral_thread_id, 1024, peripheral_thread, NULL, NULL, NULL, 3, 0, 0);

static void context_reset(void *f)
{
	k_busy_wait(1000000);

	a = 0;
	b = 0;
	count_callback = 0;
	count_core = 0;
	count_net = 0;
	count_net_log = 0;
	count_peripheral = 0;
	pkt.total = 0;
	struct net_pkt_msg *p;

	zbus_chan_claim(&net_pkt_chan, K_NO_WAIT);
	p = zbus_chan_msg(&net_pkt_chan);
	p->total = 0;
	zbus_chan_finish(&net_pkt_chan);
	struct sensor_data_msg *sd;

	zbus_chan_claim(&sensor_data_chan, K_NO_WAIT);
	sd = (struct sensor_data_msg *)sensor_data_chan.message;
	sd->a = 0;
	sd->b = 1;
	zbus_chan_finish(&sensor_data_chan);
	zbus_obs_set_enable(&critical_lis, true);
	zbus_obs_set_enable(&peripheral_sub, true);
	zbus_chan_claim(&start_measurement_chan, K_NO_WAIT);
	struct action_msg *act = (struct action_msg *)zbus_chan_msg(&start_measurement_chan);

	act->status = false;
	zbus_chan_finish(&start_measurement_chan);
	zbus_chan_claim(&net_log_chan, K_NO_WAIT);
	struct net_log_msg *lm = (struct net_log_msg *)zbus_chan_msg(&net_log_chan);

	lm->count_net = 0;
	lm->pkt_total = 0;
	zbus_chan_finish(&net_log_chan);
}

ZTEST(integration, test_basic)
{
	struct action_msg start = {true};
	struct net_log_msg *lm = (struct net_log_msg *)zbus_chan_const_msg(&net_log_chan);

	zassert_equal(0, zbus_chan_pub(&start_measurement_chan, &start, K_MSEC(200)), NULL);

	k_msleep(200);

	zassert_equal(count_callback, 1, NULL);
	zassert_equal(count_core, 1, NULL);
	zassert_equal(count_net, 1, NULL);
	zassert_equal(count_peripheral, 1, NULL);
	zassert_equal(count_net_log, 1, NULL);
	zassert_equal(count_net, lm->count_net, NULL);

	zassert_equal(0, zbus_chan_pub(&start_measurement_chan, &start, K_MSEC(200)), NULL);

	k_msleep(200);

	zassert_equal(count_callback, 2, NULL);
	zassert_equal(count_core, 2, NULL);
	zassert_equal(count_net, 2, NULL);
	zassert_equal(count_peripheral, 2, NULL);
	zassert_equal(count_net_log, 2, NULL);
	zassert_equal(count_net, lm->count_net, NULL);

	zassert_equal(0, zbus_chan_pub(&start_measurement_chan, &start, K_MSEC(200)), NULL);

	k_msleep(200);

	zassert_equal(count_callback, 3, NULL);
	zassert_equal(count_core, 3, NULL);
	zassert_equal(count_net, 3, NULL);
	zassert_equal(count_peripheral, 3, NULL);
	zassert_equal(count_net_log, 3, NULL);
	zassert_equal(count_net, lm->count_net, NULL);

	zassert_equal(pkt.total, 6, "result was %d", pkt.total);
	zassert_equal(pkt.total, lm->pkt_total, NULL);
}

ZTEST(integration, test_channel_set_enable)
{
	struct action_msg start = {true};
	const struct net_log_msg *lm = zbus_chan_const_msg(&net_log_chan);

	zassert_equal(0, zbus_obs_set_enable(&critical_lis, false), NULL);
	zassert_equal(0, zbus_obs_set_enable(&peripheral_sub, false), NULL);
	zassert_equal(0, zbus_chan_pub(&start_measurement_chan, &start, K_MSEC(200)), NULL);

	k_msleep(200);

	zassert_equal(count_callback, 0, NULL);
	zassert_equal(count_core, 0, NULL);
	zassert_equal(count_peripheral, 0, NULL);
	zassert_equal(count_net, 0, NULL);
	zassert_equal(count_net_log, 0, NULL);
	zassert_equal(count_net, lm->count_net, NULL);

	zassert_equal(0, zbus_obs_set_enable(&critical_lis, false), NULL);
	zassert_equal(0, zbus_obs_set_enable(&peripheral_sub, true), NULL);
	zassert_equal(0, zbus_chan_pub(&start_measurement_chan, &start, K_MSEC(200)), NULL);

	k_msleep(200);

	zassert_equal(count_callback, 0, NULL);
	zassert_equal(count_core, 1, NULL);
	zassert_equal(count_net, 1, NULL);
	zassert_equal(count_peripheral, 1, NULL);
	zassert_equal(count_net_log, 1, NULL);
	zassert_equal(count_net, lm->count_net, NULL);

	zassert_equal(0, zbus_obs_set_enable(&critical_lis, true), NULL);
	zassert_equal(0, zbus_obs_set_enable(&peripheral_sub, false), NULL);
	zassert_equal(0, zbus_chan_pub(&start_measurement_chan, &start, K_MSEC(200)), NULL);

	k_msleep(200);

	zassert_equal(count_callback, 1, NULL);
	zassert_equal(count_core, 1, NULL);
	zassert_equal(count_net, 1, NULL);
	zassert_equal(count_peripheral, 1, NULL);
	zassert_equal(count_net_log, 1, NULL);
	zassert_equal(count_net, lm->count_net, NULL);

	zassert_equal(0, zbus_obs_set_enable(&critical_lis, true), NULL);
	zassert_equal(0, zbus_obs_set_enable(&peripheral_sub, true), NULL);
	zassert_equal(0, zbus_chan_pub(&start_measurement_chan, &start, K_MSEC(200)), NULL);

	k_msleep(200);

	zassert_equal(count_callback, 2, NULL);
	zassert_equal(count_core, 2, NULL);
	zassert_equal(count_peripheral, 2, NULL);
	zassert_equal(count_net, 2, NULL);
	zassert_equal(count_net_log, 2, NULL);
	zassert_equal(count_net, lm->count_net, NULL);

	zassert_equal(pkt.total, 4, "result was %d", pkt.total);
	zassert_equal(pkt.total, lm->pkt_total, NULL);
}

static void greedy_thread_entry(void *p1, void *p2, void *p3)
{
	int err = zbus_chan_claim(&busy_chan, K_MSEC(500));

	zassert_equal(err, 0, "Could not claim the channel");
	k_msleep(2000);
	zassert_equal(0, zbus_chan_finish(&busy_chan), NULL);
}

K_THREAD_STACK_DEFINE(greedy_thread_stack_area, 1024);

static struct k_thread greedy_thread_data;

ZTEST(integration, test_event_dispatcher_mutex_timeout)
{
	struct action_msg read;
	struct action_msg sent = {.status = true};

	int err = zbus_chan_read(&busy_chan, &read, K_NO_WAIT);

	zassert_equal(err, 0, "Could not read the channel");

	zassert_equal(read.status, 0, "Read status must be false");

	k_thread_create(&greedy_thread_data, greedy_thread_stack_area,
			K_THREAD_STACK_SIZEOF(greedy_thread_stack_area), greedy_thread_entry, NULL,
			NULL, NULL, CONFIG_ZTEST_THREAD_PRIORITY, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_msleep(500);

	err = zbus_chan_pub(&busy_chan, &sent, K_MSEC(200));
	zassert_equal(err, -EAGAIN, "Channel must be busy and could no be published %d", err);
	err = zbus_chan_read(&busy_chan, &read, K_MSEC(200));
	zassert_equal(err, -EAGAIN, "Channel must be busy and could no be published %d", err);
	err = zbus_chan_claim(&busy_chan, K_MSEC(200));
	zassert_equal(err, -EAGAIN, "Channel must be busy and could no be claimed %d", err);
	err = zbus_chan_pub(&busy_chan, &sent, K_MSEC(200));
	zassert_equal(err, -EAGAIN, "Channel must be busy and could no be published %d", err);
	/* Wait for the greedy thread to finish and pub and read successfully */
	err = zbus_chan_pub(&busy_chan, &sent, K_MSEC(2000));
	zassert_equal(err, 0, "Channel must be busy and could no be published %d", err);
	err = zbus_chan_read(&busy_chan, &read, K_MSEC(2000));
	zassert_equal(err, 0, "Could not read the channel");

	zassert_equal(read.status, true, "Read status must be false");
}

ZTEST(integration, test_event_dispatcher_queue_timeout)
{
	struct action_msg sent = {.status = true};
	struct action_msg read = {.status = true};

	zassert_equal(0, zbus_obs_set_enable(&core_sub, false), NULL);
	zassert_equal(0, zbus_obs_set_enable(&net_sub, false), NULL);
	int err = zbus_chan_pub(&start_measurement_chan, &sent, K_MSEC(100));

	zassert_equal(err, 0, "Could not pub the channel");
	k_msleep(10);
	sent.status = false;
	err = zbus_chan_pub(&start_measurement_chan, &sent, K_MSEC(100));
	zassert_equal(err, 0, "Could not pub the channel");
	k_msleep(10);
	err = zbus_chan_pub(&start_measurement_chan, &sent, K_MSEC(100));
	zassert_equal(err, -EAGAIN, "Pub to the channel %d must not occur here", err);
	err = zbus_chan_read(&start_measurement_chan, &read, K_NO_WAIT);
	zassert_equal(err, 0, "Could not read the channel");
	zassert_equal(read.status, false,
		      "Read status must be false. The notification was not sent, but "
		      "the channel actually changed");
	k_msleep(500);
	zassert_equal(count_callback, 3, NULL);
	zassert_equal(count_peripheral, 2, NULL);
}

ZTEST_SUITE(integration, NULL, NULL, context_reset, NULL, NULL);
