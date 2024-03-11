/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bs_tracing.h>
#include <bstests.h>
#include <stdint.h>
#include <testlib/conn.h>
#include <testlib/scan.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic_builtin.h>
#include <zephyr/sys/atomic_types.h>

#include "adv_resumer.h"

LOG_MODULE_REGISTER(adv_resumer, LOG_LEVEL_INF);

static void resume_work_handler(struct k_work *work);
static struct k_work resume_work = {.handler = resume_work_handler};

static K_MUTEX_DEFINE(sync);

/* Initialized to NULL, which means restarting is disabled. */
static adv_starter_t *adv_starter;

void adv_resumer_set(adv_starter_t *new_adv_starter)
{
	k_mutex_lock(&sync, K_FOREVER);

	adv_starter = new_adv_starter;
	if (adv_starter) {
		k_work_submit(&resume_work);
	} else {
		bt_le_adv_stop();
	}

	k_mutex_unlock(&sync);
}

static void resume_work_handler(struct k_work *self)
{
	int err;

	/* The timeout is defence-in-depth. The lock has a dependency
	 * the blocking Bluetooth API. This can form a deadlock if the
	 * Bluetooth API happens to have a dependency on the
	 * work queue.
	 *
	 * The timeout is not zero to avoid busy-waiting.
	 */
	err = k_mutex_lock(&sync, K_MSEC(1));
	if (err) {
		LOG_DBG("reshed");
		k_work_submit(self);

		/* We did not get the lock. */
		return;
	}

	if (adv_starter) {
		err = adv_starter();

		switch (err) {
		case 0:
		case -EALREADY:
		case -ECONNREFUSED:
		case -ENOMEM:
			break;
		default:
			LOG_ERR("adv_starter failed: %d", err);
		}
	}

	k_mutex_unlock(&sync);
}

static void on_conn_connected(struct bt_conn *conn, uint8_t conn_err)
{
	/* In case this connection terminated the advertiser. */
	k_work_submit(&resume_work);
}

static void on_conn_recycled(void)
{
	/* In case we could not start the advertiser earlier due
	 * to lack of connection objects.
	 */
	k_work_submit(&resume_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = on_conn_connected,
	.recycled = on_conn_recycled,
};
