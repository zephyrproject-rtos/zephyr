/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/gmap.h>
#include <zephyr/sys/check.h>

#include <zephyr/logging/log.h>

#include "audio_internal.h"

LOG_MODULE_REGISTER(bt_gmap_server, CONFIG_BT_GMAP_LOG_LEVEL);

#define BT_GMAP_ROLE_MASK                                                                          \
	(BT_GMAP_ROLE_UGG | BT_GMAP_ROLE_UGT | BT_GMAP_ROLE_BGS | BT_GMAP_ROLE_BGR)

static uint8_t gmap_role;
static struct bt_gmap_feat gmap_features;

static ssize_t read_gmap_role(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	LOG_DBG("role 0x%02X", gmap_role);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &gmap_role, sizeof(gmap_role));
}

#if defined(CONFIG_BT_GMAP_UGG_SUPPORTED)
static ssize_t read_gmap_ugg_feat(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	const uint8_t feat = (uint8_t)gmap_features.ugg_feat;

	LOG_DBG("feat 0x%02X", feat);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &feat, sizeof(feat));
}

static const struct bt_gatt_attr ugg_feat_chrc[] = {
	BT_AUDIO_CHRC(BT_UUID_GMAP_UGG_FEAT, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
		      read_gmap_ugg_feat, NULL, NULL),
};
#endif /* CONFIG_BT_GMAP_UGG_SUPPORTED */

#if defined(CONFIG_BT_GMAP_UGT_SUPPORTED)
static ssize_t read_gmap_ugt_feat(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	const uint8_t feat = (uint8_t)gmap_features.ugt_feat;

	LOG_DBG("feat 0x%02X", feat);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &feat, sizeof(feat));
}

static const struct bt_gatt_attr ugt_feat_chrc[] = {
	BT_AUDIO_CHRC(BT_UUID_GMAP_UGT_FEAT, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
		      read_gmap_ugt_feat, NULL, NULL),
};

#endif /* CONFIG_BT_GMAP_UGT_SUPPORTED */

#if defined(CONFIG_BT_GMAP_BGS_SUPPORTED)
static ssize_t read_gmap_bgs_feat(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	const uint8_t feat = (uint8_t)gmap_features.bgs_feat;

	LOG_DBG("feat 0x%02X", feat);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &feat, sizeof(feat));
}

static const struct bt_gatt_attr bgs_feat_chrc[] = {
	BT_AUDIO_CHRC(BT_UUID_GMAP_BGS_FEAT, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
		      read_gmap_bgs_feat, NULL, NULL),
};
#endif /* CONFIG_BT_GMAP_BGS_SUPPORTED */

#if defined(CONFIG_BT_GMAP_BGR_SUPPORTED)
static ssize_t read_gmap_bgr_feat(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	const uint8_t feat = (uint8_t)gmap_features.bgr_feat;

	LOG_DBG("feat 0x%02X", feat);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &feat, sizeof(feat));
}

static const struct bt_gatt_attr bgr_feat_chrc[] = {
	BT_AUDIO_CHRC(BT_UUID_GMAP_BGR_FEAT, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
		      read_gmap_bgr_feat, NULL, NULL),
};
#endif /* CONFIG_BT_GMAP_BGR_SUPPORTED */

/* There are 4 optional characteristics - Use a dummy definition to allocate memory and then add the
 * characteristics at will when registering or modifying the role(s)
 */
#define GMAS_DUMMY_CHRC BT_AUDIO_CHRC(0, 0, 0, NULL, NULL, NULL)

/* Gaming Audio Service attributes */
static struct bt_gatt_attr svc_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GMAS),
	BT_AUDIO_CHRC(BT_UUID_GMAP_ROLE, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
		      read_gmap_role, NULL, NULL),
#if defined(CONFIG_BT_GMAP_UGG_SUPPORTED)
	GMAS_DUMMY_CHRC,
#endif /* CONFIG_BT_GMAP_UGG_SUPPORTED */
#if defined(CONFIG_BT_GMAP_UGT_SUPPORTED)
	GMAS_DUMMY_CHRC,
#endif /* CONFIG_BT_GMAP_UGT_SUPPORTED */
#if defined(CONFIG_BT_GMAP_BGS_SUPPORTED)
	GMAS_DUMMY_CHRC,
