/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include "babblekit/testcase.h"
#include "host/att_internal.h"
#include "audio.h"

static struct bt_aics *aics;
static struct bt_vocs *vocs;
static const struct bt_uuid *const uuids[] = {BT_UUID_AICS_DESCRIPTION, BT_UUID_VOCS_DESCRIPTION};
static atomic_t attempts[2];
static atomic_t accepted[2];
static atomic_t exhausted[2];
static atomic_t received;
static int client_phase;
static bool large_mtu;
static uint16_t handle;
static struct bt_conn *resource_conn;
static struct net_buf *held[CONFIG_BT_ATT_TX_COUNT];
static size_t held_count;

K_SEM_DEFINE(audio_sem, 0, 1);

int __real_bt_gatt_notify_cb(struct bt_conn *conn, struct bt_gatt_notify_params *params);
int __wrap_bt_gatt_notify_cb(struct bt_conn *conn, struct bt_gatt_notify_params *params);

/* Observe the real service's return value without replacing any stack operation. */
int __wrap_bt_gatt_notify_cb(struct bt_conn *conn, struct bt_gatt_notify_params *params)
{
	int err = __real_bt_gatt_notify_cb(conn, params);

	for (size_t i = 0; params->uuid != NULL && i < ARRAY_SIZE(uuids); i++) {
		if (bt_uuid_cmp(params->uuid, uuids[i]) == 0) {
			atomic_inc(&attempts[i]);
			if (err == 0) {
				atomic_inc(&accepted[i]);
			} else {
				TEST_ASSERT(err == -ENOMEM, "Unexpected audio error %d", err);
				atomic_inc(&exhausted[i]);
			}
		}
	}
	return err;
}

static void wait_audio(void)
{
	TEST_ASSERT(k_sem_take(&audio_sem, K_SECONDS(5)) == 0, "Audio operation timed out");
}

void audio_server_init(void)
{
	struct bt_aics_register_param input = {.units = 1, .description = "initial"};
	struct bt_vocs_register_param output = {.output_desc = "initial"};

	aics = bt_aics_free_instance_get();
	vocs = bt_vocs_free_instance_get();
	TEST_ASSERT(aics != NULL && vocs != NULL, "Audio instance allocation failed");
	TEST_ASSERT(bt_aics_register(aics, &input) == 0, "AICS registration failed");
	TEST_ASSERT(bt_vocs_register(vocs, &output) == 0, "VOCS registration failed");
}

static uint8_t discovered(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_chrc *chrc;

	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	TEST_ASSERT(attr != NULL, "Audio characteristic missing");
	chrc = attr->user_data;
	handle = chrc->value_handle;
	k_sem_give(&audio_sem);
	return BT_GATT_ITER_STOP;
}

static void subscribed(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	TEST_ASSERT(err == 0, "Audio subscription failed (%u)", err);
	k_sem_give(&audio_sem);
}

static uint8_t notified(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t len)
{
	const uint8_t *bytes = data;
	uint16_t expected_len = client_phase % 2 == 0 ? 100 : 10;

	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	TEST_ASSERT(data != NULL && client_phase < AUDIO_PHASE_COUNT, "Unexpected audio value");
	TEST_ASSERT(len == expected_len, "Audio value length %u, expected %u", len, expected_len);
	for (size_t i = 0; i < len; i++) {
		TEST_ASSERT(bytes[i] == 'A' + client_phase, "Corrupted audio value");
	}
	TEST_ASSERT(atomic_inc(&received) == 0, "Duplicate audio notification in phase %d",
		    client_phase);
	return BT_GATT_ITER_CONTINUE;
}

