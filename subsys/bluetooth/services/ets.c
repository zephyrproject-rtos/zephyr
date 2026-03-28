/*
 * Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/ets.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_ets, CONFIG_BT_ETS_LOG_LEVEL);

/* ETS epoch: 2000-01-01 00:00:00
 * Unix epoch: 1970-01-01 00:00:00
 * Difference: 946,684,800 seconds
 */
#define ETS_EPOCH_OFFSET_SEC 946684800LL
#define ETS_EPOCH_OFFSET_MS  (ETS_EPOCH_OFFSET_SEC * MSEC_PER_SEC)

/* Resolutions for converting ETS units to milliseconds */
#define BT_ETS_MSEC_PER_SEC    MSEC_PER_SEC /* 1000 ms */
#define BT_ETS_MSEC_PER_100_MS 100U         /* 100 ms */
#define BT_ETS_MSEC_PER_1_MS   1U           /* 1 ms */
#define BT_ETS_100US_PER_MSEC  10U          /* 10 * 100us = 1 ms */

/* TZ/DST offset unit: 15 minutes per unit */
#define BT_ETS_TZ_DST_OFFSET_UNIT_MINUTES 15U

/* Maximum 48-bit value */
#define ETS_TIME_VALUE_MAX (0xFFFFFFFFFFFFULL)

/* ATT Application Error codes (Section 2.4. Attribute Protocol Application error codes) */
#define BT_ETS_ATT_ERR_TIME_SOURCE_QUALITY_TOO_LOW 0x80
#define BT_ETS_ATT_ERR_INCORRECT_TIME_FORMAT       0x81
/* Out of Range uses standard BT_ATT_ERR_OUT_OF_RANGE (0xFF) */

/**
 * @brief Time Characteristic format (Elapsed Time Service, v1.0 - Section 3.1.1)
 *
 */
struct ets_char_value {
	/* Elapsed Time data */
	struct bt_ets_elapsed_time et;
	/* The status of the server’s clock (@see @ref BT_ETS_CLOCK_STATUS). */
	uint8_t clock_status;
	/* The server’s clock capabilities (@see @ref BT_ETS_CLOCK_CAPABILITIES). */
	uint8_t clock_capabilities;
} __packed;

static const struct bt_ets_cb *ets_cb;

/* Static indication data */
static struct bt_gatt_indicate_params indicate_params;
static struct ets_char_value indicate_data;

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	if (err != 0) {
		LOG_WRN("Indication failed with error %d\n", err);
	} else {
		LOG_DBG("Indication sent successfully");
	}
}

/* Work item for sending indications */
static void send_indication_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	int err;
	uint8_t clock_status;

	if ((ets_cb == NULL) || (ets_cb->read_elapsed_time == NULL) ||
	    (ets_cb->read_clock_status == NULL)) {
		LOG_ERR("ETS callbacks not set for indication");
		return;
	}

	/* Read current elapsed time into static buffer */
	err = ets_cb->read_elapsed_time(&indicate_data.et);
	if (err != 0) {
		LOG_ERR("Failed to read elapsed time for indication: %d", err);
		return;
	}

	/* Read clock status */
	err = ets_cb->read_clock_status(&clock_status);
	if (err != 0) {
		LOG_ERR("Failed to read clock status for indication: %d", err);
		return;
	}

	/* Send indication using the static indicate_data buffer */
	err = bt_ets_indicate(&indicate_data.et, clock_status);
	if ((err != 0) && (err != -ENOTCONN)) {
		LOG_WRN("Failed to send indication: %d", err);
	} else {
		LOG_DBG("Indication sent successfully");
	}
}

K_WORK_DEFINE(send_indication_work, send_indication_work_handler);

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool indicate_enabled = (value == BT_GATT_CCC_INDICATE);

	LOG_DBG("ETS indications %s", indicate_enabled ? "enabled" : "disabled");

	if ((ets_cb != NULL) && (ets_cb->indication_changed != NULL)) {
		ets_cb->indication_changed(indicate_enabled);
	}

	/* Per ETS spec Sec. 3.1.2.2: on reconnect or non‑natural time change the server shall
	 * send an indication
	 */
	if (indicate_enabled) {
		k_work_submit(&send_indication_work);
	}
}