#endif /* CONFIG_BT_GMAP_BGS_SUPPORTED */
#if defined(CONFIG_BT_GMAP_BGR_SUPPORTED)
	GMAS_DUMMY_CHRC,
#endif /* CONFIG_BT_GMAP_BGR_SUPPORTED */
};
static struct bt_gatt_service gmas;

static bool valid_gmap_role(enum bt_gmap_role role)
{
	if (role == 0 || (role & BT_GMAP_ROLE_MASK) != role) {
		LOG_DBG("Invalid role %d", role);
	}

	if ((role & BT_GMAP_ROLE_UGG) != 0 && !IS_ENABLED(CONFIG_BT_GMAP_UGG_SUPPORTED)) {
		LOG_DBG("Device does not support the UGG role");

		return false;
	}

	if ((role & BT_GMAP_ROLE_UGT) != 0 && !IS_ENABLED(CONFIG_BT_GMAP_UGT_SUPPORTED)) {
		LOG_DBG("Device does not support the UGT role");

		return false;
	}

	if ((role & BT_GMAP_ROLE_BGS) != 0 && !IS_ENABLED(CONFIG_BT_GMAP_BGS_SUPPORTED)) {
		LOG_DBG("Device does not support the BGS role");

		return false;
	}

	if ((role & BT_GMAP_ROLE_BGR) != 0 && !IS_ENABLED(CONFIG_BT_GMAP_BGR_SUPPORTED)) {
		LOG_DBG("Device does not support the BGR role");

		return false;
	}

	return true;
}

