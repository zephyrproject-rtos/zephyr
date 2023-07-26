/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/audio/hap.h>
#include <zephyr/bluetooth/audio/has.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/check.h>

#include "hap_harc_internal.h"
#include "has_internal.h"

#include <zephyr/logging/log.h>

#define HAP_MAX_SET_SIZE 2

LOG_MODULE_REGISTER(bt_hap_harc, CONFIG_BT_HAP_HARC_LOG_LEVEL);

static struct bt_hap_harc {
	struct bt_has *has;
	union {
		struct hap_ha_binaural binaural;
		struct hap_ha_monaural monaural;
		struct hap_ha_banded banded;
	};

	/* Pending operation context data */
	bt_hap_harc_complete_func_t func;
	void *user_data;
} harc_pool[CONFIG_BT_HAP_HARC_MAX];

const struct bt_hap_harc_cb *hap_cb;

static inline bool is_binaural(struct bt_has *has)
{
	return has_type_get(has) == BT_HAS_HEARING_AID_TYPE_BINAURAL;
}

static struct bt_hap_harc *harc_lookup_conn(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(ha_sets)) {
		if (ha_sets[i].ha_cnt == 0) {
			return &ha_sets[i];
		}
	}
}

static void set_free(struct bt_hap_ha_set *inst)
{

}

static struct bt_hap_harc *harc_get_or_new(struct bt_conn *conn)
{

}

static void csip_discover_cb(struct bt_conn *conn,
			     const struct bt_csip_set_coordinator_set_member *member,
			     int err, size_t set_count)
{

}

static struct bt_csip_set_coordinator_cb csip_cb = {
	.discover = csip_discover_cb,
};

static void has_discover_cb(struct bt_conn *conn, int err, struct bt_has *has,
			    enum bt_has_hearing_aid_type type, enum bt_has_capabilities caps)
{
	if (err != 0) {
		hap_cb->discover(conn, err);
		return;
	}

	if (type == BT_HAS_HEARING_AID_TYPE_BINAURAL) {
		err = bt_csip_set_coordinator_discover(conn);
		if (err < 0) {
			LOG_ERR("Failed to discover CSIS err %d", err);
			hap_cb->discover(conn, err);
		}
	} else {
		err = bt_vcp_vol_ctlr_discover(conn, struct bt_vcp_vol_ctlr **vol_ctlr)
	}
}

static struct bt_has_client_cb has_cb = {
	.discover = has_discover_cb,
};

int bt_hap_harc_init(struct bt_hap_harc_cb *cb)
{
	int err;

	CHECKIF(cb == NULL || cb->active_preset == NULL || cb->vol_ctrl_cb == NULL) {
		return -EINVAL;
	}

	CHECKIF(hap_cb) {
		return -EALREADY;
	}

	err = bt_csip_set_coordinator_register_cb(&csip_cb);
	if (err < 0) {
		LOG_ERR("Failed to register CSIP Set Coordinator callbacks err %d", err);
		return err;
	}

	err = bt_vcp_vol_ctlr_cb_register(cb->vol_ctrl_cb);
	if (err < 0) {
		LOG_ERR("Failed to register VCP Volume Controller callbacks err %d", err);
		return err;
	}

	err = bt_has_client_cb_register(&has_cb);
	if (err < 0) {
		LOG_ERR("Failed to register HAS Client callbacks err %d", err);
		return err;
	}

	hap_cb = cb;

	return 0;
}

int bt_hap_harc_discover(struct bt_conn *conn)
{
	struct bt_hap_harc *harc;
	int err;

	CHECKIF(conn == NULL) {
		return -EINVAL;
	}

	harc = harc_get_or_new(conn);
	if (harc == NULL) {
		return -ENOMEM;
	}

	err = bt_has_client_discover(conn);
	if (err < 0) {
		LOG_ERR("Failed to discover HAS err %d", err);
		return err;
	}

	return 0;
}

typedef void (*harc_foreach_func)(struct bt_hap_harc *harc, void *user_data);

static void harc_foreach(harc_foreach_func func, void *user_data)
{

}

static bool shall_sync_presets(struct bt_hap_harc *harc)
{
	return is_binaural(harc->has) && !has_is_preset_list_independent(harc->has);
}

static struct bt_hap_harc *harc_get_pair(struct bt_hap_harc *harc)
{
	__ASSERT_NO_MSG(is_binaural(harc->has));

	/* TODO */

	return NULL;
}

static int harc_busy_set(struct bt_hap_harc *harc, bt_hap_harc_complete_func_t func,
				  void *user_data)
{
	if (harc->func != NULL) {
		return -EBUSY;
	}

	harc->func = func;
	harc->user_data = user_data;

	return 0;
}

static void harc_busy_clear(struct bt_hap_harc *harc)
{
	harc->func = NULL;
}

static void harc_func_complete(struct bt_hap_harc *harc, int err)
{

}

