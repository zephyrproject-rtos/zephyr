/*  Bluetooth MICS
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/check.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/mics.h>
#include <bluetooth/audio/aics.h>

#include "mics_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MICS)
#define LOG_MODULE_NAME bt_mics
#include "common/log.h"

#if defined(CONFIG_BT_MICS)

static struct bt_mics_server mics_inst;

static void mute_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_mute(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	BT_DBG("Mute %u", mics_inst.mute);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &mics_inst.mute, sizeof(mics_inst.mute));
}

static ssize_t write_mute(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	const uint8_t *val = buf;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(mics_inst.mute)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if ((conn != NULL && *val == BT_MICS_MUTE_DISABLED) ||
	    *val > BT_MICS_MUTE_DISABLED) {
		return BT_GATT_ERR(BT_MICS_ERR_VAL_OUT_OF_RANGE);
	}

	if (conn != NULL && mics_inst.mute == BT_MICS_MUTE_DISABLED) {
		return BT_GATT_ERR(BT_MICS_ERR_MUTE_DISABLED);
	}

	BT_DBG("%u", *val);

	if (*val != mics_inst.mute) {
		mics_inst.mute = *val;

		bt_gatt_notify_uuid(NULL, BT_UUID_MICS_MUTE,
				    mics_inst.service_p->attrs,
				    &mics_inst.mute, sizeof(mics_inst.mute));

		if (mics_inst.cb != NULL && mics_inst.cb->mute != NULL) {
			mics_inst.cb->mute(NULL, 0, mics_inst.mute);
		}
	}

	return len;
}


#define DUMMY_INCLUDE(i, _) BT_GATT_INCLUDE_SERVICE(NULL),
#define AICS_INCLUDES(cnt) UTIL_LISTIFY(cnt, DUMMY_INCLUDE)

#define BT_MICS_SERVICE_DEFINITION \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MICS), \
	AICS_INCLUDES(CONFIG_BT_MICS_AICS_INSTANCE_COUNT) \
	BT_GATT_CHARACTERISTIC(BT_UUID_MICS_MUTE, \
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY, \
		BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		read_mute, write_mute, NULL), \
	BT_GATT_CCC(mute_cfg_changed, \
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT)

#define MICS_ATTR_COUNT \
	ARRAY_SIZE(((struct bt_gatt_attr []){ BT_MICS_SERVICE_DEFINITION }))
#define MICS_INCL_COUNT (CONFIG_BT_MICS_AICS_INSTANCE_COUNT)

static struct bt_gatt_attr mics_attrs[] = { BT_MICS_SERVICE_DEFINITION };
static struct bt_gatt_service mics_svc;

static int prepare_aics_inst(struct bt_mics_register_param *param)
{
	int i;
	int j;
	int err;

	for (j = 0, i = 0; i < ARRAY_SIZE(mics_attrs); i++) {
		if (bt_uuid_cmp(mics_attrs[i].uuid, BT_UUID_GATT_INCLUDE) == 0) {
			mics_inst.aics_insts[j] = bt_aics_free_instance_get();
			if (mics_inst.aics_insts[j] == NULL) {
				BT_DBG("Could not get free AICS instances[%u]", j);
				return -ENOMEM;
			}

			err = bt_aics_register(mics_inst.aics_insts[j],
					       &param->aics_param[j]);
			if (err != 0) {
				BT_DBG("Could not register AICS instance[%u]: %d", j, err);
				return err;
			}

			mics_attrs[i].user_data = bt_aics_svc_decl_get(mics_inst.aics_insts[j]);
			j++;

			if (j == CONFIG_BT_MICS_AICS_INSTANCE_COUNT) {
				break;
			}
		}
	}

	__ASSERT(j == CONFIG_BT_MICS_AICS_INSTANCE_COUNT,
		 "Invalid AICS instance count");

	return 0;
}

/****************************** PUBLIC API ******************************/
int bt_mics_register(struct bt_mics_register_param *param,
		     struct bt_mics **mics)
{
	int err;

	__ASSERT(param, "MICS register parameter cannot be NULL");

	if (CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0) {
		prepare_aics_inst(param);
	}

	mics_svc = (struct bt_gatt_service)BT_GATT_SERVICE(mics_attrs);
	mics_inst.service_p = &mics_svc;
	err = bt_gatt_service_register(&mics_svc);

	if (err != 0) {
		BT_ERR("MICS service register failed: %d", err);
	}

	mics_inst.cb = param->cb;

	*mics = (struct bt_mics *)&mics_inst;

	return err;
}

