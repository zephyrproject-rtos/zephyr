/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/ecc.h"

/* The debug public key of Core Vol 3, Part H, 2.3.5.6.1, little-endian */
static const uint8_t other_pk[BT_PUB_KEY_LEN] = {
	0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc, 0xdb, 0xfd, 0xf4, 0xac, 0x11,
	0x91, 0xf4, 0xef, 0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e, 0x2c, 0xbe,
	0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20, 0x8b, 0xd2, 0x89, 0x15, 0xd0, 0x8e, 0x1c,
	0x74, 0x24, 0x30, 0xed, 0x8f, 0xc2, 0x45, 0x63, 0x76, 0x5c, 0x15, 0x52, 0x5a,
	0xbf, 0x9a, 0x32, 0x63, 0x6d, 0xeb, 0x2a, 0x65, 0x49, 0x9c, 0x80, 0xdc,
};

static K_SEM_DEFINE(pub_key_sem, 0, 1);
static K_SEM_DEFINE(dh_key_sem, 0, 1);

/* The remote key that the requester keeps for bt_dh_key_gen() */
static uint8_t remote_pk[BT_PUB_KEY_LEN];

static bool dh_key_valid;

static void pub_key_ready(const uint8_t key[BT_PUB_KEY_LEN])
{
	if (key != NULL) {
		k_sem_give(&pub_key_sem);
	}
}

static void dh_key_ready(const uint8_t key[BT_DH_KEY_LEN])
{
	dh_key_valid = (key != NULL);
	k_sem_give(&dh_key_sem);
}

static void *setup(void)
{
	static struct bt_pub_key_cb pub_key_cb = {
		.func = pub_key_ready,
	};

	zassert_ok(bt_pub_key_gen(&pub_key_cb), "Failed to start the public key generation");
	zassert_ok(k_sem_take(&pub_key_sem, K_SECONDS(10)), "No public key");

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* The local public key is as good a remote key as any */
	(void)memcpy(remote_pk, bt_pub_key_get(), sizeof(remote_pk));
	dh_key_valid = false;
}

ZTEST_SUITE(host_ecc, NULL, setup, before, NULL, NULL);

static ZTEST(host_ecc, test_dh_key)
{
	zassert_ok(bt_dh_key_gen(remote_pk, dh_key_ready), "Failed to start the DH key");
	zassert_ok(k_sem_take(&dh_key_sem, K_SECONDS(10)), "No DH key callback");
	zassert_true(dh_key_valid, "No DH key");
}

/* A requester that gives its request up, as SMP does when the connection goes
 * away or a new pairing starts, may reuse the key right after cancelling: the
 * calculation must not use it then, even though it is a valid key again.
 */
static ZTEST(host_ecc, test_dh_key_cancelled)
{
	/* The calculation runs on a work queue that cannot run meanwhile */
	k_sched_lock();
	zassert_ok(bt_dh_key_gen(remote_pk, dh_key_ready), "Failed to start the DH key");
	bt_dh_key_gen_cancel(remote_pk);
	(void)memcpy(remote_pk, other_pk, sizeof(remote_pk));
	k_sched_unlock();

	zassert_ok(k_sem_take(&dh_key_sem, K_SECONDS(10)), "No DH key callback");
	zassert_false(dh_key_valid, "A cancelled calculation gave a DH key");
}

/* Cancelling with another key leaves the pending calculation alone */
static ZTEST(host_ecc, test_dh_key_cancel_other)
{
	k_sched_lock();
	zassert_ok(bt_dh_key_gen(remote_pk, dh_key_ready), "Failed to start the DH key");
	bt_dh_key_gen_cancel(other_pk);
	k_sched_unlock();

	zassert_ok(k_sem_take(&dh_key_sem, K_SECONDS(10)), "No DH key callback");
	zassert_true(dh_key_valid, "No DH key");
}
