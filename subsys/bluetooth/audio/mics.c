/*  Bluetooth MICS
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/mics.h>
#include <zephyr/bluetooth/audio/aics.h>

#include "mics_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MICS)
#define LOG_MODULE_NAME bt_mics
#include "common/log.h"

#if defined(CONFIG_BT_MICS)

static struct bt_mics mics_inst;

static void mute_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_mute(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	BT_DBG("Mute %u", mics_inst.srv.mute);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &mics_inst.srv.mute, sizeof(mics_inst.srv.mute));
}

static ssize_t write_mute(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	const uint8_t *val = buf;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(mics_inst.srv.mute)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if ((conn != NULL && *val == BT_MICS_MUTE_DISABLED) ||
	    *val > BT_MICS_MUTE_DISABLED) {
		return BT_GATT_ERR(BT_MICS_ERR_VAL_OUT_OF_RANGE);
	}

	if (conn != NULL && mics_inst.srv.mute == BT_MICS_MUTE_DISABLED) {
		return BT_GATT_ERR(BT_MICS_ERR_MUTE_DISABLED);
	}

	BT_DBG("%u", *val);

	if (*val != mics_inst.srv.mute) {
		mics_inst.srv.mute = *val;

		bt_gatt_notify_uuid(NULL, BT_UUID_MICS_MUTE,
				    mics_inst.srv.service_p->attrs,
				    &mics_inst.srv.mute, sizeof(mics_inst.srv.mute));

		if (mics_inst.srv.cb != NULL && mics_inst.srv.cb->mute != NULL) {
			mics_inst.srv.cb->mute(NULL, 0, mics_inst.srv.mute);
		}
	}

	return len;
}


#define DUMMY_INCLUDE(i, _) BT_GATT_INCLUDE_SERVICE(NULL),
#define AICS_INCLUDES(cnt) LISTIFY(cnt, DUMMY_INCLUDE, ())

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
			mics_inst.srv.aics_insts[j] = bt_aics_free_instance_get();
			if (mics_inst.srv.aics_insts[j] == NULL) {
				BT_DBG("Could not get free AICS instances[%u]", j);
				return -ENOMEM;
			}

			err = bt_aics_register(mics_inst.srv.aics_insts[j],
					       &param->aics_param[j]);
			if (err != 0) {
				BT_DBG("Could not register AICS instance[%u]: %d", j, err);
				return err;
			}

			mics_attrs[i].user_data = bt_aics_svc_decl_get(mics_inst.srv.aics_insts[j]);
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
	static bool registered;

	if (registered) {
		*mics = &mics_inst;
		return -EALREADY;
	}

	__ASSERT(param, "MICS register parameter cannot be NULL");

	if (CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0) {
		prepare_aics_inst(param);
	}

	mics_svc = (struct bt_gatt_service)BT_GATT_SERVICE(mics_attrs);
	mics_inst.srv.service_p = &mics_svc;
	err = bt_gatt_service_register(&mics_svc);

	if (err != 0) {
		BT_ERR("MICS service register failed: %d", err);
	}

	mics_inst.srv.cb = param->cb;

	*mics = &mics_inst;
	registered = true;

	return err;
}

int bt_mics_aics_deactivate(struct bt_mics *mics, struct bt_aics *inst)
{
	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics");
		return -EINVAL;
	}

	CHECKIF(inst == NULL) {
		return -EINVAL;
	}

	if (mics->client_instance) {
		BT_DBG("Can only deactivate AICS on a server instance");
		return -EINVAL;
	}

	if (CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0) {
		return bt_aics_deactivate(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_activate(struct bt_mics *mics, struct bt_aics *inst)
{
	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics");
		return -EINVAL;
	}

	CHECKIF(inst == NULL) {
		return -EINVAL;
	}

	if (mics->client_instance) {
		BT_DBG("Can only activate AICS on a server instance");
		return -EINVAL;
	}

	if (CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0) {
		return bt_aics_activate(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_mute_disable(struct bt_mics *mics)
{
	uint8_t val = BT_MICS_MUTE_DISABLED;
	int err;

	if (mics->client_instance) {
		BT_DBG("Can only disable mute on a server instance");
		return -EINVAL;
	}

	err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

	return err > 0 ? 0 : err;
}

#endif /* CONFIG_BT_MICS */