static bool valid_gmap_features(enum bt_gmap_role role, struct bt_gmap_feat features)
{
	/* Guard with BT_GMAP_UGG_SUPPORTED as the Kconfigs may not be available without it*/
#if defined(CONFIG_BT_GMAP_UGG_SUPPORTED)
	if ((role & BT_GMAP_ROLE_UGG) != 0) {
		enum bt_gmap_ugg_feat ugg_feat = features.ugg_feat;

		if ((ugg_feat & BT_GMAP_UGG_FEAT_MULTIPLEX) != 0 &&
		    CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT == 0) {
			LOG_DBG("Cannot support BT_GMAP_UGG_FEAT_MULTIPLEX with "
				"CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT == 0");

			return false;
		}

		if ((ugg_feat & BT_GMAP_UGG_FEAT_96KBPS_SOURCE) != 0 &&
		    CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT == 0) {
			LOG_DBG("Cannot support BT_GMAP_UGG_FEAT_96KBPS_SOURCE with "
				"CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SRC_COUNT == 0");

			return false;
		}

		if ((ugg_feat & BT_GMAP_UGG_FEAT_MULTISINK) != 0 &&
		    (CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT < 2 ||
		     CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT < 2)) {
			LOG_DBG("Cannot support BT_GMAP_UGG_FEAT_MULTISINK with "
				"CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT (%d) or "
				"CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT (%d) < 2",
				CONFIG_BT_BAP_UNICAST_CLIENT_ASE_SNK_COUNT,
				CONFIG_BT_BAP_UNICAST_CLIENT_GROUP_STREAM_COUNT);

			return false;
		}
	}
#endif /* CONFIG_BT_BAP_UNICAST_CLIENT */

#if defined(CONFIG_BT_GMAP_UGT_SUPPORTED)
	if ((role & BT_GMAP_ROLE_UGT) != 0) {
		enum bt_gmap_ugt_feat ugt_feat = features.ugt_feat;
		enum bt_gmap_bgr_feat bgr_feat = features.bgr_feat;

		if ((ugt_feat & BT_GMAP_UGT_FEAT_SOURCE) == 0 &&
		    (ugt_feat & BT_GMAP_UGT_FEAT_SINK) == 0) {
			LOG_DBG("Device shall support either BT_GMAP_UGT_FEAT_SOURCE or "
				"BT_GMAP_UGT_FEAT_SINK");

			return false;
		}

		if ((ugt_feat & BT_GMAP_UGT_FEAT_SOURCE) == 0 &&
		    ((ugt_feat & BT_GMAP_UGT_FEAT_80KBPS_SOURCE) != 0 ||
		     (ugt_feat & BT_GMAP_UGT_FEAT_MULTISOURCE) != 0)) {
			LOG_DBG("Device shall support BT_GMAP_UGT_FEAT_SOURCE if "
				"BT_GMAP_UGT_FEAT_80KBPS_SOURCE or BT_GMAP_UGT_FEAT_MULTISOURCE is "
				"supported");

			return false;
		}

		if ((ugt_feat & BT_GMAP_UGT_FEAT_SOURCE) != 0 &&
		    CONFIG_BT_ASCS_ASE_SRC_COUNT == 0) {
			LOG_DBG("Cannot support BT_GMAP_UGT_FEAT_SOURCE with "
				"CONFIG_BT_ASCS_ASE_SRC_COUNT == 0");

			return false;
		}

		if ((ugt_feat & BT_GMAP_UGT_FEAT_MULTISOURCE) != 0 &&
		    (CONFIG_BT_ASCS_ASE_SRC_COUNT < 2 || CONFIG_BT_ASCS_MAX_ACTIVE_ASES < 2)) {
			LOG_DBG("Cannot support BT_GMAP_UGT_FEAT_MULTISOURCE with "
				"CONFIG_BT_ASCS_ASE_SRC_COUNT (%d) or "
				"CONFIG_BT_ASCS_MAX_ACTIVE_ASES (%d) < 2",
				CONFIG_BT_ASCS_ASE_SRC_COUNT, CONFIG_BT_ASCS_MAX_ACTIVE_ASES);

			return false;
		}

		if ((ugt_feat & BT_GMAP_UGT_FEAT_SINK) != 0 && CONFIG_BT_ASCS_ASE_SNK_COUNT == 0) {
			LOG_DBG("Cannot support BT_GMAP_UGT_FEAT_SINK with "
				"CONFIG_BT_ASCS_ASE_SNK_COUNT == 0");

			return false;
		}

		if ((ugt_feat & BT_GMAP_UGT_FEAT_SINK) == 0 &&
		    ((ugt_feat & BT_GMAP_UGT_FEAT_64KBPS_SINK) != 0 ||
		     (ugt_feat & BT_GMAP_UGT_FEAT_MULTISINK) != 0 ||
		     (ugt_feat & BT_GMAP_UGT_FEAT_MULTIPLEX) != 0)) {
			LOG_DBG("Device shall support BT_GMAP_UGT_FEAT_SINK if "
				"BT_GMAP_UGT_FEAT_64KBPS_SINK, BT_GMAP_UGT_FEAT_MULTISINK or "
				"BT_GMAP_UGT_FEAT_MULTIPLEX is supported");

			return false;
		}

		if ((ugt_feat & BT_GMAP_UGT_FEAT_MULTISINK) != 0 &&
		    (CONFIG_BT_ASCS_ASE_SNK_COUNT < 2 || CONFIG_BT_ASCS_MAX_ACTIVE_ASES < 2)) {
			LOG_DBG("Cannot support BT_GMAP_UGT_FEAT_MULTISINK with "
				"CONFIG_BT_ASCS_ASE_SNK_COUNT (%d) or "
				"CONFIG_BT_ASCS_MAX_ACTIVE_ASES (%d) < 2",
				CONFIG_BT_ASCS_ASE_SNK_COUNT, CONFIG_BT_ASCS_MAX_ACTIVE_ASES);

			return false;
		}

		/* If the device supports both the UGT and BGT roles, then it needs have the same
		 * support for multiplexing for both roles
		 */
		if ((role & BT_GMAP_ROLE_BGR) != 0 && !IS_ENABLED(CONFIG_BT_GMAP_BGR_SUPPORTED) &&
		    (ugt_feat & BT_GMAP_UGT_FEAT_MULTIPLEX) !=
			    (bgr_feat & BT_GMAP_BGR_FEAT_MULTIPLEX)) {
			LOG_DBG("Device shall support BT_GMAP_UGT_FEAT_MULTIPLEX if "
				"BT_GMAP_BGR_FEAT_MULTIPLEX is supported, and vice versa");
		}
	}
#endif /* CONFIG_BT_GMAP_UGT_SUPPORTED */

#if defined(CONFIG_BT_GMAP_BGR_SUPPORTED)
	if ((role & BT_GMAP_ROLE_BGR) != 0) {
		enum bt_gmap_bgr_feat bgr_feat = features.bgr_feat;

		if ((bgr_feat & BT_GMAP_BGR_FEAT_MULTISINK) != 0 &&
		    CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT < 2) {
			LOG_DBG("Cannot support BT_GMAP_BGR_FEAT_MULTISINK with "
				"CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT (%d) < 2",
				CONFIG_BT_BAP_BROADCAST_SNK_STREAM_COUNT);

			return false;
		}
	}
#endif /* CONFIG_BT_GMAP_BGR_SUPPORTED */

	/* If the roles are not supported, then the feature characteristics are not instantiated and
	 * the feature values do not need to be checked, as they will never be read (thus ignore by
	 * the stack)
	 */

	return true;
}

