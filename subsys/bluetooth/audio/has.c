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

/* Hearing Access Service GATT Attributes */
BT_GATT_SERVICE_DEFINE(has_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HAS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HAS_HEARING_AID_FEATURES, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_ENCRYPT, read_features, NULL, NULL),
);

static int has_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Initialize the supported features characteristic value */
	has.features = CONFIG_BT_HAS_HEARING_AID_TYPE & BT_HAS_FEAT_HEARING_AID_TYPE_MASK;

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
