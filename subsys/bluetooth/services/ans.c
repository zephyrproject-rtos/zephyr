/*
 * Copyright (c) 2025 Sean Kyer
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * https://www.bluetooth.com/specifications/specs/alert-notification-service-1-0/
 */

#include <errno.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/ans.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_ans, CONFIG_BT_ANS_LOG_LEVEL);

/* Build time ANS supported category bit mask */
#define BT_ANS_NALRT_CAT_MASK                                                                      \
	((IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_SIMPLE_ALERT) << bt_ans_cat_simple_alert) |           \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_EMAIL) << bt_ans_cat_email) |                         \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_NEWS) << bt_ans_cat_news) |                           \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_CALL) << bt_ans_cat_call) |                           \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_MISSED_CALL) << bt_ans_cat_missed_call) |             \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_SMS_MMS) << bt_ans_cat_sms_mms) |                     \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_VOICE_MAIL) << bt_ans_cat_voice_mail) |               \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_SCHEDULE) << bt_ans_cat_schedule) |                   \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_HIGH_PRI_ALERT) << bt_ans_cat_high_pri_alert) |       \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_INSTANT_MESSAGE) << bt_ans_cat_instant_message))

/* Build time ANS supported category bit mask */
#define BT_ANS_UNALRT_CAT_MASK                                                                     \
	((IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_SIMPLE_ALERT) << bt_ans_cat_simple_alert) |          \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_EMAIL) << bt_ans_cat_email) |                        \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_NEWS) << bt_ans_cat_news) |                          \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_CALL) << bt_ans_cat_call) |                          \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_MISSED_CALL) << bt_ans_cat_missed_call) |            \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_SMS_MMS) << bt_ans_cat_sms_mms) |                    \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_VOICE_MAIL) << bt_ans_cat_voice_mail) |              \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_SCHEDULE) << bt_ans_cat_schedule) |                  \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_HIGH_PRI_ALERT) << bt_ans_cat_high_pri_alert) |      \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_INSTANT_MESSAGE) << bt_ans_cat_instant_message))

/* As per spec, ensure at least one New Alert category is supported */
BUILD_ASSERT(BT_ANS_NALRT_CAT_MASK != 0,
	     "At least one ANS New Alert category must be enabled in Kconfig");

/* Statically configured via Kconfig */
const static uint16_t alert_cat_id_bit_mask = BT_ANS_NALRT_CAT_MASK;
const static uint16_t unread_cat_id_bit_mask = BT_ANS_UNALRT_CAT_MASK;

/* Command ID definitions */
#define BT_ANS_SEND_ALL_CATEGORY 0xff
typedef uint8_t bt_ans_command_id_t;
enum {
	bt_ans_ctrl_enable_new_alert,
	bt_ans_ctrl_enable_unread,
	bt_ans_ctrl_disable_new_alert,
	bt_ans_ctrl_disable_unread,
	bt_ans_ctrl_notify_new_alert_immediate,
	bt_ans_ctrl_notify_unread_immediate,
};

/* Struct definition for Alert Notification Control Point */
struct  __packed alert_ctrl_p {
	bt_ans_command_id_t cmd_id;
	bt_ans_cat_t category;
};

/* Struct definition for New Alert */
struct  __packed new_alert {
	bt_ans_cat_t category_id;
	uint8_t num_new_alerts;
	char text_string[BT_ANS_MAX_TEXT_STR_SIZE];
};

/* Struct definition for Unread Alert */
struct  __packed unread_alert_status {
	bt_ans_cat_t category_id;
	uint8_t unread_count;
};

/* Mutex for modifying database */
K_MUTEX_DEFINE(new_alert_mutex);
K_MUTEX_DEFINE(unread_mutex);

/* Saved messages database */
static struct new_alert new_alerts[bt_ans_cat_num];
static struct unread_alert_status unread_alerts[bt_ans_cat_num];

/* Initialize to 0, it is control point responsibility to enable once connected */
static uint16_t alert_cat_enabled;
static uint16_t unread_cat_enabled;

/* Notification variables configured by CCC */
static bool new_alert_notify;
static bool unread_alert_notify;

/* Supported New Alert Category */
static ssize_t read_supp_new_alert_cat(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				       void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("Supported New Alert Category Read");

	/* Return the bit mask of the supported categories */
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &alert_cat_id_bit_mask,
				 sizeof(alert_cat_id_bit_mask));
}

/* New Alert Notifications */
static void new_alert_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	new_alert_notify = (value == BT_GATT_CCC_NOTIFY);

	LOG_DBG("New Alert Notifications %s", new_alert_notify ? "enabled" : "disabled");
}