void audio_client_subscribe(struct bt_conn *conn, bool large)
{
	static struct bt_gatt_discover_params ccc[2];
	static struct bt_gatt_subscribe_params subscriptions[2];

	large_mtu = large;
	for (size_t i = 0; i < ARRAY_SIZE(uuids); i++) {
		struct bt_gatt_discover_params discover = {
			.uuid = uuids[i],
			.func = discovered,
			.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
			.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
			.type = BT_GATT_DISCOVER_CHARACTERISTIC,
		};

		TEST_ASSERT(bt_gatt_discover(conn, &discover) == 0, "Audio discovery failed");
		wait_audio();
		subscriptions[i] = (struct bt_gatt_subscribe_params){
			.notify = notified,
			.subscribe = subscribed,
			.value_handle = handle,
			.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
			.disc_params = &ccc[i],
			.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
			.value = BT_GATT_CCC_NOTIFY,
		};
		TEST_ASSERT(bt_gatt_subscribe(conn, &subscriptions[i]) == 0,
			    "Audio subscription could not be queued");
		wait_audio();
	}
}

void audio_client_check(int phase)
{
	int expected = large_mtu || phase % 2 != 0;

	TEST_ASSERT(phase == client_phase, "Incorrect audio marker");
	TEST_ASSERT(atomic_get(&received) == expected, "Audio phase %d: expected %d, received %ld",
		    phase, expected, (long)atomic_get(&received));
	atomic_clear(&received);
	client_phase++;
}

static void hold_buffers(struct k_work *work)
{
	ARG_UNUSED(work);
	for (held_count = 0; held_count < ARRAY_SIZE(held); held_count++) {
		held[held_count] = bt_att_create_pdu(resource_conn, BT_ATT_OP_NOTIFY, 0);
		if (held[held_count] == NULL) {
			break;
		}
	}
	TEST_ASSERT(held_count > 0, "No buffers to hold");
	k_sem_give(&audio_sem);
}

void audio_server_run(struct bt_conn *conns[2], void (*checkpoint)(int phase))
{
	struct k_work hold_work;
	char description[101];
	bool any_large = bt_gatt_get_mtu(conns[0]) >= 103 || bt_gatt_get_mtu(conns[1]) >= 103;

	resource_conn = conns[0];
	k_work_init(&hold_work, hold_buffers);
	for (int phase = 0; phase < AUDIO_PHASE_COUNT; phase++) {
		size_t service = phase / 2;
		bool retry = phase % 2 != 0;
		size_t len = retry ? 10 : 100;
		int err;

		if (retry) {
			TEST_ASSERT(k_work_submit(&hold_work) >= 0,
				    "Buffer hold submission failed");
			wait_audio();
		}
		atomic_clear(&attempts[service]);
		atomic_clear(&accepted[service]);
		atomic_clear(&exhausted[service]);
		memset(description, 'A' + phase, len);
		description[len] = '\0';
		err = service == 0 ? bt_aics_description_set(aics, description)
				   : bt_vocs_description_set(vocs, description);
		TEST_ASSERT(err == 0, "Description update failed (%d)", err);
		if (retry) {
			k_sleep(K_MSEC(150));
			TEST_ASSERT(atomic_get(&exhausted[service]) >= 2 &&
					    atomic_get(&accepted[service]) == 0,
				    "Audio did not retry ENOMEM");
			for (size_t i = 0; i < held_count; i++) {
				net_buf_unref(held[i]);
			}
		}
		/* More than ten default retry intervals, including time for radio delivery. */
		k_sleep(K_MSEC(350));
		TEST_PRINT("Audio %s phase %d: attempts %ld, accepted %ld, ENOMEM %ld",
			   service == 0 ? "AICS" : "VOCS", phase,
			   (long)atomic_get(&attempts[service]),
			   (long)atomic_get(&accepted[service]),
			   (long)atomic_get(&exhausted[service]));
		if (retry || any_large) {
			TEST_ASSERT(atomic_get(&accepted[service]) == 1,
				    "Audio must accept one broadcast");
			if (!retry) {
				TEST_ASSERT(atomic_get(&attempts[service]) == 1,
					    "Partial success must not trigger an audio retry");
			}
		} else {
			TEST_ASSERT(atomic_get(&accepted[service]) == 0 &&
					    atomic_get(&exhausted[service]) >= 2,
				    "All MTU failures must retain the existing ENOMEM retry");
		}
		checkpoint(phase);
	}
}
