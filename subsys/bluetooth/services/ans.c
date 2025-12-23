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
#include <zephyr/sys/util_utf8.h>

LOG_MODULE_REGISTER(bt_ans, CONFIG_BT_ANS_LOG_LEVEL);

/*
 * This only enforces a necessary lower bound at build time. It does
 * not guarantee that notification/transmit operations will never fail at
 * runtime because other subsystems/services can hold outstanding ATT
 * buffers concurrently.
 */
BUILD_ASSERT(CONFIG_BT_MAX_CONN <= CONFIG_BT_ATT_TX_COUNT,
	     "CONFIG_BT_ATT_TX_COUNT must be >= CONFIG_BT_MAX_CONN");

/* Build time ANS supported category bit mask */
#define BT_ANS_NALRT_CAT_MASK                                                                      \
	((IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_SIMPLE_ALERT) << BT_ANS_CAT_SIMPLE_ALERT) |           \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_EMAIL) << BT_ANS_CAT_EMAIL) |                         \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_NEWS) << BT_ANS_CAT_NEWS) |                           \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_CALL) << BT_ANS_CAT_CALL) |                           \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_MISSED_CALL) << BT_ANS_CAT_MISSED_CALL) |             \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_SMS_MMS) << BT_ANS_CAT_SMS_MMS) |                     \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_VOICE_MAIL) << BT_ANS_CAT_VOICE_MAIL) |               \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_SCHEDULE) << BT_ANS_CAT_SCHEDULE) |                   \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_HIGH_PRI_ALERT) << BT_ANS_CAT_HIGH_PRI_ALERT) |       \
	 (IS_ENABLED(CONFIG_BT_ANS_NALRT_CAT_INSTANT_MESSAGE) << BT_ANS_CAT_INSTANT_MESSAGE))

/* Build time ANS supported category bit mask */
#define BT_ANS_UNALRT_CAT_MASK                                                                     \
	((IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_SIMPLE_ALERT) << BT_ANS_CAT_SIMPLE_ALERT) |          \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_EMAIL) << BT_ANS_CAT_EMAIL) |                        \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_NEWS) << BT_ANS_CAT_NEWS) |                          \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_CALL) << BT_ANS_CAT_CALL) |                          \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_MISSED_CALL) << BT_ANS_CAT_MISSED_CALL) |            \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_SMS_MMS) << BT_ANS_CAT_SMS_MMS) |                    \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_VOICE_MAIL) << BT_ANS_CAT_VOICE_MAIL) |              \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_SCHEDULE) << BT_ANS_CAT_SCHEDULE) |                  \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_HIGH_PRI_ALERT) << BT_ANS_CAT_HIGH_PRI_ALERT) |      \
	 (IS_ENABLED(CONFIG_BT_ANS_UNALRT_CAT_INSTANT_MESSAGE) << BT_ANS_CAT_INSTANT_MESSAGE))

/* As per spec, ensure at least one New Alert category is supported */
BUILD_ASSERT(BT_ANS_NALRT_CAT_MASK != 0,
	     "At least one ANS New Alert category must be enabled in Kconfig");

static uint16_t alert_sup_cat_bit_mask = BT_ANS_NALRT_CAT_MASK;
static uint16_t unread_sup_cat_bit_mask = BT_ANS_UNALRT_CAT_MASK;

/* Command ID definitions */
#define BT_ANS_SEND_ALL_CATEGORY 0xFF
enum bt_ans_command_id {
	BT_ANS_CTRL_ENABLE_NEW_ALERT,
	BT_ANS_CTRL_ENABLE_UNREAD,
	BT_ANS_CTRL_DISABLE_NEW_ALERT,
	BT_ANS_CTRL_DISABLE_UNREAD,
	BT_ANS_CTRL_NOTIFY_NEW_ALERT_IMMEDIATE,
	BT_ANS_CTRL_NOTIFY_UNREAD_IMMEDIATE,
};

/* Struct definition for Alert Notification Control Point */
struct alert_ctrl_p {
	uint8_t cmd_id;
	uint8_t category;
} __packed;

/* Struct definition for New Alert */
struct new_alert {
	uint8_t category_id;
	uint8_t num_new_alerts;
	char text_string[BT_ANS_MAX_TEXT_STR_SIZE + 1];
} __packed;