static ssize_t read_elapsed_time(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	int err;
	struct ets_char_value response;

	if ((ets_cb == NULL) || (ets_cb->read_elapsed_time == NULL) ||
	    (ets_cb->read_clock_status == NULL)) {
		LOG_ERR("ETS read callbacks not set");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = ets_cb->read_elapsed_time(&response.et);
	if (err != 0) {
		LOG_ERR("Read failed: %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = ets_cb->read_clock_status(&response.clock_status);
	if (err != 0) {
		LOG_ERR("ETS read_clock_status callback failed: %d", err);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	response.clock_capabilities = BT_ETS_CLOCK_CAPABILITIES_VALUE;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &response, sizeof(response));
}

#ifdef CONFIG_BT_ETS_CURRENT_ELAPSED_TIME_WRITABLE
static ssize_t write_elapsed_time(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_ets_elapsed_time et;
	uint64_t time_value;
	enum bt_ets_write_result result;

	if (!ets_cb || !ets_cb->write_elapsed_time) {
		LOG_ERR("callback is required, but not set");
		return BT_GATT_ERR(BT_ATT_ERR_WRITE_NOT_PERMITTED);
	}

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(et)) {
		LOG_ERR("Invalid write length: %u (expected %zu)", len, sizeof(et));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&et, buf, sizeof(et));

	/* Validate reserved bits (protocol-level check) */
	if (et.flags & BT_ETS_FLAG_RESERVED_MASK) {
		LOG_ERR("Reserved bits set in flags: 0x%02x", et.flags);
		return BT_GATT_ERR(BT_ETS_ATT_ERR_INCORRECT_TIME_FORMAT);
	}

	/* Validate time value range */
	time_value = sys_get_le48(et.time_value);
	if (time_value >= ETS_TIME_VALUE_MAX) {
		LOG_ERR("Time value out of range: %llu (max %llu)", time_value, ETS_TIME_VALUE_MAX);
		return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
	}

	/* Check resolution bits match configuration (static property) */
	if ((et.flags & BT_ETS_FLAG_RESOLUTION_MASK) !=
	    (BT_ETS_SUPPORTED_FLAGS_MASK & BT_ETS_FLAG_RESOLUTION_MASK)) {
		LOG_ERR("Invalid resolution in flags: 0x%02lx (expected: 0x%02lx)",
			et.flags & BT_ETS_FLAG_RESOLUTION_MASK,
			BT_ETS_SUPPORTED_FLAGS_MASK & BT_ETS_FLAG_RESOLUTION_MASK);
		return BT_GATT_ERR(BT_ETS_ATT_ERR_INCORRECT_TIME_FORMAT);
	}

	/* Check unsupported flags are not set (static properties) */
	if (et.flags & ~BT_ETS_SUPPORTED_FLAGS_MASK) {
		LOG_ERR("Unsupported flags set: 0x%02x (supported: 0x%02lx)", et.flags,
			BT_ETS_SUPPORTED_FLAGS_MASK);
		return BT_GATT_ERR(BT_ETS_ATT_ERR_INCORRECT_TIME_FORMAT);
	}

	result = ets_cb->write_elapsed_time(&et);

	/* Map application result to ATT error */
	switch (result) {
	case BT_ETS_WRITE_SUCCESS:
		/* Success - continue processing */
		break;
	case BT_ETS_WRITE_TIME_SOURCE_TOO_LOW:
		LOG_WRN("Time source quality too low");
		return BT_GATT_ERR(BT_ETS_ATT_ERR_TIME_SOURCE_QUALITY_TOO_LOW);
	case BT_ETS_WRITE_OUT_OF_RANGE:
		LOG_WRN("Time value out of range");
		return BT_GATT_ERR(BT_ATT_ERR_OUT_OF_RANGE);
	case BT_ETS_WRITE_INCORRECT_FORMAT:
		LOG_WRN("Incorrect time format (application validation)");
		return BT_GATT_ERR(BT_ETS_ATT_ERR_INCORRECT_TIME_FORMAT);
	default:
		LOG_ERR("Unknown write result: %d", result);
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	LOG_INF("ETS time written successfully");

	/* Report time change (Section 3.1.2.2): When the server changes its time after a write by a
	 * client to the Current Elapsed Time characteristic, the server shall send an indication of
	 * the Current Elapsed Time characteristic to other connected clients.
	 */
	k_work_submit(&send_indication_work);

	return len;
}
#endif /* CONFIG_BT_ETS_CURRENT_ELAPSED_TIME_WRITABLE */

/* Elapsed Time Service Declaration */

#ifdef CONFIG_BT_ETS_CURRENT_ELAPSED_TIME_WRITABLE
BT_GATT_SERVICE_DEFINE(ets_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_ETS),
		       BT_GATT_CHARACTERISTIC(BT_UUID_ETS_CURRENT_ELAPSED_TIME,
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE |
						      BT_GATT_CHRC_INDICATE,
					      BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
					      read_elapsed_time, write_elapsed_time, NULL),
		       BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));
#else
BT_GATT_SERVICE_DEFINE(ets_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_ETS),
		       BT_GATT_CHARACTERISTIC(BT_UUID_ETS_CURRENT_ELAPSED_TIME,
					      BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE,
					      BT_GATT_PERM_READ, read_elapsed_time, NULL, NULL),
		       BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));