static bool valid_aics_inst(struct bt_mics *mics, struct bt_aics *aics)
{
	if (mics == NULL) {
		return false;
	}

	if (aics == NULL) {
		return false;
	}

	if (mics->client_instance) {
		return false;
	}

#if defined(CONFIG_BT_MICS)
	for (int i = 0; i < ARRAY_SIZE(mics_inst.srv.aics_insts); i++) {
		if (mics_inst.srv.aics_insts[i] == aics) {
			return true;
		}
	}
#endif /* CONFIG_BT_MICS */
	return false;
}

int bt_mics_included_get(struct bt_mics *mics,
			 struct bt_mics_included *included)
{
	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics pointer");
		return -EINVAL;
	}

	CHECKIF(included == NULL) {
		BT_DBG("NULL service pointer");
		return -EINVAL;
	}


	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT) && mics->client_instance) {
		return bt_mics_client_included_get(mics, included);
	}

#if defined(CONFIG_BT_MICS)
	included->aics_cnt = ARRAY_SIZE(mics_inst.srv.aics_insts);
	included->aics = mics_inst.srv.aics_insts;

	return 0;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_MICS */
}

int bt_mics_unmute(struct bt_mics *mics)
{
	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT) && mics->client_instance) {
		return bt_mics_client_unmute(mics);
	}

#if defined(CONFIG_BT_MICS)
	uint8_t val = BT_MICS_MUTE_UNMUTED;
	int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_MICS */
}

int bt_mics_mute(struct bt_mics *mics)
{
	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT) && mics->client_instance) {
		return bt_mics_client_mute(mics);
	}

#if defined(CONFIG_BT_MICS)
	uint8_t val = BT_MICS_MUTE_MUTED;
	int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_MICS */
}

int bt_mics_mute_get(struct bt_mics *mics)
{
	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT) && mics->client_instance) {
		return bt_mics_client_mute_get(mics);
	}

#if defined(CONFIG_BT_MICS)
	if (mics_inst.srv.cb && mics_inst.srv.cb->mute) {
		mics_inst.srv.cb->mute(NULL, 0, mics_inst.srv.mute);
	}

	return 0;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_MICS */
}

int bt_mics_aics_state_get(struct bt_mics *mics, struct bt_aics *inst)
{
	CHECKIF(mics == NULL) {
		BT_DBG("NULL mics pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_state_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_state_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_gain_setting_get(struct bt_mics *mics, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_gain_setting_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_gain_setting_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_type_get(struct bt_mics *mics, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_type_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_type_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_status_get(struct bt_mics *mics, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_status_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_status_get(inst);
	}

	return -EOPNOTSUPP;
}
int bt_mics_aics_unmute(struct bt_mics *mics, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_unmute(inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_unmute(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_mute(struct bt_mics *mics, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_mute(inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_mute(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_manual_gain_set(struct bt_mics *mics, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_manual_gain_set(inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_manual_gain_set(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_automatic_gain_set(struct bt_mics *mics, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_automatic_gain_set(inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_automatic_gain_set(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_gain_set(struct bt_mics *mics, struct bt_aics *inst,
			  int8_t gain)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_gain_set(inst, gain);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_gain_set(inst, gain);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_description_get(struct bt_mics *mics, struct bt_aics *inst)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_description_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_description_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_mics_aics_description_set(struct bt_mics *mics, struct bt_aics *inst,
				 const char *description)
{
	if (IS_ENABLED(CONFIG_BT_MICS_CLIENT_AICS) &&
	    bt_mics_client_valid_aics_inst(mics, inst)) {
		return bt_aics_description_set(inst, description);
	}

	if (IS_ENABLED(CONFIG_BT_MICS_AICS) &&
	    valid_aics_inst(mics, inst)) {
		return bt_aics_description_set(inst, description);
	}

	return -EOPNOTSUPP;
}