/* Supported Unread Alert Category*/
static ssize_t read_supp_unread_alert_cat(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					  void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("Supported Unread Alert Category Read");

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &unread_cat_id_bit_mask,
				 sizeof(unread_cat_id_bit_mask));
}

/* Unread Alert Status Notifications */
static void unread_alert_status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	unread_alert_notify = (value == BT_GATT_CCC_NOTIFY);

	LOG_DBG("Unread Alert Status Notifications %s",
		unread_alert_notify ? "enabled" : "disabled");
}

/* Alert Notification Control Point */
static int transmit_new_alert(bt_ans_cat_t category);
static int transmit_unread_new_alert(bt_ans_cat_t category);
static ssize_t write_alert_notif_ctrl_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					    const void *buf, uint16_t len, uint16_t offset,
					    uint8_t flags)
{
	int rc;

	LOG_DBG("Alert Control Point Written %d", len);

	if (len != sizeof(struct alert_ctrl_p)) {
		LOG_ERR("Length of control packet is %d when expected %d", len,
			sizeof(struct alert_ctrl_p));
		return BT_GATT_ERR(BT_ATT_ERR_CMD_NOT_SUP);
	}

	struct alert_ctrl_p command = *((const struct alert_ctrl_p *)buf);

	LOG_DBG("Command ID 0x%x", command.cmd_id);
	LOG_DBG("Category 0x%x", command.category);

	if (command.category >= bt_ans_cat_num && command.category != BT_ANS_SEND_ALL_CATEGORY) {
		LOG_ERR("Received control point request for category out of bounds: %d",
			command.category);
		return BT_GATT_ERR(BT_ATT_ERR_CMD_NOT_SUP);
	}

	switch (command.cmd_id) {
	case bt_ans_ctrl_enable_new_alert:
		if ((alert_cat_id_bit_mask & (1U << command.category)) == 0) {
			LOG_ERR("Received control point request for unsupported category: %d",
				command.category);
			return BT_GATT_ERR(BT_ATT_ERR_CMD_NOT_SUP);
		}
		alert_cat_enabled |= (1U << command.category);
		return len;
	case bt_ans_ctrl_enable_unread:
		if ((unread_cat_id_bit_mask & (1U << command.category)) == 0) {
			LOG_ERR("Received control point request for unsupported category: %d",
				command.category);
			return BT_GATT_ERR(BT_ATT_ERR_CMD_NOT_SUP);
		}
		unread_cat_enabled |= (1U << command.category);
		return len;
	case bt_ans_ctrl_disable_new_alert:
		if ((alert_cat_id_bit_mask & (1U << command.category)) == 0) {
			LOG_ERR("Received control point request for unsupported category: %d",
				command.category);
			return BT_GATT_ERR(BT_ATT_ERR_CMD_NOT_SUP);
		}
		alert_cat_enabled &= ~(1U << command.category);
		return len;
	case bt_ans_ctrl_disable_unread:
		if ((unread_cat_id_bit_mask & (1U << command.category)) == 0) {
			LOG_ERR("Received control point request for unsupported category: %d",
				command.category);
			return BT_GATT_ERR(BT_ATT_ERR_CMD_NOT_SUP);
		}
		unread_cat_enabled &= ~(1U << command.category);
		return len;
	case bt_ans_ctrl_notify_new_alert_immediate:
		rc = transmit_new_alert(command.category);
		if (rc) {
			LOG_ERR("Error transmitting New Alert during control point request! "
				"(rc=%d)",
				rc);
			return BT_GATT_ERR(BT_ATT_ERR_CMD_NOT_SUP);
		}
		return len;
	case bt_ans_ctrl_notify_unread_immediate:
		rc = transmit_unread_new_alert(command.category);
		if (rc) {
			LOG_ERR("Error transmitting Unread Alert during control point request! "
				"(rc=%d)",
				rc);
			return BT_GATT_ERR(BT_ATT_ERR_CMD_NOT_SUP);
		}
		return len;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_CMD_NOT_SUP);
	}
}

static int ans_init(void)
{
	for (int i = 0; i < bt_ans_cat_num; i++) {
		new_alerts[i].category_id = i;
		unread_alerts[i].category_id = i;
	}

	LOG_INF("ANS initialization complete");

	return 0;
}

/* Alert Notification Service Declaration */
BT_GATT_SERVICE_DEFINE(
	ans_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_ANS),
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_SNALRTC, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_supp_new_alert_cat, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_NALRT, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL,
			       NULL, NULL),
	BT_GATT_CCC(new_alert_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_SUALRTC, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       read_supp_unread_alert_cat, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_UALRTS, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL,
			       NULL, NULL),
	BT_GATT_CCC(unread_alert_status_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(BT_UUID_GATT_ALRTNCP, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL,
			       write_alert_notif_ctrl_point, NULL),);