/* Struct definition for Unread Alert */
struct unread_alert_status {
	uint8_t category_id;
	uint8_t unread_count;
} __packed;

/* Mutex for modifying database */
K_MUTEX_DEFINE(new_alert_mutex);
K_MUTEX_DEFINE(unread_mutex);

/* Saved messages database */
static struct new_alert new_alerts[BT_ANS_CAT_NUM];
static struct unread_alert_status unread_alerts[BT_ANS_CAT_NUM];

/* Initialize to 0, it is control point responsibility to enable once connected */
static uint16_t alert_cat_enabled_map;
static uint16_t unread_cat_enabled_map;

/* Supported New Alert Category */
static ssize_t read_supp_new_alert_cat(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				       void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("Supported New Alert Category Read");

	/* Return the bit mask of the supported categories */
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &alert_sup_cat_bit_mask,
				 sizeof(alert_sup_cat_bit_mask));
}

/* New Alert Notifications */
static void new_alert_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	LOG_DBG("New Alert Notifications %s", value == BT_GATT_CCC_NOTIFY ? "enabled" : "disabled");
}

/* Supported Unread Alert Category*/
static ssize_t read_supp_unread_alert_cat(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					  void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("Supported Unread Alert Category Read");

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &unread_sup_cat_bit_mask,
				 sizeof(unread_sup_cat_bit_mask));
}

/* Unread Alert Status Notifications */
static void unread_alert_status_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	LOG_DBG("Unread Alert Status Notifications %s",
		value == BT_GATT_CCC_NOTIFY ? "enabled" : "disabled");
}

/* Alert Notification Control Point */
static int transmit_new_alert(struct bt_conn *conn, enum bt_ans_cat category);
static int transmit_unread_alert(struct bt_conn *conn, enum bt_ans_cat category);
static ssize_t write_alert_notif_ctrl_point(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					    const void *buf, uint16_t len, uint16_t offset,
					    uint8_t flags)
{
	int rc;

	LOG_DBG("Alert Control Point Written %u", len);

	if (len != sizeof(struct alert_ctrl_p)) {
		LOG_DBG("Length of control packet is %u when expected %zu", len,
			sizeof(struct alert_ctrl_p));
		return BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP);
	}

	struct alert_ctrl_p command = *((const struct alert_ctrl_p *)buf);

	LOG_DBG("Command ID 0x%x", command.cmd_id);
	LOG_DBG("Category 0x%x", command.category);

	if (command.category >= BT_ANS_CAT_NUM && command.category != BT_ANS_SEND_ALL_CATEGORY) {
		LOG_DBG("Received control point request for category out of bounds: %d",
			command.category);
		return BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP);
	}

	/*
	 * If category is BT_ANS_SEND_ALL_CATEGORY then only BT_ANS_CTRL_NOTIFY_NEW_ALERT_IMMEDIATE
	 * or BT_ANS_CTRL_NOTIFY_UNREAD_IMMEDIATE are valid command IDs
	 */
	if (command.category == BT_ANS_SEND_ALL_CATEGORY) {
		switch (command.cmd_id) {
		case BT_ANS_CTRL_NOTIFY_NEW_ALERT_IMMEDIATE:
			rc = transmit_new_alert(conn, command.category);
			return rc ? BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP) : len;
		case BT_ANS_CTRL_NOTIFY_UNREAD_IMMEDIATE:
			rc = transmit_unread_alert(conn, command.category);
			return rc ? BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP) : len;
		default:
			return BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP);
		}
	}

	switch (command.cmd_id) {
	case BT_ANS_CTRL_ENABLE_NEW_ALERT:
		if ((alert_sup_cat_bit_mask & (1U << command.category)) == 0) {
			LOG_DBG("Received control point request for unsupported category: "
				"%d",
				command.category);
			return BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP);
		}
		alert_cat_enabled_map |= (1U << command.category);
		break;
	case BT_ANS_CTRL_ENABLE_UNREAD:
		if ((unread_sup_cat_bit_mask & (1U << command.category)) == 0) {
			LOG_DBG("Received control point request for unsupported category: "
				"%d",
				command.category);
			return BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP);
		}
		unread_cat_enabled_map |= (1U << command.category);
		break;
	case BT_ANS_CTRL_DISABLE_NEW_ALERT:
		if ((alert_sup_cat_bit_mask & (1U << command.category)) == 0) {
			LOG_DBG("Received control point request for unsupported category: "
				"%d",
				command.category);

			return BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP);
		}
		alert_cat_enabled_map &= ~(1U << command.category);
		break;
	case BT_ANS_CTRL_DISABLE_UNREAD:
		if ((unread_sup_cat_bit_mask & (1U << command.category)) == 0) {
			LOG_DBG("Received control point request for unsupported category: "
				"%d",
				command.category);
			return BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP);
		}
		unread_cat_enabled_map &= ~(1U << command.category);
		break;
	default:
		return BT_GATT_ERR(BT_ANS_ERR_CMD_NOT_SUP);
	}

	return len;
}