static void update_service(enum bt_gmap_role role)
{
	gmas.attrs = svc_attrs;
	gmas.attr_count = 3; /* service + 2 attributes for BT_UUID_GMAP_ROLE */

	/* Add characteristics based on the role selected and what is supported */
#if defined(CONFIG_BT_GMAP_UGG_SUPPORTED)
	if (role & BT_GMAP_ROLE_UGG) {
		memcpy(&gmas.attrs[gmas.attr_count], ugg_feat_chrc, sizeof(ugg_feat_chrc));
		gmas.attr_count += ARRAY_SIZE(ugg_feat_chrc);
	}
#endif /* CONFIG_BT_GMAP_UGG_SUPPORTED */

#if defined(CONFIG_BT_GMAP_UGT_SUPPORTED)
	if (role & BT_GMAP_ROLE_UGT) {
		memcpy(&gmas.attrs[gmas.attr_count], ugt_feat_chrc, sizeof(ugt_feat_chrc));
		gmas.attr_count += ARRAY_SIZE(ugt_feat_chrc);
	}
#endif /* CONFIG_BT_GMAP_UGT_SUPPORTED */

#if defined(CONFIG_BT_GMAP_BGS_SUPPORTED)
	if (role & BT_GMAP_ROLE_BGS) {
		memcpy(&gmas.attrs[gmas.attr_count], bgs_feat_chrc, sizeof(bgs_feat_chrc));
		gmas.attr_count += ARRAY_SIZE(bgs_feat_chrc);
	}
#endif /* CONFIG_BT_GMAP_BGS_SUPPORTED */

#if defined(CONFIG_BT_GMAP_BGR_SUPPORTED)
	if (role & BT_GMAP_ROLE_BGR) {
		memcpy(&gmas.attrs[gmas.attr_count], bgr_feat_chrc, sizeof(bgr_feat_chrc));
		gmas.attr_count += ARRAY_SIZE(bgr_feat_chrc);
	}
#endif /* CONFIG_BT_GMAP_BGR_SUPPORTED */
}

int bt_gmap_register(enum bt_gmap_role role, struct bt_gmap_feat features)
{
	int err;

	CHECKIF(!valid_gmap_role(role)) {
		LOG_DBG("Invalid role: %d", role);

		return -EINVAL;
	}

	CHECKIF(!valid_gmap_features(role, features)) {
		LOG_DBG("Invalid features");

		return -EINVAL;
	}

	update_service(role);

	err = bt_gatt_service_register(&gmas);
	if (err) {
		LOG_DBG("Could not register the GMAS service");

		return -ENOEXEC;
	}

	gmap_role = role;
	gmap_features = features;

	return 0;
}

int bt_gmap_set_role(enum bt_gmap_role role, struct bt_gmap_feat features)
{
	int err;

	if (gmap_role == 0) { /* not registered if this is 0 */
		LOG_DBG("GMAP not registered");

		return -ENOEXEC;
	}

	CHECKIF(!valid_gmap_role(role)) {
		LOG_DBG("Invalid role: %d", role);

		return -EINVAL;
	}

	CHECKIF(!valid_gmap_features(role, features)) {
		LOG_DBG("Invalid features");

		return -EINVAL;
	}

	if (gmap_role == role) {
		LOG_DBG("No role change");

		if (memcmp(&gmap_features, &features, sizeof(gmap_features)) == 0) {
			LOG_DBG("No feature change");

			return -EALREADY;
		}

		gmap_features = features;

		return 0;
	}

	/* Re-register the service to trigger a db_changed() if the roles changed */
	err = bt_gatt_service_unregister(&gmas);
	if (err != 0) {
		LOG_DBG("Failed to unregister service: %d", err);

		return -ENOENT;
	}

	err = bt_gmap_register(role, gmap_features);
	if (err != 0) {
		LOG_DBG("Failed to update GMAS: %d", err);

		return -ECANCELED;
	}

	return 0;
}