/* Transmit New Alert */
static int transmit_new_alert(bt_ans_cat_t category)
{
	int rc;

	if (new_alert_notify) {

		/*
		 * If special code BT_ANS_SEND_ALL_CATEGORY requested, transmit all supported
		 * categories
		 */
		if (category == BT_ANS_SEND_ALL_CATEGORY) {
			for (int i = 0; i < bt_ans_cat_num; i++) {
				if ((alert_cat_id_bit_mask & (1U << category)) &&
				    (alert_cat_enabled & (1U << category))) {
					k_mutex_lock(&new_alert_mutex, K_FOREVER);
					rc = bt_gatt_notify(NULL, &ans_svc.attrs[3],
							    &new_alerts[category],
							    sizeof(struct new_alert));
					k_mutex_unlock(&new_alert_mutex);
					if (rc) {
						LOG_ERR("Error notifying New Alert category %d rc: "
							"%d",
							category, rc);
						return rc;
					}
				}
			}
			return 0;
		}

		if (alert_cat_enabled & (1U << category)) {
			k_mutex_lock(&new_alert_mutex, K_FOREVER);
			rc = bt_gatt_notify(NULL, &ans_svc.attrs[3], &new_alerts[category],
					    sizeof(struct new_alert));
			k_mutex_unlock(&new_alert_mutex);
			if (rc) {
				LOG_ERR("Error notifying New Alert category %d rc: "
					"%d",
					category, rc);
				return rc;
			}
			return rc;
		}
	}

	return 0;
}

/* Transmit Unread Alert */
static int transmit_unread_new_alert(bt_ans_cat_t category)
{
	int rc;

	if (unread_alert_notify) {

		/*
		 * If special code BT_ANS_SEND_ALL_CATEGORY requested, transmit all supported
		 * categories
		 */
		if (category == BT_ANS_SEND_ALL_CATEGORY) {
			for (int i = 0; i < bt_ans_cat_num; i++) {
				if ((unread_cat_id_bit_mask & (1U << category)) &&
				    (unread_cat_enabled & (1U << category))) {
					k_mutex_lock(&unread_mutex, K_FOREVER);
					rc = bt_gatt_notify(NULL, &ans_svc.attrs[8],
							    &unread_alerts[category],
							    sizeof(struct unread_alert_status));
					k_mutex_unlock(&unread_mutex);
					if (rc) {
						LOG_ERR("Error notifying Unread Alert category %d "
							"rc: %d",
							category, rc);
						return rc;
					}
				}
			}
			return 0;
		}

		if (unread_cat_enabled & (1U << category)) {
			k_mutex_lock(&unread_mutex, K_FOREVER);
			rc = bt_gatt_notify(NULL, &ans_svc.attrs[8], &unread_alerts[category],
					    sizeof(struct unread_alert_status));
			k_mutex_unlock(&unread_mutex);
			if (rc) {
				LOG_ERR("Error notifying Unread Alert category %d rc: "
					"%d",
					category, rc);
				return rc;
			}
			return rc;
		}
	}

	return 0;
}

int bt_ans_notify_new_alert(bt_ans_cat_t category, uint8_t num_new, const char *text, int text_len)
{
	/* Check if the category is supported */
	if ((alert_cat_id_bit_mask & (1U << category)) == 0) {
		LOG_ERR("Category %d unsupported", category);
		return -ENOTSUP;
	}

	/* Update the saved value */
	k_mutex_lock(&new_alert_mutex, K_FOREVER);
	new_alerts[category].num_new_alerts = num_new;
	strncpy(new_alerts[category].text_string, text, MIN(BT_ANS_MAX_TEXT_STR_SIZE, text_len));
	k_mutex_unlock(&new_alert_mutex);

	return transmit_new_alert(category);
}

int bt_ans_set_unread_count(bt_ans_cat_t category, uint8_t unread)
{
	/* Check if the category is supported */
	if ((unread_cat_id_bit_mask & (1U << category)) == 0) {
		LOG_ERR("Category %d unsupported", category);
		return -ENOTSUP;
	}

	/* Update the saved value */
	k_mutex_lock(&unread_mutex, K_FOREVER);
	unread_alerts[category].unread_count = unread;
	k_mutex_unlock(&unread_mutex);

	return transmit_unread_new_alert(category);
}

SYS_INIT(ans_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