static int ans_init(void)
{
	for (int i = 0; i < BT_ANS_CAT_NUM; i++) {
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
			       write_alert_notif_ctrl_point, NULL));

/* Helper to notify a single category */
static int notify_new_alert_category(struct bt_conn *conn, uint8_t cat)
{
	int rc;
	int ret;

	ret = k_mutex_lock(&new_alert_mutex, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Unable to lock mutex (err: %d)", ret);
		return -EAGAIN;
	}

	rc = bt_gatt_notify_uuid(conn, BT_UUID_GATT_NALRT, ans_svc.attrs, &new_alerts[cat],
				 sizeof(struct new_alert));

	ret = k_mutex_unlock(&new_alert_mutex);
	__ASSERT(ret == 0, "Unable to unlock mutex (err: %d)", ret);

	/* If the client is not connected, that is fine */
	if (rc != 0 && rc != -ENOTCONN) {
		LOG_DBG("Error notifying New Alert category %d rc: %d", cat, rc);
		return rc;
	}

	return 0;
}

/* Transmit New Alert */
static int transmit_new_alert(struct bt_conn *conn, enum bt_ans_cat category)
{
	int rc;

	uint8_t cat_in = (uint8_t)category;

	/* Nothing to do if notify is disabled */
	if (conn != NULL && !bt_gatt_is_subscribed(conn, &ans_svc.attrs[3], BT_GATT_CCC_NOTIFY)) {
		return 0;
	}

	/* Special case: send all categories */
	if (cat_in == BT_ANS_SEND_ALL_CATEGORY) {
		for (int i = 0; i < BT_ANS_CAT_NUM; i++) {
			if (((alert_sup_cat_bit_mask & BIT(i)) != 0) &&
			    ((alert_cat_enabled_map & BIT(i)) != 0)) {
				rc = notify_new_alert_category(conn, i);
				if (rc < 0) {
					return rc;
				}
			}
		}
		return 0;
	}

	/* Otherwise send just the requested category (if enabled) */
	if ((alert_cat_enabled_map & BIT(cat_in)) != 0) {
		return notify_new_alert_category(conn, cat_in);
	}

	return 0;
}

/* Helper to notify a single unread alert category */
static int notify_unread_alert_category(struct bt_conn *conn, uint8_t cat)
{
	int rc;
	int ret;

	ret = k_mutex_lock(&unread_mutex, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Unable to lock mutex (err: %d)", ret);
		return -EAGAIN;
	}

	rc = bt_gatt_notify_uuid(conn, BT_UUID_GATT_UALRTS, ans_svc.attrs, &unread_alerts[cat],
				 sizeof(struct unread_alert_status));

	ret = k_mutex_unlock(&unread_mutex);
	__ASSERT(ret == 0, "Unable to unlock mutex (err: %d)", ret);

	/* If the client is not connected, that is fine */
	if (rc != 0 && rc != -ENOTCONN) {
		LOG_DBG("Error notifying Unread Alert category %d rc: %d", cat, rc);
		return rc;
	}

	return 0;
}