int bt_mics_aics_deactivate(struct bt_aics *inst)
{
	CHECKIF(inst == NULL) {
		return -EINVAL;
	}

	if (CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0) {
		return bt_aics_deactivate(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_activate(struct bt_aics *inst)
{
	CHECKIF(inst == NULL) {
		return -EINVAL;
	}

	if (CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0) {
		return bt_aics_activate(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_mute_disable(void)
{
	uint8_t val = BT_MICS_MUTE_DISABLED;
	int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

	return err > 0 ? 0 : err;
}

#endif /* CONFIG_BT_MICS */

static bool valid_aics_inst(struct bt_aics *aics)
{
	if (aics == NULL) {
		return false;
	}

#if defined(CONFIG_BT_MICS)
	for (int i = 0; i < ARRAY_SIZE(mics_inst.aics_insts); i++) {
		if (mics_inst.aics_insts[i] == aics) {
			return true;
		}
	}
#endif /* CONFIG_BT_MICS */
	return false;
}

int bt_mics_included_get(struct bt_conn *conn,
			 struct bt_mics_included *included)
{
	CHECKIF(included == NULL) {
		BT_DBG("NULL service pointer");
		return -EINVAL;
	}

#if defined(CONFIG_BT_MICS_CLIENT)
	if (conn != NULL) {
		return bt_mics_client_included_get(conn, included);
	}
#endif /* CONFIG_BT_MICS_CLIENT */

#if defined(CONFIG_BT_MICS)
	if (conn == NULL) {
		included->aics_cnt = ARRAY_SIZE(mics_inst.aics_insts);
		included->aics = mics_inst.aics_insts;

		return 0;
	}
#endif /* CONFIG_BT_MICS */
	return -EOPNOTSUPP;
}

int bt_mics_unmute(struct bt_conn *conn)
{
#if defined(CONFIG_BT_MICS_CLIENT)
	if (conn != NULL) {
		return bt_mics_client_unmute(conn);
	}
#endif /* CONFIG_BT_MICS_CLIENT */

#if defined(CONFIG_BT_MICS)
	if (conn == NULL) {
		uint8_t val = BT_MICS_MUTE_UNMUTED;
		int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

		return err > 0 ? 0 : err;
	}
#endif /* CONFIG_BT_MICS */
	return -EOPNOTSUPP;
}

int bt_mics_mute(struct bt_conn *conn)
{
	if (conn != NULL) {
		if (IS_ENABLED(CONFIG_BT_MICS_CLIENT)) {
			return bt_mics_client_mute(conn);
		} else {
			return -EOPNOTSUPP;
		}
	}

#if defined(CONFIG_BT_MICS)
	uint8_t val = BT_MICS_MUTE_MUTED;
	int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_MICS */
}

int bt_mics_mute_get(struct bt_conn *conn)
{
	if (conn != NULL) {
		if (IS_ENABLED(CONFIG_BT_MICS_CLIENT)) {
			return bt_mics_client_mute_get(conn);
		} else {
			return -EOPNOTSUPP;
		}
	}

#if defined(CONFIG_BT_MICS)
	if (mics_inst.cb && mics_inst.cb->mute) {
		mics_inst.cb->mute(NULL, 0, mics_inst.mute);
	}

	return 0;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_MICS */
}

int bt_mics_aics_state_get(struct bt_conn *conn, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_state_get(conn, inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_state_get(NULL, inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_gain_setting_get(struct bt_conn *conn, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_gain_setting_get(conn, inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_gain_setting_get(NULL, inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_type_get(struct bt_conn *conn, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_type_get(conn, inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_type_get(NULL, inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_status_get(struct bt_conn *conn, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_status_get(conn, inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_status_get(NULL, inst);
	}

	return -EOPNOTSUPP;
}
int bt_mics_aics_unmute(struct bt_conn *conn, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_unmute(conn, inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_unmute(NULL, inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_mute(struct bt_conn *conn, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_mute(conn, inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_mute(NULL, inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_manual_gain_set(struct bt_conn *conn, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_manual_gain_set(conn, inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_manual_gain_set(NULL, inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_automatic_gain_set(struct bt_conn *conn, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_automatic_gain_set(conn, inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_automatic_gain_set(NULL, inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_gain_set(struct bt_conn *conn, struct bt_aics *inst,
			  int8_t gain)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_gain_set(conn, inst, gain);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_gain_set(NULL, inst, gain);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_description_get(struct bt_conn *conn, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_description_get(conn, inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_description_get(NULL, inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_description_set(struct bt_conn *conn, struct bt_aics *inst,
				 const char *description)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    conn != NULL &&
	    bt_mics_client_valid_aics_inst(conn, inst)) {
		return bt_aics_description_set(conn, inst, description);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    conn == NULL &&
	    valid_aics_inst(inst)) {
		return bt_aics_description_set(NULL, inst, description);
	}

	return -EOPNOTSUPP;
}
