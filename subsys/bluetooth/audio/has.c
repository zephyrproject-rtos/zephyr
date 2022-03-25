/*
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <device.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/capabilities.h>

#include "has_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HAS)
#define LOG_MODULE_NAME bt_has
#include "common/log.h"

static struct bt_has has;

static ssize_t read_features(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p offset %d", (void *)conn, attr, offset);

	if (offset > sizeof(has.features)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &has.features,
				 sizeof(has.features));
}

#if CONFIG_BT_HAS_PRESET_COUNT > 0
static uint8_t handle_control_point_op(struct bt_conn *conn, struct net_buf_simple *buf)
{
	const struct bt_has_cp_hdr *hdr;

	hdr = net_buf_simple_pull_mem(buf, sizeof(*hdr));

	BT_DBG("conn %p opcode %s (0x%02x)", (void *)conn, bt_has_op_str(hdr->opcode),
	       hdr->opcode);

	/* TODO: handle request here */

	return BT_HAS_ERR_INVALID_OPCODE;
}

static ssize_t write_control_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				   const void *data, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct net_buf_simple buf;
	uint8_t err;

	BT_DBG("conn %p attr %p data %p len %d offset %d flags 0x%02x", (void *)conn, attr, data,
	       len, offset, flags);

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len == 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&buf, (void *)data, len);

	err = handle_control_point_op(conn, &buf);
	if (err) {
		BT_WARN("err 0x%02x", err);
		return BT_GATT_ERR(err);
	}

	return len;
}

static ssize_t read_active_preset_index(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("conn %p attr %p offset %d", (void *)conn, attr, offset);

	if (offset > sizeof(has.active_index)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &has.active_index,
				 sizeof(has.active_index));
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}
#endif /* CONFIG_BT_HAS_PRESET_COUNT > 0 */

/* Hearing Access Service GATT Attributes */
BT_GATT_SERVICE_DEFINE(has_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_HEARING_AID_FEATURES, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_ENCRYPT, read_features, NULL, NULL),
#if CONFIG_BT_HAS_PRESET_COUNT > 0
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_PRESET_CONTROL_POINT,
#if defined(CONFIG_BT_EATT)
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_NOTIFY,
#else
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_INDICATE,
#endif /* CONFIG_BT_EATT */
			       BT_GATT_PERM_WRITE_ENCRYPT, NULL, write_control_point, NULL),
	BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_ACTIVE_PRESET_INDEX,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_READ_ENCRYPT,
			       read_active_preset_index, NULL, NULL),
	BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT),
#endif /* CONFIG_BT_HAS_PRESET_COUNT > 0 */
);

static int has_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Initialize the supported features characteristic value */
	has.features = CONFIG_BT_HAS_HEARING_AID_TYPE & BT_HAS_FEAT_HEARING_AID_TYPE_MASK;

	if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_BINAURAL)) {
		if (IS_ENABLED(CONFIG_BT_HAS_PRESET_SYNC_SUPPORT)) {
			has.features |= BT_HAS_FEAT_PRESET_SYNC_SUPP;
		}

		if (!IS_ENABLED(CONFIG_BT_HAS_IDENTICAL_PRESET_RECORDS)) {
			has.features |= BT_HAS_FEAT_INDEPENDENT_PRESETS;
		}
	}

	if (IS_ENABLED(CONFIG_BT_HAS_PRESET_NAME_DYNAMIC)) {
		has.features |= BT_HAS_FEAT_WRITABLE_PRESETS_SUPP;
	}

	if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_BANDED)) {
		/* HAP_d1.0r00; 3.7 BAP Unicast Server role requirements
		 * A Banded Hearing Aid in the HA role shall set the Front Left and the Front
		 * Right bits to a value of 0b1 in the Sink Audio Locations characteristic value.
		 */
		bt_audio_capability_set_location(BT_AUDIO_SINK, BT_AUDIO_LOCATION_FRONT_LEFT |
								BT_AUDIO_LOCATION_FRONT_RIGHT);
	} else if (IS_ENABLED(CONFIG_BT_HAS_HEARING_AID_LEFT)) {
		bt_audio_capability_set_location(BT_AUDIO_SINK, BT_AUDIO_LOCATION_FRONT_LEFT);
	} else {
		bt_audio_capability_set_location(BT_AUDIO_SINK, BT_AUDIO_LOCATION_FRONT_RIGHT);
	}

	return 0;
}

SYS_INIT(has_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
