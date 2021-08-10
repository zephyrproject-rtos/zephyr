/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/addr.h>
#include <net/buf.h>
#include <stdlib.h>

// TODO: Make this configurable

// we use one connection for advertisements while staying connected
#define L2CAP_MESH_MAX_CONN (CONFIG_BT_MAX_CONN - 1)
#define L2CAP_MESH_UUID 0xFFFF
#define L2CAP_MESH_PSM 0x6c
#define L2CAP_MESH_PARALLEL_BUFFERS 1
#define NB_BLE_DEBUG 0
#define TIMEOUT_WARNING_MS 10000

// Our mesh connection management object
struct link {
	struct bt_conn *conn; // the reference to the zephyr connetion
	struct k_sem destroy_sem; // this semaphore
	struct bt_l2cap_le_chan le_chan; // space for our channel
	bool chan_connected; // whether the channel is connected or not
	uint64_t bytes_sent; // the amount of bytes sent
	uint64_t bytes_received; // the amount of bytes received

	// some event timestamps for easier testing and debugging
	uint64_t connect_ts; // us that the connection was established
	uint64_t disconnect_ts; // us that the disconnect happened
	uint64_t channel_up_ts; // us that the channel was established
	uint64_t channel_down_ts; // us that the channel went down
	uint64_t rx_ts; // the last us to receive bytes
	uint64_t tx_ts; // the last us to send bytes
};

/*
 * Some Event Handling
 */
// we simply reuse own and other address strings for convenience
// This however also means, that the on_ methods should not be called simultaneously! (i.e. with locked active_lists mutex)
static char own_addr_str[BT_ADDR_LE_STR_LEN];
static char other_addr_str[BT_ADDR_LE_STR_LEN];

static void format_link_recipient_addr(struct link *link)
{
	struct bt_conn_info info;

	if (!bt_conn_get_info(link->conn, &info)) {
		bt_addr_le_to_str(info.le.remote, other_addr_str, sizeof(other_addr_str));
	} else {
		other_addr_str[0] = '?';
		other_addr_str[1] = '\0';
	}
}

static void on_tx(struct link *link)
{
	link->tx_ts = k_uptime_get();
	format_link_recipient_addr(link);

	printk("on_tx | %s | %s | \n", own_addr_str, other_addr_str);
}

static void on_rx(struct link *link)
{
	link->rx_ts = k_uptime_get();
	format_link_recipient_addr(link);
	int avg_kbits_per_sec =
		(link->bytes_received * 8) / MAX(1, (link->rx_ts - link->channel_up_ts));
	printk("on_rx | %s | %s | %d kbps\n", own_addr_str, other_addr_str, avg_kbits_per_sec);
}

static void on_connect(struct link *link)
{
	link->connect_ts = k_uptime_get();
	format_link_recipient_addr(link);

	printk("on_connect | %s | %s | \n", own_addr_str, other_addr_str);
}

static void on_disconnect(struct link *link)
{
	link->disconnect_ts = k_uptime_get();
	format_link_recipient_addr(link);

	printk("on_disconnect | %s | %s | \n", own_addr_str, other_addr_str);
}

static void on_channel_up(struct link *link)
{
	link->channel_up_ts = k_uptime_get();
	format_link_recipient_addr(link);

	printk("on_channel_up | %s | %s | \n", own_addr_str, other_addr_str);
}

static void on_channel_down(struct link *link)
{
	link->channel_down_ts = k_uptime_get();
	format_link_recipient_addr(link);

	printk("on_channel_down | %s | %s | \n", own_addr_str, other_addr_str);
}

struct link *active_links[L2CAP_MESH_MAX_CONN];
uint8_t num_active_links;

// A little mutex to prevent access by the main thread while e.g. deleting entries from the list
K_MUTEX_DEFINE(active_links_mutex);

static void nb_ble_start(void);

void safely_add_active_link(struct link *link)
{
	k_mutex_lock(&active_links_mutex, K_FOREVER);
	__ASSERT(num_active_links < L2CAP_MESH_MAX_CONN, "Too many active links!");

	on_connect(link); // TODO: This is not the right place for that...

	active_links[num_active_links] = link;
	num_active_links++;
	k_mutex_unlock(&active_links_mutex);
}

struct link *find_active_link_by_connection(struct bt_conn *conn)
{
	struct link *link = NULL;
	for (int i = 0; i < num_active_links; i++) {
		if (active_links[i]->conn == conn) {
			link = active_links[i];
			break;
		}
	}
	return link;
}

void remove_active_link(struct link *link)
{
	for (int i = 0; i < num_active_links; i++) {
		if (active_links[i] == link) {
			// we first swap the links so we can safely delete the one at the end
			if (i != num_active_links - 1) {
				active_links[i] = active_links[num_active_links - 1];
			}
			active_links[num_active_links - 1] = NULL;
			num_active_links--;
			break;
		}
	}
}