static void preset_next_intermediate_complete(struct bt_conn *conn, int err, void *user_data)
{
	struct bt_hap_harc *harc, *pair;

	harc = harc_lookup_conn(conn);
	if (harc == NULL) {
		return;
	}

	pair = user_data;
	__ASSERT_NO_MSG(pair != NULL);

	/* The operation is complete as HA will synchronize the preset with paired HA */
	if (err != 0 || has_is_preset_oob_sync_supported(harc->has)) {
		harc_func_complete(pair, err);
		return;
	}

	/* Process the paired device */
	err = bt_has_client_preset_prev(pair->has, false);
	if (err != 0) {
		LOG_ERR("Failed to set previous preset err %d", err);
		harc_func_complete(pair, err);
	}
}

static int hap_harc_preset_next_sync(struct bt_hap_harc *harc, bt_hap_harc_complete_func_t func,
				     void *user_data)
{
	struct bt_hap_harc *pair;
	int err;

	__ASSERT_NO_MSG(is_binaural(harc->has));

	if (has_is_preset_oob_sync_supported(harc->has)) {
		return bt_has_client_preset_next(harc->has, true);
	}

	pair = harc_get_pair(harc);
	if (pair == NULL) {
		/* The paired device is not connected, so proceed with the current one only */
		return bt_has_client_preset_next(harc->has, false);
	}

	/* Set the internal complete callback to handle the intermediate operation complete state */
	err = harc_busy_set(pair, preset_next_intermediate_complete, harc);
	if (err != 0) {
		return err;
	}

	/* Process with the paired device first */
	err = bt_has_client_preset_next(pair->has, has_is_preset_oob_sync_supported(pair->has));
	if (err != 0) {
		harc_busy_clear(pair);
		return err;
	}

	return 0;
}

int bt_hap_harc_preset_next(struct bt_conn *conn, bt_hap_harc_complete_func_t func,
			    void *user_data)
{
	struct bt_hap_harc *harc;
	int err;

	CHECKIF(conn == NULL) {
		return -EINVAL;
	}

	harc = harc_lookup_conn(conn);
	if (harc == NULL) {
		return -ENODEV;
	}

	err = harc_busy_set(harc, func, user_data);
	if (err != 0) {
		return err;
	}

	if (shall_sync_presets(harc)) {
		err = hap_harc_preset_next_sync(harc, func, user_data);
	} else {
		err = bt_has_client_preset_next(harc->has, false);
	}

	if (err != 0) {
		harc_busy_clear(harc);
		return err;
	}

	return 0;
}

static void preset_prev_intermediate_complete(struct bt_conn *conn, int err, void *user_data)
{
	struct bt_hap_harc *pair, *harc;

	pair = harc_lookup_conn(conn);
	if (pair == NULL) {
		return;
	}

	harc = user_data;
	__ASSERT_NO_MSG(harc != NULL);

	/* The operation is complete as HA will synchronize the preset with paired HA */
	if (err != 0 || has_is_preset_oob_sync_supported(pair->has)) {
		harc_func_complete(harc, err);
		return;
	}

	/* Process the paired device */
	err = bt_has_client_preset_prev(harc->has, false);
	if (err != 0) {
		LOG_ERR("Failed to set previous preset err %d", err);
		harc_func_complete(harc, err);
	}
}

static int hap_harc_preset_prev_sync(struct bt_hap_harc *harc, bt_hap_harc_complete_func_t func,
				     void *user_data)
{
	struct bt_hap_harc *pair;
	int err;

	__ASSERT_NO_MSG(is_binaural(harc->has));

	if (has_is_preset_oob_sync_supported(harc->has)) {
		return bt_has_client_preset_prev(harc->has, true);
	}

	pair = harc_get_pair(harc);
	if (pair == NULL) {
		/* The paired device is not connected, so proceed with the current one only */
		return bt_has_client_preset_prev(harc->has, false);
	}

	/* Set the internal complete callback to handle the intermediate operation complete state */
	err = harc_busy_set(pair, preset_prev_intermediate_complete, harc);
	if (err != 0) {
		return err;
	}

	/* Process with the paired device first */
	err = bt_has_client_preset_prev(pair->has, has_is_preset_oob_sync_supported(pair->has));
	if (err != 0) {
		harc_busy_clear(pair);
		return err;
	}

	return 0;
}

int bt_hap_harc_preset_prev(struct bt_conn *conn, bt_hap_harc_complete_func_t func,
			    void *user_data)
{
	struct bt_hap_harc *harc;
	int err;

	CHECKIF(conn == NULL) {
		return -EINVAL;
	}

	harc = harc_lookup_conn(conn);
	if (harc == NULL) {
		return -ENODEV;
	}

	err = harc_busy_set(harc, func, user_data);
	if (err != 0) {
		return err;
	}

	if (shall_sync_presets(harc)) {
		err = hap_harc_preset_prev_sync(harc, func, user_data);
	} else {
		err = bt_has_client_preset_prev(harc->has, false);
	}

	if (err != 0) {
		harc_busy_clear(harc);
		return err;
	}

	return 0;
}