#endif /* CONFIG_BT_ETS_CURRENT_ELAPSED_TIME_WRITABLE */

int bt_ets_init(const struct bt_ets_cb *cb)
{
	__ASSERT(cb != NULL, "Elapsed Time Service requires valid `struct bt_ets_cb` callback");
	__ASSERT(cb->read_elapsed_time != NULL, "`read_elapsed_time` callback is required for ETS");
	__ASSERT(cb->read_clock_status != NULL, "`read_clock_status` callback is required for ETS");

	if ((cb == NULL) || (cb->read_elapsed_time == NULL) || (cb->read_clock_status == NULL)) {
		LOG_ERR("Mandatory ETS callbacks are NULL");
		return -EINVAL;
	}

	ets_cb = cb;
	LOG_INF("ETS initialized");

	return 0;
}

int bt_ets_indicate(const struct bt_ets_elapsed_time *elapsed_time, uint8_t clock_status)
{
	int err;

	if (elapsed_time == NULL) {
		LOG_ERR("elapsed_time parameter is NULL");
		return -EINVAL;
	}

	memcpy(&indicate_data.et, elapsed_time, sizeof(indicate_data.et));
	indicate_data.clock_status = clock_status;
	indicate_data.clock_capabilities = BT_ETS_CLOCK_CAPABILITIES_VALUE;

	/* Setup indication parameters */
	indicate_params.attr = &ets_svc.attrs[1];
	indicate_params.func = indicate_cb;
	indicate_params.data = &indicate_data;
	indicate_params.len = sizeof(indicate_data);

	/* Send indication to all connected clients that have enabled indications */
	err = bt_gatt_indicate(NULL, &indicate_params);
	if (err != 0) {
		LOG_ERR("Failed to send indication: %d", err);
	}

	return err;
}

#ifdef CONFIG_BT_ETS_HELPER_API

int bt_ets_time_to_unix_ms(const struct bt_ets_elapsed_time *et_time, int64_t *unix_ms)
{
	uint64_t ets_time_value;
	int64_t ets_ms;
	uint8_t resolution;

	if ((et_time == NULL) || (unix_ms == NULL)) {
		LOG_ERR("Invalid NULL parameter(s)");
		return -EINVAL;
	}

	/* Decode the 48-bit time value */
	ets_time_value = sys_get_le48(et_time->time_value);

	/* Extract resolution from flags (bits 2-3) */
	resolution = (et_time->flags & BT_ETS_FLAG_RESOLUTION_MASK) >> 2;

	/* Convert to milliseconds based on resolution */
	switch (resolution) {
	case BT_ETS_RESOLUTION_1_SEC:
		/* 1 second resolution: multiply by 1000 */
		if (ets_time_value > (INT64_MAX / BT_ETS_MSEC_PER_SEC)) {
			LOG_ERR("Time value overflow during conversion");
			return -EOVERFLOW;
		}
		ets_ms = (int64_t)ets_time_value * BT_ETS_MSEC_PER_SEC;
		break;

	case BT_ETS_RESOLUTION_100_MS:
		/* 100 ms resolution: multiply by 100 */
		if (ets_time_value > (INT64_MAX / BT_ETS_MSEC_PER_100_MS)) {
			LOG_ERR("Time value overflow during conversion");
			return -EOVERFLOW;
		}
		ets_ms = (int64_t)ets_time_value * BT_ETS_MSEC_PER_100_MS;
		break;

	case BT_ETS_RESOLUTION_1_MS:
		/* 1 ms resolution: direct conversion */
		if (ets_time_value > INT64_MAX) {
			LOG_ERR("Time value overflow during conversion");
			return -EOVERFLOW;
		}
		ets_ms = (int64_t)ets_time_value;
		break;

	case BT_ETS_RESOLUTION_100_US:
		/* 100 us resolution: divide by 10 to get ms */
		ets_ms = (int64_t)(ets_time_value / BT_ETS_100US_PER_MSEC);
		break;

	default:
		LOG_ERR("Invalid resolution value: %u", resolution);
		return -EINVAL;
	}

	/* Convert from ETS epoch (2000) to Unix epoch (1970) */
	*unix_ms = ets_ms + ETS_EPOCH_OFFSET_MS;

	/* Apply TZ/DST offset based on time mode. */
	if ((et_time->flags & BT_ETS_FLAG_TZ_DST_USED) && !(et_time->flags & BT_ETS_FLAG_UTC)) {
		/* Local time mode: subtract offset to get UTC */
		*unix_ms -= (int64_t)et_time->tz_dst_offset * BT_ETS_TZ_DST_OFFSET_UNIT_MINUTES *
			    SEC_PER_MIN * MSEC_PER_SEC;
		LOG_DBG("Converted local time to UTC using offset: %d", et_time->tz_dst_offset);
	}

	LOG_DBG("ETS->Unix: ets_value=%llu resolution=%u ets_ms=%lld unix_ms=%lld flags=0x%02x",
		ets_time_value, resolution, ets_ms, *unix_ms, et_time->flags);

	return 0;
}

