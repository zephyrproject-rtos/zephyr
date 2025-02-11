/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/psa/key_ids.h>
#include "argparse.h"
#include "mesh/crypto.h"

#define LOG_MODULE_NAME distribute_keys
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Mesh requires to keep in persistent memory network keys (2 keys per subnetwork),
 * application keys (2 real keys per 1 configured) and device key + device key candidate.
 */
#if defined CONFIG_BT_MESH_CDB
#define BT_MESH_CDB_KEY_ID_RANGE_SIZE (2 * SUBNET_COUNT + \
		2 * APP_KEY_COUNT + NODE_COUNT)
#else
#define BT_MESH_CDB_KEY_ID_RANGE_SIZE  0
#endif

#define BT_MESH_PSA_KEY_ID_RANGE_SIZE (2 * CONFIG_BT_MESH_SUBNET_COUNT + \
		2 * CONFIG_BT_MESH_APP_KEY_COUNT + 2 + BT_MESH_CDB_KEY_ID_RANGE_SIZE)
#define BT_MESH_TEST_PSA_KEY_ID_MIN (ZEPHYR_PSA_BT_MESH_KEY_ID_RANGE_BEGIN + \
		BT_MESH_PSA_KEY_ID_RANGE_SIZE * get_device_nbr())

static ATOMIC_DEFINE(pst_keys, BT_MESH_PSA_KEY_ID_RANGE_SIZE);

psa_key_id_t bt_mesh_user_keyid_alloc(void)
{
	for (int i = 0; i < BT_MESH_PSA_KEY_ID_RANGE_SIZE; i++) {
		if (!atomic_test_bit(pst_keys, i)) {
			atomic_set_bit(pst_keys, i);

			LOG_INF("key id %d is allocated", BT_MESH_TEST_PSA_KEY_ID_MIN + i);

			return BT_MESH_TEST_PSA_KEY_ID_MIN + i;
		}
	}

	return PSA_KEY_ID_NULL;
}

int bt_mesh_user_keyid_free(psa_key_id_t key_id)
{
	if (IN_RANGE(key_id, BT_MESH_TEST_PSA_KEY_ID_MIN,
			BT_MESH_TEST_PSA_KEY_ID_MIN + BT_MESH_PSA_KEY_ID_RANGE_SIZE - 1)) {
		atomic_clear_bit(pst_keys, key_id - BT_MESH_TEST_PSA_KEY_ID_MIN);

		LOG_INF("key id %d is freed", key_id);

		return 0;
	}

	return -EIO;
}

void bt_mesh_user_keyid_assign(psa_key_id_t key_id)
{
	if (IN_RANGE(key_id, BT_MESH_TEST_PSA_KEY_ID_MIN,
			BT_MESH_TEST_PSA_KEY_ID_MIN + BT_MESH_PSA_KEY_ID_RANGE_SIZE - 1)) {
		atomic_set_bit(pst_keys, key_id - BT_MESH_TEST_PSA_KEY_ID_MIN);
		LOG_INF("key id %d is assigned", key_id);
	} else {
		LOG_WRN("key id %d is out of the reserved id range", key_id);
	}
}
