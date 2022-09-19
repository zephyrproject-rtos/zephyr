/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/direction.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_vs.h>

/* Latency set to zero, to enforce PDU exchange every connection event */
#define CONN_LATENCY 0U
/* Arbitrary selected timeout value */
#define CONN_TIMEOUT 400U
/* Interval used to run CTE request procedure periodically.
 * Value is a number of connection events.
 */
#define CTE_REQ_INTERVAL (CONN_LATENCY + 10U)
/* Length of CTE in unit of 8 us */
#define CTE_LEN (0x14U)

#define DF_FEAT_ENABLED BIT64(BT_LE_FEAT_BIT_CONN_CTE_RESP)

static struct bt_conn *default_conn;
static const struct bt_le_conn_param conn_params = BT_LE_CONN_PARAM_INIT(
	BT_GAP_INIT_CONN_INT_MIN, BT_GAP_INIT_CONN_INT_MAX, CONN_LATENCY, CONN_TIMEOUT);

#if defined(CONFIG_BT_DF_CTE_RX_AOA)
static const uint8_t ant_patterns[] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA };
#endif /* CONFIG_BT_DF_CTE_RX_AOA */

static void start_scan(void);

static const char *cte_type2str(uint8_t type)
{
	switch (type) {
	case BT_DF_CTE_TYPE_AOA:
		return "AOA";
	case BT_DF_CTE_TYPE_AOD_1US:
		return "AOD 1 [us]";
	case BT_DF_CTE_TYPE_AOD_2US:
		return "AOD 2 [us]";
	default:
		return "Unknown";
	}
}

static const char *sample_type2str(enum bt_df_iq_sample type)
{
	switch (type) {
	case BT_DF_IQ_SAMPLE_8_BITS_INT:
		return "8 bits int";
	case BT_DF_IQ_SAMPLE_16_BITS_INT:
		return "16 bits int";
	default:
		return "Unknown";
	}
}

static const char *packet_status2str(uint8_t status)
{
	switch (status) {
	case BT_DF_CTE_CRC_OK:
		return "CRC OK";
	case BT_DF_CTE_CRC_ERR_CTE_BASED_TIME:
		return "CRC not OK, CTE Info OK";
	case BT_DF_CTE_CRC_ERR_CTE_BASED_OTHER:
		return "CRC not OK, Sampled other way";
	case BT_DF_CTE_INSUFFICIENT_RESOURCES:
		return "No resources";
	default:
		return "Unknown";
	}
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	uint64_t u64 = 0U;
	int err;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	switch (data->type) {
	case BT_DATA_LE_SUPPORTED_FEATURES:
		if (data->data_len > sizeof(u64)) {
			return true;
		}

		(void)memcpy(&u64, data->data, data->data_len);

		u64 = sys_le64_to_cpu(u64);

		if (!(u64 & DF_FEAT_ENABLED)) {
			return true;
		}

		err = bt_le_scan_stop();
		if (err) {
			printk("Stop LE scan failed (err %d)\n", err);
			return true;
		}

		err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, &conn_params, &default_conn);
		if (err) {
			printk("Create conn failed (err %d)\n", err);
			start_scan();
		}
		return false;
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n", dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_GAP_ADV_TYPE_ADV_IND || type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static void enable_cte_reqest(void)
{
	int err;

	const struct bt_df_conn_cte_rx_param cte_rx_params = {
#if defined(CONFIG_BT_DF_CTE_RX_AOA)
		.cte_types = BT_DF_CTE_TYPE_ALL,
		.slot_durations = 0x2,
		.num_ant_ids = ARRAY_SIZE(ant_patterns),
		.ant_ids = ant_patterns,
#else
		.cte_types = BT_DF_CTE_TYPE_AOD_1US | BT_DF_CTE_TYPE_AOD_2US,
#endif /* CONFIG_BT_DF_CTE_RX_AOA */
	};

	const struct bt_df_conn_cte_req_params cte_req_params = {
		.interval = CTE_REQ_INTERVAL,
		.cte_length = CTE_LEN,
#if defined(CONFIG_BT_DF_CTE_RX_AOA)
		.cte_type = BT_DF_CTE_TYPE_AOA,
#else
		.cte_type = BT_DF_CTE_TYPE_AOD_2US,
#endif /* CONFIG_BT_DF_CTE_RX_AOA */
	};

	printk("Enable receiving of CTE...\n");
	err = bt_df_conn_cte_rx_enable(default_conn, &cte_rx_params);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success. CTE receive enabled.\n");

	printk("Request CTE from peer device...\n");
	err = bt_df_conn_cte_req_enable(default_conn, &cte_req_params);
	if (err) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success. CTE request enabled.\n");
}

static void start_scan(void)
{
	int err;

	/* Use active scanning and disable duplicate filtering to handle any
	 * devices that might update their advertising data at runtime.
	 */
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	printk("Connected: %s\n", addr);

	if (conn == default_conn) {
		enable_cte_reqest();
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	start_scan();
}

static void cte_recv_cb(struct bt_conn *conn, struct bt_df_conn_iq_samples_report const *report)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (report->err == BT_DF_IQ_REPORT_ERR_SUCCESS) {
		printk("CTE[%s]: samples type: %s, samples count %d, cte type %s, slot durations: "
			"%u [us], packet status %s, RSSI %i\n", addr,
			sample_type2str(report->sample_type), report->sample_count,
			cte_type2str(report->cte_type), report->slot_durations,
			packet_status2str(report->packet_status), report->rssi);

		if (IS_ENABLED(CONFIG_DF_CENTRAL_APP_IQ_REPORT_PRINT_IQ_SAMPLES)) {
			for (uint8_t idx = 0; idx < report->sample_count; idx++) {
				if (report->sample_type == BT_DF_IQ_SAMPLE_8_BITS_INT) {
					printk(" IQ[%d]: %d, %d\n", idx, report->sample[idx].i,
					       report->sample[idx].q);
				} else if (IS_ENABLED(
					CONFIG_BT_DF_VS_CONN_IQ_REPORT_16_BITS_IQ_SAMPLES)) {
					printk(" IQ[%" PRIu8 "]: %d, %d\n", idx,
					       report->sample16[idx].i, report->sample16[idx].q);
				} else {
					printk("Unhandled vendor specific IQ samples type\n");
					break;
				}
			}
		}
	} else {
		printk("CTE[%s]: request failed, err %u\n", addr, report->err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.cte_report_cb = cte_recv_cb,
};

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	start_scan();
}
