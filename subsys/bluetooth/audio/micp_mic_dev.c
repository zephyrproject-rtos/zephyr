/*  Bluetooth MICS
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "audio_internal.h"

LOG_MODULE_REGISTER(bt_micp, CONFIG_BT_MICP_MIC_DEV_LOG_LEVEL);

struct bt_micp_server {
	uint8_t mute;
	struct bt_micp_mic_dev_cb *cb;
	struct bt_gatt_service *service_p;
	struct bt_aics *aics_insts[CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT];
	struct k_work_delayable notify_work;
};

static struct bt_micp_server micp_inst;

static void mute_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_mute(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	LOG_DBG("Mute %u", micp_inst.mute);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &micp_inst.mute, sizeof(micp_inst.mute));
}

static void notify_work_handler(struct k_work *work)
{
	struct k_work_delayable *d_work = k_work_delayable_from_work(work);
	struct bt_micp_server *server = CONTAINER_OF(d_work, struct bt_micp_server, notify_work);
	int err;

	err = bt_gatt_notify_uuid(NULL, BT_UUID_MICS_MUTE, server->service_p->attrs, &server->mute,
				  sizeof(server->mute));
	if (err == 0 || err == -ENOTCONN) {
		return;
	}

	if (err == -ENOMEM &&
	    k_work_reschedule(d_work, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US)) >= 0) {
		LOG_WRN("Out of buffers for mute state notification. Will retry in %dms",
			BT_AUDIO_NOTIFY_RETRY_DELAY_US);
		return;
	}

	LOG_ERR("Mute state notification err %d", err);
}

static ssize_t write_mute(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	const uint8_t *val = buf;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(micp_inst.mute)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if ((conn != NULL && *val == BT_MICP_MUTE_DISABLED) ||
	    *val > BT_MICP_MUTE_DISABLED) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (conn != NULL && micp_inst.mute == BT_MICP_MUTE_DISABLED) {
		return BT_GATT_ERR(BT_MICP_ERR_MUTE_DISABLED);
	}

	LOG_DBG("%u", *val);

	if (*val != micp_inst.mute) {
		int err;

		micp_inst.mute = *val;

		err = k_work_reschedule(&micp_inst.notify_work, K_NO_WAIT);
		if (err < 0) {
			LOG_ERR("Failed to schedule mute state notification err %d", err);
		}

		if (micp_inst.cb != NULL && micp_inst.cb->mute != NULL) {
			micp_inst.cb->mute(micp_inst.mute);
		}
	}

	return len;
}


#define DUMMY_INCLUDE(i, _) BT_GATT_INCLUDE_SERVICE(NULL),
#define AICS_INCLUDES(cnt) LISTIFY(cnt, DUMMY_INCLUDE, ())

#define BT_MICP_SERVICE_DEFINITION \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_MICS), \
	AICS_INCLUDES(CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT) \
	BT_AUDIO_CHRC(BT_UUID_MICS_MUTE, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT, \
		      read_mute, write_mute, NULL), \
	BT_AUDIO_CCC(mute_cfg_changed)

#define MICS_ATTR_COUNT \
	ARRAY_SIZE(((struct bt_gatt_attr []){ BT_MICP_SERVICE_DEFINITION }))
#define MICS_INCL_COUNT (CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT)

static struct bt_gatt_attr mics_attrs[] = { BT_MICP_SERVICE_DEFINITION };
static struct bt_gatt_service mics_svc;

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
static int prepare_aics_inst(struct bt_micp_mic_dev_register_param *param)
{
	int i;
	int j;
	int err;

	if (CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT == 0) {
		return 0;
	}

	for (j = 0, i = 0; i < ARRAY_SIZE(mics_attrs); i++) {
		if (bt_uuid_cmp(mics_attrs[i].uuid, BT_UUID_GATT_INCLUDE) == 0) {
			micp_inst.aics_insts[j] = bt_aics_free_instance_get();
			if (micp_inst.aics_insts[j] == NULL) {
				LOG_DBG("Could not get free AICS instances[%u]", j);
				return -ENOMEM;
			}

			err = bt_aics_register(micp_inst.aics_insts[j],
					       &param->aics_param[j]);
			if (err != 0) {
				LOG_DBG("Could not register AICS instance[%u]: %d", j, err);
				return err;
			}

			mics_attrs[i].user_data = bt_aics_svc_decl_get(micp_inst.aics_insts[j]);
			j++;

			if (j == CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT) {
				break;
			}
		}
	}

	__ASSERT(j == CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT,
		 "Invalid AICS instance count");

	return 0;
}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

/****************************** PUBLIC API ******************************/
int bt_micp_mic_dev_register(struct bt_micp_mic_dev_register_param *param)
{
	int err;
	static bool registered;

	if (registered) {
		return -EALREADY;
	}

	__ASSERT(param, "MICS register parameter cannot be NULL");

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
	err = prepare_aics_inst(param);
	if (err != 0) {
		LOG_DBG("Failed to prepare AICS instances: %d", err);

		return err;
	}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

	mics_svc = (struct bt_gatt_service)BT_GATT_SERVICE(mics_attrs);
	micp_inst.service_p = &mics_svc;
	err = bt_gatt_service_register(&mics_svc);

	if (err != 0) {
		LOG_ERR("MICS service register failed: %d", err);
	}

	micp_inst.cb = param->cb;

	k_work_init_delayable(&micp_inst.notify_work, notify_work_handler);

	registered = true;

	return err;
}

int bt_micp_mic_dev_mute_disable(void)
{
	uint8_t val = BT_MICP_MUTE_DISABLED;
	int err;

	err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_micp_mic_dev_included_get(struct bt_micp_included *included)
{
	CHECKIF(included == NULL) {
		LOG_DBG("NULL service pointer");
		return -EINVAL;
	}

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
	included->aics_cnt = ARRAY_SIZE(micp_inst.aics_insts);
	included->aics = micp_inst.aics_insts;
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

	return 0;
}

int bt_micp_mic_dev_unmute(void)
{
	const uint8_t val = BT_MICP_MUTE_UNMUTED;
	const int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_micp_mic_dev_mute(void)
{
	const uint8_t val = BT_MICP_MUTE_MUTED;
	const int err = write_mute(NULL, NULL, &val, sizeof(val), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_micp_mic_dev_mute_get(void)
{
	if (micp_inst.cb && micp_inst.cb->mute) {
		micp_inst.cb->mute(micp_inst.mute);
	}

	return 0;
}
