/*
 * Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/ets.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>

/* Clock status: indicates if time needs to be set */
static uint8_t clock_status = BT_ETS_CLOCK_STATUS_NEEDS_SET;
/* Timezone/DST offset in 15-minute units */
static int8_t tz_dst_offset;

/* ETS epoch: 2000-01-01 00:00:00 */
#define ETS_EPOCH_UNIX_MS 946684800000LL

/* Advertising data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_ETS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void advertise(struct k_work *work)
{
	ARG_UNUSED(work);
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err != 0) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static K_WORK_DEFINE(advertise_work, advertise);

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s (err %u)\n", addr, err);
		return;
	}

	printk("Connected: %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));
}

static void on_conn_recycled(void)
{
	k_work_submit(&advertise_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = on_conn_recycled,
};

/* Read current time and convert to ETS format */
static int read_elapsed_time(struct bt_ets_elapsed_time *elapsed_time)
{
	struct timespec ts;
	int64_t unix_ms;
	int err;

	/* Get current wall clock time (Unix epoch 1970-01-01) */
	err = sys_clock_gettime(SYS_CLOCK_REALTIME, &ts);
	if (err != 0) {
		printk("Failed to get realtime clock: %d\n", err);
		return err;
	}

	/* Convert to milliseconds */
	unix_ms = ts.tv_sec * MSEC_PER_SEC + ts.tv_nsec / NSEC_PER_MSEC;

	/* If clock hasn't been set yet, use ETS epoch as default */
	if (unix_ms < ETS_EPOCH_UNIX_MS) {
		unix_ms = ETS_EPOCH_UNIX_MS;
	}

	/* Convert Unix time to ETS format */
	err = bt_ets_time_from_unix_ms(elapsed_time, unix_ms, BT_ETS_TIME_SOURCE_MANUAL,
				       tz_dst_offset);
	if (err != 0) {
		printk("Failed to convert time: %d\n", err);
		return err;
	}

	return 0;
}

/* Handle time write from client */
static enum bt_ets_write_result write_elapsed_time(const struct bt_ets_elapsed_time *elapsed_time)
{
	struct timespec ts;
	int64_t unix_ms;
	int err;

	/* Convert ETS time to Unix milliseconds (UTC) */
	err = bt_ets_time_to_unix_ms(elapsed_time, &unix_ms);
	if (err != 0) {
		printk("Failed to decode time: %d\n", err);
		return BT_ETS_WRITE_INCORRECT_FORMAT;
	}

	/* Set the system realtime clock */
	ts.tv_sec = unix_ms / MSEC_PER_SEC;
	ts.tv_nsec = (unix_ms % MSEC_PER_SEC) * NSEC_PER_MSEC;

	err = sys_clock_settime(SYS_CLOCK_REALTIME, &ts);
	if (err != 0) {
		printk("Failed to set realtime clock: %d\n", err);
		return BT_ETS_WRITE_OUT_OF_RANGE;
	}

	/* Store timezone/DST offset if provided */
	tz_dst_offset = elapsed_time->tz_dst_offset;

	/* Clear "clock needs to be set" flag after successful write */
	clock_status &= ~BT_ETS_CLOCK_STATUS_NEEDS_SET;

	/* Log the time update with human-readable format */
	time_t unix_sec = unix_ms / MSEC_PER_SEC;
	struct tm const *timeinfo = gmtime(&unix_sec);
	char time_str[64];

	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
	printk("Time updated: %s UTC (source: %d)\n", time_str, elapsed_time->time_sync_src);

	return BT_ETS_WRITE_SUCCESS;
}

static int read_clock_status(uint8_t *status)
{
	*status = clock_status;
	return 0;
}

static void indication_changed(bool enabled)
{
	printk("ETS indications %s\n", enabled ? "enabled" : "disabled");
}

static const struct bt_ets_cb ets_callbacks = {
	.read_elapsed_time = read_elapsed_time,
	.write_elapsed_time = write_elapsed_time,
	.read_clock_status = read_clock_status,
	.indication_changed = indication_changed,
};

static void bt_ready(int err)
{
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	k_work_submit(&advertise_work);
}

int main(void)
{
	int err;

	printk("Bluetooth Elapsed Time Service Peripheral Sample\n");

#if defined(CONFIG_BT_ETS_SUPPORT_UTC)
	printk("Mode: UTC time\n");
#elif defined(CONFIG_BT_ETS_SUPPORT_LOCAL_TIME)
	printk("Mode: Local time\n");
#endif

#if defined(CONFIG_BT_ETS_SUPPORT_TZ_DST)
	printk("TZ/DST offset: Supported\n");
#endif

	/* Initialize ETS */
	err = bt_ets_init(&ets_callbacks);
	if (err != 0) {
		printk("ETS init failed (err %d)\n", err);
		return 0;
	}

	printk("ETS initialized successfully\n");

	/* Initialize Bluetooth */
	err = bt_enable(bt_ready);
	if (err != 0) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}
	return 0;
}