/* Transmit Unread Alert */
static int transmit_unread_alert(struct bt_conn *conn, enum bt_ans_cat category)
{
	int rc;

	uint8_t cat_in = (uint8_t)category;

	/* Nothing to do if notify is disabled */
	if (conn != NULL && !bt_gatt_is_subscribed(conn, &ans_svc.attrs[6], BT_GATT_CCC_NOTIFY)) {
		return 0;
	}

	/* Special case: send all categories */
	if (cat_in == BT_ANS_SEND_ALL_CATEGORY) {
		for (int i = 0; i < BT_ANS_CAT_NUM; i++) {
			if ((unread_sup_cat_bit_mask & BIT(i)) &&
			    (unread_cat_enabled_map & BIT(i))) {
				rc = notify_unread_alert_category(conn, i);
				if (rc < 0) {
					return rc;
				}
			}
		}
		return 0;
	}

	/* Otherwise send just the requested category (if enabled) */
	if ((unread_cat_enabled_map & BIT(cat_in)) != 0) {
		return notify_unread_alert_category(conn, cat_in);
	}

	return 0;
}

int bt_ans_notify_new_alert(struct bt_conn *conn, enum bt_ans_cat category, uint8_t num_new,
			    const char *text)
{
	int ret;

	uint8_t cat_in = (uint8_t)category;

	/* Check if the category is supported */
	if ((alert_sup_cat_bit_mask & (1U << cat_in)) == 0) {
		LOG_DBG("Category %d unsupported", cat_in);
		return -EINVAL;
	}

	/* Update the saved value */
	ret = k_mutex_lock(&new_alert_mutex, K_FOREVER);
	__ASSERT(ret == 0, "Unable to lock mutex (err: %d)", ret);
	new_alerts[cat_in].num_new_alerts = num_new;
	utf8_lcpy(new_alerts[cat_in].text_string, text, sizeof(new_alerts[cat_in].text_string));
	ret = k_mutex_unlock(&new_alert_mutex);
	__ASSERT(ret == 0, "Unable to unlock mutex (err: %d)", ret);

	return transmit_new_alert(conn, category);
}

int bt_ans_set_unread_count(struct bt_conn *conn, enum bt_ans_cat category, uint8_t unread)
{
	int ret;

	uint8_t cat_in = (uint8_t)category;

	/* Check if the category is supported */
	if ((unread_sup_cat_bit_mask & (1U << cat_in)) == 0) {
		LOG_DBG("Category %d unsupported", cat_in);
		return -EINVAL;
	}

	/* Update the saved value */
	ret = k_mutex_lock(&unread_mutex, K_FOREVER);
	__ASSERT(ret == 0, "Unable to lock mutex (err: %d)", ret);
	unread_alerts[cat_in].unread_count = unread;
	ret = k_mutex_unlock(&unread_mutex);
	__ASSERT(ret == 0, "Unable to unlock mutex (err: %d)", ret);

	return transmit_unread_alert(conn, category);
}

/* Callback used by bt_conn_foreach() to detect an established connection. */
static void ans_conn_check_cb(struct bt_conn *conn, void *data)
{
	int err;
	bool *has_conn = data;
	struct bt_conn_info info;

	err = bt_conn_get_info(conn, &info);
	if (err == 0 && info.state == BT_CONN_STATE_CONNECTED) {
		*has_conn = true;
	}
}

/* Used to check if an active connect exists, stopping modifications to supported features */
static bool ans_check_conn_busy(void)
{
	bool has_conn = false;

	bt_conn_foreach(BT_CONN_TYPE_ALL, ans_conn_check_cb, &has_conn);

	if (has_conn) {
		/* Cannot change support while connection exists */
		return true;
	}

	return false;
}

int bt_ans_set_new_alert_support_category(uint16_t mask)
{
	if (ans_check_conn_busy()) {
		/* Cannot change support while connection exists */
		return -EBUSY;
	}

	alert_sup_cat_bit_mask = mask;

	LOG_DBG("New Alert Support Bit Mask: %x", alert_sup_cat_bit_mask);

	return 0;
}

int bt_ans_set_unread_support_category(uint16_t mask)
{
	if (ans_check_conn_busy()) {
		/* Cannot change support while connection exists */
		return -EBUSY;
	}

	unread_sup_cat_bit_mask = mask;

	LOG_DBG("Unread Support Bit Mask: %x", unread_sup_cat_bit_mask);

	return 0;
}

SYS_INIT(ans_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