static void chan_connected_cb(struct bt_l2cap_chan *chan)
{
	k_mutex_lock(&active_links_mutex, K_FOREVER);
	struct link *link = find_active_link_by_connection(chan->conn);
	if (link != NULL) {
		link->chan_connected = true;
		on_channel_up(link);
	} else {
		printk("Could not find link in chan_connected_cb\n");
	}
	k_mutex_unlock(&active_links_mutex);
}

static void chan_disconnected_cb(struct bt_l2cap_chan *chan)
{
	k_mutex_lock(&active_links_mutex, K_FOREVER);
	struct link *link = find_active_link_by_connection(chan->conn);
	if (link != NULL) {
		link->chan_connected = false;
		on_channel_down(link);
	} else {
		printk("Could not find link in chan_disconnected_cb\n");
	}
	k_mutex_unlock(&active_links_mutex);
}

static int chan_recv_cb(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
	size_t num_bytes_received = net_buf_frags_len(buf);
	k_mutex_lock(&active_links_mutex, K_FOREVER);
	struct link *link = find_active_link_by_connection(chan->conn);
	if (link != NULL) {
		link->bytes_received += num_bytes_received;
		//printk("Received %d bytes\n", num_bytes_received);
		on_rx(link);
	} else {
		printk("Could not find link in chan_recv_cb\n");
	}
	k_mutex_unlock(&active_links_mutex);
	return 0;
}

int chan_accept(struct bt_conn *conn, struct bt_l2cap_chan **chan)
{
	k_mutex_lock(&active_links_mutex, K_FOREVER);
	struct link *link = find_active_link_by_connection(conn);
	int ret = 0;
	if (link != NULL) {
		link->chan_connected = false;
		*chan = &link->le_chan.chan;
	} else {
		printk("Could not find link in chan_disconnected_cb\n");
		*chan = NULL;
		ret = -EACCES;
	}
	k_mutex_unlock(&active_links_mutex);
	return ret;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct link *link = NULL;

	// we can check the num_active links as this function is the only one that adds links
	if (!err) {
		if (num_active_links == L2CAP_MESH_MAX_CONN) {
			printk("No more free links!\n");
			goto fail;
		}
		link = malloc(sizeof(struct link));

		if (link != NULL) {
			memset(link, 0, sizeof(struct link));
			static struct bt_l2cap_chan_ops chan_ops = { .connected = chan_connected_cb,
								     .disconnected =
									     chan_disconnected_cb,
								     .recv = chan_recv_cb };
			link->le_chan.chan.ops = &chan_ops;

			link->conn = bt_conn_ref(conn);

			struct bt_conn_info info;
			if (!bt_conn_get_info(link->conn, &info)) {
				if (info.role == BT_CONN_ROLE_MASTER) {
					int err = bt_l2cap_chan_connect(
						link->conn, &link->le_chan.chan, L2CAP_MESH_PSM);
					if (err) {
						// we disconnect, this will eventually also clear this link
						printk("Could not connect to channel!\n");
						goto fail;
					} else {
						safely_add_active_link(link);
					}
				} else {
					// NOOP, we simply await a connection...
					safely_add_active_link(link);
				}
			} else {
				// disconnect handler will free link
				printk("Failed to get connection info!\n");
				goto fail;
			}
		} else {
			// seems like we don't have enough memory to connect :(
			printk("Could not allocate link to connect\n");
			goto fail;
		}
	} else {
		printk("BLE Connection failed (err 0x%02x)\n", err);
	}

	return;
fail:
	if (link) {
		if (link->conn) {
			bt_conn_unref(conn);
			link->conn = NULL;
		}
		free(link);
	}
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	k_mutex_lock(&active_links_mutex, K_FOREVER);

	struct link *link = find_active_link_by_connection(conn);

	if (link != NULL) {
		remove_active_link(link);
		on_disconnect(link);
		bt_conn_unref(link->conn);
		free(link); // free memory again
	} else {
		printk("Could not find link for connection!\n");
	}

	k_mutex_unlock(&active_links_mutex);
}

/*
 * Advertisement Handling
 */
static void device_found_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			    struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	//printk("Device found: %s (RSSI %d)\n", addr_str, rssi);

	/* connect only to devices in close proximity */
	// TODO: Make this configurable!
	if (rssi < -70) {
		return;
	}

	// we also check if we reached our limit of active links
	// (so we do not initialized connections we can not handle)
	if (num_active_links == L2CAP_MESH_MAX_CONN) {
		return;
	}

	// we first check if we have a connection to this peer
	struct bt_conn *conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);

	if (conn != NULL) {
		// we already have connection -> abort
		bt_conn_unref(conn);
		return;
	}
	// we do not have a connection yet -> we try to initialize it, IF we are the one with the "bigger" mac address!
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = 1;
	bt_id_get(addrs, &count);

	__ASSERT(count > 0, "BT_ID_DEFAULT not available");

	if (bt_addr_le_cmp(&addrs[BT_ID_DEFAULT], addr) <= 0) {
		return; // our addr is smaller :( but we expect a connection from the other node \o/
	}

	// we not stop the scan process to create the connection
	if (bt_le_scan_stop()) {
		return;
	}

	conn = NULL;
	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &conn);
	if (err) {
		printk("Create conn to %s failed (%u)\n", addr_str, err);
		nb_ble_start(); // try to start the scan immediately
	} else {
		bt_conn_unref(conn); // we ref the connection later again
	}
}

