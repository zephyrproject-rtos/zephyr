/** @file
 *  @brief GATT Current Time Service
 */

/*
 * Copyright (c) 2024 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/posix/time.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/cts.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cts, CONFIG_BT_CTS_LOG_LEVEL);

#define BT_CTS_ATT_ERR_VALUES_IGNORED 0x80
#define BT_CTS_FRACTION_256_MAX_VALUE 255

static const struct bt_cts_cb *cts_cb;

#ifdef CONFIG_BT_CTS_HELPER_API

#include <time.h>
#include <zephyr/sys/timeutil.h>

int bt_cts_time_to_unix_ms(const struct bt_cts_time_format *ct_time, int64_t *unix_ms)
{
	struct tm date_time;
	/* fill date time structure */
	date_time.tm_year = sys_le16_to_cpu(ct_time->year); /* year (little endian) */
	date_time.tm_year -= TIME_UTILS_BASE_YEAR;
	date_time.tm_mon = ct_time->mon - 1;   /* month start from 1, but need from 0 */
	date_time.tm_mday = ct_time->mday;     /* day of month */
	date_time.tm_hour = ct_time->hours;    /* hours of day */
	date_time.tm_min = ct_time->min;       /* minute of hour */
	date_time.tm_sec = ct_time->sec;       /* seconds of minute */
	date_time.tm_wday = ct_time->wday % 7; /* for sundays convert to 0, else keep same */

	LOG_DBG("CTS Write Time: %d/%d/%d %d:%d:%d", date_time.tm_year, date_time.tm_mon,
		date_time.tm_mday, date_time.tm_hour, date_time.tm_min, date_time.tm_sec);
	/* get unit timestamp from datetime */
	(*unix_ms) = timeutil_timegm64(&date_time);
	if ((*unix_ms) == ((time_t)-1)) {
		return -EOVERFLOW;
	}
	LOG_DBG("CTS Write Unix: %lld", (*unix_ms));
	(*unix_ms) *= MSEC_PER_SEC;
	/* add fraction 256 part*/
	(*unix_ms) += ((ct_time->fractions256 * MSEC_PER_SEC) / BT_CTS_FRACTION_256_MAX_VALUE);

	return 0;
}

int bt_cts_time_from_unix_ms(struct bt_cts_time_format *ct_time, int64_t unix_ms)
{
	struct tm date_time;
	time_t unix_ts = unix_ms / MSEC_PER_SEC;

	/* 'Fractions 256 part of 'Exact Time 256' */
	unix_ms %= MSEC_PER_SEC;
	unix_ms *= BT_CTS_FRACTION_256_MAX_VALUE;
	unix_ms /= MSEC_PER_SEC;
	ct_time->fractions256 = unix_ms;

	/* convert unix_ts to */
	LOG_DBG("CTS Read Unix: %lld", unix_ts);
	/* generate date time from unix timestamp */
	if (gmtime_r(&unix_ts, &date_time) == NULL) {
		return -EOVERFLOW;
	}
	date_time.tm_year += TIME_UTILS_BASE_YEAR;

	LOG_DBG("CTS Read Time: %d/%d/%d %d:%d:%d", date_time.tm_year, date_time.tm_mon,
		date_time.tm_mday, date_time.tm_hour, date_time.tm_min, date_time.tm_sec);

	/* 'Exact Time 256' contains 'Day Date Time' which contains
	 * 'Date Time' - characteristic contains fields for:
	 * year, month, day, hours, minutes and seconds.
	 */
	ct_time->year = sys_cpu_to_le16(date_time.tm_year);
	ct_time->mon = date_time.tm_mon + 1; /* months starting from 1 */
	ct_time->mday = date_time.tm_mday;   /* Day of month */
	ct_time->hours = date_time.tm_hour;  /* hours */
	ct_time->min = date_time.tm_min;     /* minutes */
	ct_time->sec = date_time.tm_sec;     /* seconds */
	/* day of week starting from 1-monday, 7-sunday */
	ct_time->wday = date_time.tm_wday;
	if (ct_time->wday == 0) {
		ct_time->wday = 7; /* sunday is represented as 7 */
	}
	return 0;
}

#endif /* CONFIG_BT_CTS_HELPER_API */

static void ct_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("CTS Notifications %s", notif_enabled ? "enabled" : "disabled");

	if (cts_cb->notification_changed) {
		cts_cb->notification_changed(notif_enabled);
	}
}

static ssize_t read_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
		       uint16_t len, uint16_t offset)
{
	int err;
	struct bt_cts_time_format ct_time;

	err = cts_cb->fill_current_cts_time(&ct_time);
	ct_time.reason = BT_CTS_UPDATE_REASON_UNKNOWN;

	if (!err) {
		return bt_gatt_attr_read(conn, attr, buf, len, offset, &ct_time, sizeof(ct_time));
	} else {
		return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
	}
}

static ssize_t write_ct(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			uint16_t len, uint16_t offset, uint8_t flags)
{
	int err;
	struct bt_cts_time_format ct_time;

	if (cts_cb->cts_time_write == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	if ((offset != 0) || (offset + len != sizeof(ct_time))) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(&ct_time, buf, sizeof(ct_time));
	err = cts_cb->cts_time_write(&ct_time);
	if (err) {
		return BT_GATT_ERR(BT_CTS_ATT_ERR_VALUES_IGNORED);
	}

	err = bt_cts_send_notification(BT_CTS_UPDATE_REASON_MANUAL);
	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return len;
}

/* Current Time Service Declaration */
BT_GATT_SERVICE_DEFINE(cts_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_CTS),
		       BT_GATT_CHARACTERISTIC(BT_UUID_CTS_CURRENT_TIME,
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
						      BT_GATT_CHRC_NOTIFY,
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_ct,
					      write_ct, NULL),
		       BT_GATT_CCC(ct_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

int bt_cts_init(const struct bt_cts_cb *cb)
{
	__ASSERT(cb == NULL, "Current Time service need valid `struct bt_cts_cb` callback");
	__ASSERT(cb->fill_current_cts_time == NULL,
		 "`fill_current_cts_time` callback api is required for functioning of CTS");
	if (!cb || !cb->fill_current_cts_time) {
		return -EINVAL;
	}
	cts_cb = cb;
	return 0;
}

int bt_cts_send_notification(enum bt_cts_update_reason reason)
{
	int err;
	struct bt_cts_time_format ct_time;

	err = cts_cb->fill_current_cts_time(&ct_time);
	ct_time.reason = reason;
	if (err) {
		return err;
	}
	return bt_gatt_notify(NULL, &cts_svc.attrs[1], &ct_time, sizeof(ct_time));
}