int bt_ets_time_from_unix_ms(struct bt_ets_elapsed_time *et_time, int64_t unix_ms, uint8_t time_src,
			     int8_t __maybe_unused tz_dst_offset)
{
	int64_t ets_ms;
	uint64_t ets_time_value;
	uint8_t flags = 0;

	if (et_time == NULL) {
		return -EINVAL;
	}

	/* Convert from Unix epoch (1970) to ETS epoch (2000) */
	ets_ms = unix_ms - ETS_EPOCH_OFFSET_MS;

	/* Apply offset for conversion in local time mode. */
#ifdef CONFIG_BT_ETS_SUPPORT_LOCAL_TIME
	ets_ms += (int64_t)tz_dst_offset * BT_ETS_TZ_DST_OFFSET_UNIT_MINUTES * SEC_PER_MIN *
		  MSEC_PER_SEC;
#endif

	/* Check if time is before ETS epoch */
	if (ets_ms < 0) {
		LOG_ERR("Time is before ETS epoch (2000-01-01)");
		return -EINVAL;
	}

	/* Convert from milliseconds to configured resolution */
#if defined(CONFIG_BT_ETS_RESOLUTION_1_SEC)
	/* 1 second resolution: divide by 1000 */
	ets_time_value = (uint64_t)(ets_ms / BT_ETS_MSEC_PER_SEC);
	flags |= 0; /* Resolution bits 2-3 = 00 */

#elif defined(CONFIG_BT_ETS_RESOLUTION_100_MS)
	/* 100 ms resolution: divide by 100 */
	ets_time_value = (uint64_t)(ets_ms / BT_ETS_MSEC_PER_100_MS);
	flags |= BIT(2); /* Resolution bits 2-3 = 01 */

#elif defined(CONFIG_BT_ETS_RESOLUTION_1_MS)
	/* 1 ms resolution: direct conversion */
	ets_time_value = (uint64_t)ets_ms;
	flags |= BIT(3); /* Resolution bits 2-3 = 10 */

#elif defined(CONFIG_BT_ETS_RESOLUTION_100_US)
	/* 100 us resolution: multiply by 10 */
	if (ets_ms > (INT64_MAX / BT_ETS_100US_PER_MSEC)) {
		LOG_ERR("Time value overflow during conversion");
		return -EOVERFLOW;
	}
	ets_time_value = (uint64_t)(ets_ms * BT_ETS_100US_PER_MSEC);
	flags |= (BIT(2) | BIT(3)); /* Resolution bits 2-3 = 11 */
#endif

	/* Check if value fits in 48 bits */
	if (ets_time_value > ETS_TIME_VALUE_MAX) {
		LOG_ERR("Time value exceeds 48-bit limit: %llu", ets_time_value);
		return -EOVERFLOW;
	}

	/* Set flags based on configuration (time-of-day mode only).
	 * These flags are STATIC per spec Section 3.1.2.1 - they are determined at
	 * compile-time and never change:
	 * - Bit 1: UTC flag (0=local time, 1=UTC) - static
	 * - Bits 2-3: Time resolution - static (set above based on CONFIG)
	 * - Bit 4: TZ/DST offset support - static (0=not supported, 1=supported)
	 * - Bit 5: Current timeline flag - always 1 when generating time
	 */
	if (IS_ENABLED(CONFIG_BT_ETS_SUPPORT_UTC)) {
		flags |= BT_ETS_FLAG_UTC;
	}

	if (IS_ENABLED(CONFIG_BT_ETS_SUPPORT_TZ_DST)) {
		flags |= BT_ETS_FLAG_TZ_DST_USED;
	}

	/* Always set current timeline flag */
	flags |= BT_ETS_FLAG_CURRENT_TIMELINE;

	/* Fill the structure */
	et_time->flags = flags;
	sys_put_le48(ets_time_value, et_time->time_value);
	et_time->time_sync_src = time_src;
	et_time->tz_dst_offset = IS_ENABLED(CONFIG_BT_ETS_SUPPORT_TZ_DST) ? tz_dst_offset : 0;

	LOG_DBG("Unix->ETS: unix_ms=%lld ets_ms=%lld value=%llu src=%d offset=%d flags=0x%02x",
		unix_ms, ets_ms, ets_time_value, time_src, et_time->tz_dst_offset, flags);

	return 0;
}

#endif /* CONFIG_BT_ETS_HELPER_API */