static void nb_ble_start()
{
	int err = 0;

	size_t data_len = 2; // 2 byte uuid
	char *data = malloc(data_len);

	// Store the UUID in little endian format
	*data = L2CAP_MESH_UUID & 0xFF;
	*(data + 1) = (L2CAP_MESH_UUID >> 8) & 0xFF;

	struct bt_data ad[] = { BT_DATA_BYTES(BT_DATA_FLAGS,
					      (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
				BT_DATA(BT_DATA_SVC_DATA16, data, data_len) };

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);

	free(data);

#if NB_BLE_DEBUG
	if (err) {
		printk("NB BLE: advertising failed to start (err %d)\n", err);
	}
#endif

	/* Use active scanning and disable duplicate filtering to handle any
	* devices that might update their advertising data at runtime. */
	struct bt_le_scan_param scan_param = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_NONE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL, // TODO: Adapt interval and window
		.window = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found_cb);

#if NB_BLE_DEBUG
	if (err) {
		printk("NB BLE: Scanning failed to start (err %d)\n", err);
	}
#endif
}

NET_BUF_POOL_DEFINE(tx_data_pool, L2CAP_MESH_PARALLEL_BUFFERS,
		    BT_L2CAP_CHAN_SEND_RESERVE + CONFIG_BT_L2CAP_TX_MTU, 0, NULL);

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	struct bt_l2cap_server l2cap_server;

	memset(&l2cap_server, 0, sizeof(struct bt_l2cap_server));

	l2cap_server.psm = L2CAP_MESH_PSM;
	l2cap_server.accept = chan_accept;

	err = bt_l2cap_server_register(&l2cap_server);

	if (err) {
		printk("L2CAP server init failed (err %d)\n", err);
		return;
	}

	// we do not have a connection yet -> we try to initialize it, IF we are the one with the "bigger" mac address!
	bt_addr_le_t own_addr;
	size_t count = 1;
	bt_id_get(&own_addr, &count);
	bt_addr_le_to_str(&own_addr, own_addr_str, sizeof(own_addr_str));
	printk("Own address: %s\n", own_addr_str);

	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};

	bt_conn_cb_register(&conn_callbacks);

	char payload[CONFIG_BT_L2CAP_TX_MTU];

	int cur_index = 0;
	while (true) {
		nb_ble_start();
		bool worked = false;

		k_mutex_lock(&active_links_mutex, K_FOREVER);

		if (num_active_links > 0) {
			// we check first as the number could have changed
			if (cur_index >= num_active_links) {
				cur_index = 0;
			}

			struct link *link = active_links[cur_index];
			cur_index++; // Note that the actual order of the links might change, so this is not totally "fair"

			if (link->chan_connected && atomic_get(&link->le_chan.tx.credits)) {
				// chan is connected and we have credits available!

				// now try to get a free buffer
				uint32_t size = link->le_chan.tx.mtu;
				struct net_buf *buf =
					net_buf_alloc_len(&tx_data_pool,
							  BT_L2CAP_CHAN_SEND_RESERVE + size,
							  K_NO_WAIT);

				if (buf) {
					net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
					net_buf_add_mem(buf, payload, size);

					int ret = bt_l2cap_chan_send(&link->le_chan.chan, buf);

					if (ret >= 0) {
						link->bytes_sent += size;
						on_tx(link);
						worked = true;
					} else {
						net_buf_unref(buf);
						if (ret != -EAGAIN) {
							printk("Error: could not send packet %d\n",
							       ret);
							//bt_conn_disconnect(link->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
						}
					}
				} else {
					printk("Error: could not allocate buffer\n");
				}
			}

#if TIMEOUT_WARNING_MS > 0
			uint64_t now = k_uptime_get();

			if (link->connect_ts + TIMEOUT_WARNING_MS < now) {
				// connection is old enough!
				if (!link->channel_up_ts) {
					printk("Timeout warning: channel is still not up!\n");
				} else {
					if (link->tx_ts + TIMEOUT_WARNING_MS < now) {
						printk("Timeout warning: tx timeout!\n");
					}
					if (link->rx_ts + TIMEOUT_WARNING_MS < now) {
						printk("Timeout warning: rx timeout!\n");
					}
				}
			}

			if (link->channel_down_ts &&
			    link->channel_down_ts + TIMEOUT_WARNING_MS < now) {
				printk("Timeout warning: channel_down timeout!\n");
			}

			if (link->disconnect_ts && link->disconnect_ts + TIMEOUT_WARNING_MS < now) {
				printk("Timeout warning: disconnect timeout!\n");
			}
#endif
		}

		if (!worked) {
			k_msleep(10);
		}

		k_mutex_unlock(&active_links_mutex);
	}
}
