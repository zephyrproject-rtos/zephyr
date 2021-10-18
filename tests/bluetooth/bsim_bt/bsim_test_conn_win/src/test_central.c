#ifdef __INTELLISENSE__
    #pragma diag_suppress 3365
#endif

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <bluetooth/conn.h>

#include "lll_test.h"

#define LATE_SENDING_INSIDE_WIN_US	9375
#define CONNECTION_INTERVAL_US		11250
#define US_UNITS_TO_625US_UNITS(us_units) ((uint16_t)(((uint32_t)(us_units))/625))
#define US_UNITS_TO_1250US_UNITS(us_units) ((US_UNITS_TO_625US_UNITS(us_units))/2)

enum invalid_conn_timing_test{
	EARLY_SENDING_1,
	LATE_SENDING_1,
	HCTO_TRUNCATE_1,
	EARLY_SENDING_2,
	LATE_SENDING_2,
	HCTO_TRUNCATE_2,
	NORMAL
};

struct bt_conn_win_test conn_win_test;

static struct bt_conn_le_create_param *create_param = BT_CONN_LE_CREATE_CONN;
static struct bt_le_conn_param *conn_param = BT_LE_CONN_PARAM_DEFAULT;
static struct bt_conn *default_conn;

static uint8_t expect_disconn = 0;
static uint8_t test_err = 0;


static void start_scan(void);

static void test_conn(const bt_addr_le_t *addr, struct bt_conn **conn)
{
	int err;
	static uint8_t i = 0;

	switch (i)
	{
	case EARLY_SENDING_1:
		/* CONNECTION TERMINATED BY LOCAL HOST (0x16) */
		expect_disconn = 0x16;
		conn_win_test.tr_win_offset = 0;
		conn_win_test.win_size = 1;
		conn_win_test.pkt_conn_delay_us = 0;
		printk("EARLY_SENDING_1 \n");
		break;

	case LATE_SENDING_1:
		expect_disconn = 0x16;
		conn_win_test.tr_win_offset = 0;
		conn_win_test.win_size = 8;
		conn_win_test.pkt_conn_delay_us = LATE_SENDING_INSIDE_WIN_US;
		printk("LATE_SENDING_1 \n");
		break;


	case HCTO_TRUNCATE_1:
		/* CONN FAILED TO BE ESTABLISHED /SYNCHRONIZATION TIMEOUT (0x3E) */
		expect_disconn = 0x3E;
		conn_win_test.pkt_conn_delay_us = CONNECTION_INTERVAL_US;
		printk("HCTO_TRUNCATE_1 \n");
		break;


	case EARLY_SENDING_2:
		expect_disconn = 0x16;
		conn_win_test.tr_win_offset = 8;
		conn_win_test.win_size = 1;
		conn_win_test.pkt_conn_delay_us = 0;
		printk("EARLY_SENDING_2 \n");
		break;

	case LATE_SENDING_2:
		expect_disconn = 0x16;
		conn_win_test.tr_win_offset = 8;
		conn_win_test.win_size = 8;
		conn_win_test.pkt_conn_delay_us = LATE_SENDING_INSIDE_WIN_US;
		printk("LATE_SENDING_2 \n");
		break;

	case HCTO_TRUNCATE_2:
		expect_disconn = 0x3E;
		conn_win_test.pkt_conn_delay_us = CONNECTION_INTERVAL_US;
		printk("HCTO_TRUNCATE_2 \n");
		break;

	case NORMAL:
		expect_disconn = 0x16;
		conn_win_test.tr_win_offset = 0;
		conn_win_test.win_size = 1;
		conn_win_test.pkt_conn_delay_us = 0;
		printk("NORMAL \n");
		break;

	default:
		return;
		break;
	}

	err = bt_conn_le_create(addr, create_param,	conn_param, &default_conn);
	if (err) {
		printk("Create conn to failed (%u)\n", err);
		start_scan();
	}

	i++;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	if (default_conn) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND &&
	    type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	if (rssi < -50) {
		return;
	}

	if (bt_le_scan_stop()) {
		return;
	}

	test_conn(addr, &default_conn);
}

static void start_scan(void)
{
	int err;

	/* This demo doesn't require active scan */
	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n Wait 100ms\n", addr);

	k_sleep(K_MSEC(100));

	printk("Wake up\n");

	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Central Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;

	if(reason != expect_disconn){
		test_err = reason;

		printk("Test fail at disconnection:  (expected disconnection 0x%02x) \
					insteed of(reason 0x%02x)\n", expect_disconn, reason);
		return;
	}

	start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int init_central(void)
{
	conn_param->interval_min = US_UNITS_TO_1250US_UNITS(8000);
	conn_param->interval_max = US_UNITS_TO_1250US_UNITS(CONNECTION_INTERVAL_US);
	conn_win_test.pkt_conn_delay_us = 0;
	conn_win_test.skip_pkt = 5;
	conn_win_test.conn_interval_us = CONNECTION_INTERVAL_US;

	test_err = bt_enable(NULL);
	if (test_err) {
		printk("Bluetooth init failed (err %d)\n", test_err);
		return test_err;
	}

	printk("Bluetooth initialized\n");

	start_scan();

	k_sleep(K_MSEC(2500));

	return test_err;
}
