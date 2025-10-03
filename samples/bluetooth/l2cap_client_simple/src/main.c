/* samples/bluetooth/l2cap_client_simple/src/main.c
 *
 * Minimal L2CAP client: scans, connects, opens a channel and sends messages.
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/printk.h>
#include <zephyr.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/sys/byteorder.h>

/* Use same PSM as server sample */
#define PSM              0x29
#define SEND_INTERVAL_MS 2000

static struct bt_conn *default_conn;

/* Forward declarations */
static void start_scan(void);

/* L2CAP channel ops and channel object used for client-initiated connection */
static int client_chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf);
static void client_chan_connected(struct bt_l2cap_chan *chan);
static void client_chan_disconnected(struct bt_l2cap_chan *chan);

static struct bt_l2cap_chan_ops client_ops = {
	.recv = client_chan_recv,
	.connected = client_chan_connected,
	.disconnected = client_chan_disconnected,
};

/* Use a transport-specific channel type for LE */
static struct bt_l2cap_le_chan client_chan = {
	.chan.ops = &client_ops,
};

/* Send a small message periodically over the channel */
static void send_task(struct bt_l2cap_chan *chan)
{
	int count = 0;
	while (chan->state == BT_L2CAP_CONNECTED) {
		const char *msg = "Hello from client\n";
		struct net_buf *buf;

		/* Reserve required header space for L2CAP SDU (stack requires reserve) */
		buf = net_buf_alloc(bt_l2cap_get_tx_pool(chan), K_NO_WAIT);
		if (!buf) {
			printk("Failed to allocate tx buffer\n");
			k_sleep(K_MSEC(SEND_INTERVAL_MS));
			continue;
		}

		/* Ensure necessary reserve for L2CAP header (use helper macro in your Zephyr
		 * version) */
		net_buf_add_mem(buf, msg, strlen(msg));

		/* Try send; bt_l2cap_chan_send may block; check return code */
		if (bt_l2cap_chan_send(chan, buf) < 0) {
			printk("bt_l2cap_chan_send failed\n");
			net_buf_unref(buf);
		}

		count++;
		k_sleep(K_MSEC(SEND_INTERVAL_MS));
	}
}

/* Implement callbacks */
static int client_chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	printk("Client received %u bytes\n", buf->len);
	/* For synchronous processing, return 0 and let stack free buffer */
	return 0;
}

static void client_chan_connected(struct bt_l2cap_chan *chan)
{
	printk("Client: L2CAP channel connected\n");
	/* Launch a new thread to send periodic messages (so we don't block callback) */
	k_thread_create(
		&(struct k_thread){},
		K_THREAD_STACK_SIZEOF((K_THREAD_STACK_DEFINE(stack_area, 1024), stack_area)),
		(k_thread_entry_t)send_task, chan, NULL, NULL, K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
}

static void client_chan_disconnected(struct bt_l2cap_chan *chan)
{
	printk("Client: L2CAP channel disconnected\n");
}

/* Connection callbacks for central role */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err %u)\n", err);
		bt_conn_unref(conn);
		start_scan();
		return;
	}

	printk("Connected\n");
	default_conn = bt_conn_ref(conn);

	/* Now initiate L2CAP channel connect on this connection */
	int rc = bt_l2cap_chan_connect(default_conn, &client_chan.chan, PSM);
	if (rc) {
		printk("bt_l2cap_chan_connect failed: %d\n", rc);
	} else {
		printk("bt_l2cap_chan_connect initiated\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason %u)\n", reason);
	if (default_conn) {
		bt_conn_unref(default_conn);
		default_conn = NULL;
	}
	/* Restart scanning to find another peer */
	start_scan();
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Found device: %s (RSSI %d)\n", addr_str, rssi);

	/* Stop scanning and connect to first found device */
	bt_le_scan_stop();

	struct bt_le_conn_param *param = BT_LE_CONN_PARAM_DEFAULT;
	int rc = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, param, &default_conn);
	if (rc) {
		printk("bt_conn_le_create failed: %d\n", rc);
		start_scan();
	} else {
		printk("Connecting to %s ...\n", addr_str);
	}
}

static void start_scan(void)
{
	int err;
	struct bt_le_scan_param scan_param = {
		.type = BT_HCI_LE_SCAN_ACTIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
	} else {
		printk("Scanning started\n");
	}
}

void main(void)
{
	int err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed: %d\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	bt_conn_cb_register(&conn_callbacks);

	start_scan();
}
