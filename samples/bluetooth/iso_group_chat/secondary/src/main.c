/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************************
 * Group Chat Feature: Secondary device
 * 
 *   See ../../README.rst for more details
 * 
 * --------------------------------------------------------------------------------------
 * 
 *   This file contains the source code for the secondary device.
 *   The secondary device is responsible for receiving audio data from the primary
 *   device on BIS1 and sending audio data on BIS2.
 * 
 *   This file used the main.c from the mixer sample as a reference which in turn was
 *   based on the iso_receive sample.
 * 
 *   At the moment, given the core does not have the capability to send on BIS2, the
 *   secondary device will only receive on BIS1 and print the BIS data every 50 frames.
 *   Once the core has the capability to send on BIS2, the secondary devicecan send data
 *   on BIS2 by enableing the define ENABLE_SEND_ON_BIS2.
 * 
 * --------------------------------------------------------------------------------------
 * 
 *   Testing: Use the Nordic nrf52840dk_nrf52840 board 
 * 
 ***************************************************************************************/

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/settings/settings.h>

#include "../../common/common.h"


#if !defined(CONFIG_BT_ISO_GROUPCHAT_COMMON)
#error "CONFIG_BT_ISO_GROUPCHAT_COMMON=y has not been defined in proj.conf"
#endif
#if !defined(CONFIG_BT_ISO_GROUPCHAT_SECONDARY)
#error "CONFIG_BT_ISO_GROUPCHAT_SECONDARY=y has not been defined in proj.conf"
#endif

/* --------------------------------------------------------------------------------------
To ensure a function is not optimised in anyway, use the following before it and ensure
that CONFIG_GROUPCHAT_FUNCTION_DEBUG=y is defined in prj.conf

#if defined(CONFIG_GROUPCHAT_FUNCTION_DEBUG)
__attribute__((optimize("O0")))
#endif

It is a kludge but it works as the documented way to enable no optimisation does not
work when Zephyr build is invoked.
----------------------------------------------------------------------------------------*/

/* When the BLE controller stack has been updated so that it is capable of sending or
   receiving on BIS2, then the following define should be enabled by removing the 'not'
   at the start of the line so that data is continuously sent on BIS2
*/
#define ENABLE_SEND_ON_BIS2

#define TIMEOUT_SYNC_CREATE K_SECONDS(10)
#define NAME_LEN            30
#define BIG_SDU_INTERVAL_US      (10000)
#define BUF_ALLOC_TIMEOUT_US     (BIG_SDU_INTERVAL_US * 2U) /* milliseconds */

#define BT_LE_SCAN_CUSTOM BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, \
					   BT_LE_SCAN_OPT_NONE, \
					   BT_GAP_SCAN_FAST_INTERVAL, \
					   BT_GAP_SCAN_FAST_WINDOW)

#define PA_RETRY_COUNT 6

static bool         per_adv_found;
static bool         per_adv_lost;
static bt_addr_le_t per_addr;
static uint8_t      per_sid;
static uint32_t     per_interval_us;

static uint32_t     iso_bis_lost_count[BIS_ISO_CHAN_COUNT];
static uint32_t     iso_frame_count;
static bool         big_synced=false;
static bool 	    bis_connected[BIS_ISO_CHAN_COUNT]={0};

/* Define a semaphore which when available implies that ALL the net_bufs 
   associated with that bis channel is free. It should be initialised to 
   a count of UNIQUE_NET_BUFS_PER_BIS */
static struct k_sem sem_chan_sent[CONFIG_BT_ISO_MAX_CHAN];

static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);
static K_SEM_DEFINE(sem_per_big_info, 0, 1);
static K_SEM_DEFINE(sem_big_sync, 0, BIS_ISO_CHAN_COUNT);
static K_SEM_DEFINE(sem_big_sync_lost, 0, BIS_ISO_CHAN_COUNT);

#define UNIQUE_NET_BUFS_PER_BIS  (1+1) /* +1 because PTO=1 in BIG - this will be assert checked
                                          after the BIG is created */

NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, 
                          BIS_ISO_CHAN_COUNT, /* _count = 2 */
			  			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU ) /* _data_size */,
			  			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, /* _ud_size = 16 */
						  NULL);

struct app_src_context {
	struct app_bis_payload app_bis_payload[BIS_ISO_CHAN_COUNT];
	uint16_t 	seq_num;  /* HCI Packet Sequence number */
	uint32_t 	iso_frame_count;
	uint32_t    end_start_cycles; /* how many times it has been terminated and restarted*/
};

static struct app_src_context src_ctx;

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#ifdef CONFIG_ISO_BLINK_LED0
static const struct gpio_dt_spec led_gpio = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#define BLINK_ONOFF K_MSEC(500)

static struct k_work_delayable blink_work;
static bool                    led_is_on;
static bool                    blink;

static void blink_timeout(struct k_work *work)
{
	if (!blink) {
		return;
	}

	led_is_on = !led_is_on;
	gpio_pin_set_dt(&led_gpio, (int)led_is_on);

	k_work_schedule(&blink_work, BLINK_ONOFF);
}
#endif

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	if(!big_synced){
		char le_addr[BT_ADDR_LE_STR_LEN];
		char name[NAME_LEN];

		(void)memset(name, 0, sizeof(name));

		bt_data_parse(buf, data_cb, name);

		bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
		printk("ADV [DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s "
			"C:%u S:%u D:%u SR:%u E:%u Prim: %s, Secn: %s, "
			"Interval: 0x%04x (%u us), SID: %u\n",
			le_addr, info->adv_type, info->tx_power, info->rssi, name,
			(info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
			(info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
			(info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
			(info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
			(info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
			phy2str(info->primary_phy), phy2str(info->secondary_phy),
			info->interval, BT_CONN_INTERVAL_TO_US(info->interval), info->sid);

		if (!per_adv_found && info->interval) {
			per_adv_found = true;

			per_sid = info->sid;
			per_interval_us = BT_CONN_INTERVAL_TO_US(info->interval);
			bt_addr_le_copy(&per_addr, info->addr);

			k_sem_give(&sem_per_adv);
		}
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void sync_cb(struct bt_le_per_adv_sync *sync,
		    struct bt_le_per_adv_sync_synced_info *info)
{
	if(!big_synced){
		char le_addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
                
		printk("PA--SYNC[%u]: [DEVICE]: %s synced, "
			"Interval 0x%04x (%u ms), PHY %s\n",
			bt_le_per_adv_sync_get_index(sync), le_addr,
			info->interval, info->interval * 5 / 4, phy2str(info->phy));

		k_sem_give(&sem_per_sync);
	}
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PA--SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);

	per_adv_lost = true;
	k_sem_give(&sem_per_sync_lost);
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	if(!big_synced){
		char le_addr[BT_ADDR_LE_STR_LEN];
		char data_str[129];

		bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
		bin2hex(buf->data, buf->len, data_str, sizeof(data_str));

		printk("PA--SYNC[%u]: [DEVICE]: %s, tx_power %i, "
			"RSSI %i, CTE %u, data length %u, data: %s\n",
			bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
			info->rssi, info->cte_type, buf->len, data_str);
	}
}

static void biginfo_cb(struct bt_le_per_adv_sync *sync,
		       const struct bt_iso_biginfo *biginfo)
{
	if(!big_synced){
		char le_addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(biginfo->addr, le_addr, sizeof(le_addr));

		printk("BIG INFO[%u]: [DEVICE]: %s, sid 0x%02x, "
			"num_bis %u, nse %u, interval 0x%04x (%u ms), "
			"bn %u, pto %u, irc %u, max_pdu %u, "
			"sdu_interval %u us, max_sdu %u, phy %s, "
			"%s framing, %sencrypted\n",
			bt_le_per_adv_sync_get_index(sync), le_addr, biginfo->sid,
			biginfo->num_bis, biginfo->sub_evt_count,
			biginfo->iso_interval,
			(biginfo->iso_interval * 5 / 4),
			biginfo->burst_number, biginfo->offset,
			biginfo->rep_count, biginfo->max_pdu, biginfo->sdu_interval,
			biginfo->max_sdu, phy2str(biginfo->phy),
			biginfo->framing ? "with" : "without",
			biginfo->encryption ? "" : "not ");


		k_sem_give(&sem_per_big_info);
	}
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.recv = recv_cb,
	.biginfo = biginfo_cb,
};

/* forward declare callback functions*/
static void iso_bis_connected(struct bt_iso_chan *chan);
static void iso_bis_disconnected(struct bt_iso_chan *chan, uint8_t reason);
static void iso_bis_recv(struct bt_iso_chan *chan, 
                         const struct bt_iso_recv_info *info,
		                 struct net_buf *buf);
static void iso_bis_sent(struct bt_iso_chan *chan);

static struct bt_iso_chan_ops iso_ops = {
	.recv		    = iso_bis_recv,
	.connected	    = iso_bis_connected,
	.disconnected	= iso_bis_disconnected,
	.sent           = iso_bis_sent,
};

static struct bt_iso_chan_io_qos iso_rx_qos[BIS_ISO_CHAN_COUNT];

static struct bt_iso_chan_qos bis_iso_qos[] = {
	{ .rx = &iso_rx_qos[0], },
	{ .rx = &iso_rx_qos[1], },
};

static struct bt_iso_chan bis_iso_chan[] = {
	{ .bis_idx=1, .ops = &iso_ops, .qos = &bis_iso_qos[0], },
	{ .bis_idx=2, .ops = &iso_ops, .qos = &bis_iso_qos[1], },
};

static struct bt_iso_chan *bis[] = {
	&bis_iso_chan[0],
	&bis_iso_chan[1],
};

static struct bt_iso_big_sync_param big_sync_param = {
	.bis_channels = bis,
	.num_bis = BIS_ISO_CHAN_COUNT,
	.bis_bitfield = (BIT_MASK(BIS_ISO_CHAN_COUNT)),
	.mse = BT_ISO_SYNC_MSE_ANY, /* any number of subevents */
	.sync_timeout = 100, /* in 10 ms units */
};

static void reset_semaphores(void)
{
	k_sem_reset(&sem_per_adv);
	k_sem_reset(&sem_per_sync);
	k_sem_reset(&sem_per_sync_lost);
	k_sem_reset(&sem_per_big_info);
	k_sem_reset(&sem_big_sync);
	k_sem_reset(&sem_big_sync_lost);
}

static void iso_bis_connected(struct bt_iso_chan *chan)
{
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx==0) {
		printk("Invalid BIS Index %u\n", chan->bis_idx);
		return;
	}
	printk("BIS Index %u connected\n", bis_idx);
	/* reset all the bis lost counts */
	for (uint8_t i = 0U; i < BIS_ISO_CHAN_COUNT; i++) {
		iso_bis_lost_count[i]=0;
	}
	iso_frame_count=0;
	bis_connected[bis_idx-1]=true;
	k_sem_give(&sem_big_sync);
}

static void iso_bis_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx==0) {
		printk("Invalid BIS Index %u\n", chan->bis_idx);
		return;
	}
	printk("BIS Index %u disconnected with reason 0x%02x\n",
		bis_idx, reason);

	if (reason != BT_HCI_ERR_OP_CANCELLED_BY_HOST) {
		bis_connected[bis_idx-1]=false;
		k_sem_give(&sem_big_sync_lost);
	}
}

static void iso_bis_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info, struct net_buf *buf)
{
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx == 0) {
		printk("Invalid BIS Index %u\n", chan->bis_idx);
		return;
	}
	uint8_t index = bis_idx-1;

	/* Increment frame count if this is BIS1 as that is always the start of the frame*/
	if(bis_idx==1){
		iso_frame_count++;
	}

	/* if invalid packet then increment appropriate count*/
	if((info->flags & BT_ISO_FLAGS_VALID)==0) {
		iso_bis_lost_count[index]++;
	}

	if ((iso_frame_count % CONFIG_ISO_PRINT_INTERVAL) == 0) {
		struct app_bis_payload *payload = (struct app_bis_payload *)buf->data;
		/* print [n,m]:count
		      where n is the BIS index 1..N
			  m is the source id 1=Primary, 2=Mixer, 10..99=Secondary
			  count is the number of packets sent as per the payload
		*/
		/* Print the current BIS number for this callback and a newline also if BIS1*/
		if(bis_idx==1){
			printk("\nS< %u:", bis_idx);
		} else {
			printk(" %u:", bis_idx);
		}
		/* print data if valid in this callback */
		if(info->flags & BT_ISO_FLAGS_VALID){
			printk("[%u,%u,%u,%u]", payload->src_bis_index, payload->src_type_id, payload->src_send_count, iso_bis_lost_count[index]);
		} else {
			printk("[-,-,-,%u]", iso_bis_lost_count[index]);
		}
		/* reset the lost count for this BIS channel */
		iso_bis_lost_count[index]=0;
	}
}

static void iso_bis_sent(struct bt_iso_chan *chan)
{
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx == 0) {
		printk("Invalid BIS channel %u\n", chan->bis_idx);
		return;
	}

	/* In a secondary implementation we should only get a sent for BIS1*/
	if(bis_idx==1){
		printk("Unexpected tx of BIS channel %u\n", chan->bis_idx);
		return;
	}

	k_sem_give(&sem_chan_sent[bis_idx-1]);  //HACK-MJT
}

static bool bis_semaphores_create(uint8_t max_bis_chans)
{
	int err;
	__ASSERT(max_bis_chans <= CONFIG_BT_ISO_MAX_CHAN, "bis chan count exceeds max");
	for (uint8_t chan_idx = 0U; chan_idx < max_bis_chans; chan_idx++) {
		err = k_sem_init(&sem_chan_sent[chan_idx], UNIQUE_NET_BUFS_PER_BIS, UNIQUE_NET_BUFS_PER_BIS);
		if(err) {
			printk("Failed to create semaphore for BIS channel %u\n", chan_idx);
			return false;
		}
	}
	return true;
}

#if defined(ENABLE_SEND_ON_BIS2)
static bool bis_channel_send(uint8_t chan_idx)
{
	struct net_buf *buf;
	int ret;

	/* Wait for up to 20ms for a net_buf to be free */
	ret = k_sem_take(&sem_chan_sent[chan_idx], K_USEC(BUF_ALLOC_TIMEOUT_US));
	if (ret) {
		printk("k_sem_take for ISO chan %u failed\n", chan_idx);
		return 0;
	}

	/* Allocate a buffer for the ISO data - will not have to wait */
	buf = net_buf_alloc(&bis_tx_pool, K_USEC(BUF_ALLOC_TIMEOUT_US));
	if (!buf) {
		printk("Data buffer allocate timeout on channel %u\n", chan_idx);
		return 0;
	}

	src_ctx.app_bis_payload[chan_idx].src_send_count++;
	src_ctx.app_bis_payload[chan_idx].src_type_id = 1; /* Primary */

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, &src_ctx.app_bis_payload[chan_idx], sizeof(struct app_bis_payload));  /* a memcpy will happen to put copy into the buffer */
	printk("send: %d bytes of data with PSN %u\n", BT_ISO_CHAN_SEND_RESERVE, src_ctx.seq_num);
	ret = bt_iso_chan_send(&bis_iso_chan[chan_idx], buf, src_ctx.seq_num);
	if (ret < 0) {
		printk("Unable to broadcast data on channel %u : %d\n", chan_idx, ret);
		net_buf_unref(buf);
		return 0;
	}
	return true;
}
#endif

static void init_app_context()
{
	src_ctx.seq_num = 0U;
	src_ctx.iso_frame_count = 0U;
	src_ctx.end_start_cycles = 0U;
	for (uint8_t chan_idx = 0U; chan_idx < BIS_ISO_CHAN_COUNT; chan_idx++) {
		src_ctx.app_bis_payload[chan_idx].src_send_count = 0U;
		src_ctx.app_bis_payload[chan_idx].src_bis_index = chan_idx + 1U;
	}
}

int main(void)
{
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync;
	struct bt_iso_big *big;
	uint32_t sem_timeout_us;
	int err;

	printk("Starting GroupChat Secondary\n");

#ifdef CONFIG_ISO_BLINK_LED0
	printk("Get reference to LED device...");

	if (!gpio_is_ready_dt(&led_gpio)) {
		printk("LED gpio device not ready.\n");
		return 0;
	}
	printk("done.\n");

	printk("Configure GPIO pin...");
	err = gpio_pin_configure_dt(&led_gpio, GPIO_OUTPUT_ACTIVE);
	if (err) {
		return 0;
	}
	printk("done.\n");

	init_app_context();

	/* create and initialise BIS net_buf related semaphores for each channel*/
	if(!bis_semaphores_create(CONFIG_BT_ISO_MAX_CHAN)){
		printk("Failed to create channel related semaphores\n");
		return 0;
	}

	k_work_init_delayable(&blink_work, blink_timeout);
#endif /* CONFIG_ISO_BLINK_LED0 */

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	/* Make it so that the same bdaddr is used on power up to make it
	   more convenient when using a sniffer to check behvior */
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		settings_load();
	}

	printk("Scan callbacks register...");
	printk("CALL bt_le_scan_cb_register()\n");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("EXIT bt_le_scan_cb_register()\n");

	printk("Periodic Advertising callbacks register...");
	printk("CALL bt_le_per_adv_sync_cb_register()\n");
	bt_le_per_adv_sync_cb_register(&sync_callbacks);
	printk("EXIT bt_le_per_adv_sync_cb_register()\n");

	do {
		big_synced=false;
		reset_semaphores();
		per_adv_lost = false;

		printk("Start scanning...\n");
		printk("CALL bt_le_scan_start()\n");
		err = bt_le_scan_start(BT_LE_SCAN_CUSTOM, NULL);
		if (err) {
			printk("EXIT - ERR bt_le_scan_start() failed (err %d)\n", err);
			return 0;
		}
		printk("EXIT bt_le_scan_start()\n");

#ifdef CONFIG_ISO_BLINK_LED0
		printk("Start blinking LED...\n");
		led_is_on = false;
		blink = true;
		gpio_pin_set_dt(&led_gpio, (int)led_is_on);
		k_work_reschedule(&blink_work, BLINK_ONOFF);
#endif /* CONFIG_ISO_BLINK_LED0 */

		printk("Waiting for periodic advertising...\n");
		per_adv_found = false;
		err = k_sem_take(&sem_per_adv, K_FOREVER);
		if (err) {
			printk("k_sem_take(&sem_per_adv) failed (err %d)\n", err);
			return 0;
		}
		printk("Found periodic advertising.\n");

		printk("Stop scanning...\n");
		printk("CALL bt_le_scan_stop()\n");
		err = bt_le_scan_stop();
		if (err) {
			printk("EXIT - ERR bt_le_scan_stop() cfailed (err %d)\n", err);
			return 0;
		}
		printk("EXIT bt_le_scan_stop()\n");

		printk("Creating Periodic Advertising Sync...");
		bt_addr_le_copy(&sync_create_param.addr, &per_addr);
		sync_create_param.options = 0;
		sync_create_param.sid = per_sid;
		sync_create_param.skip = 0;
		/* Multiple PA interval with retry count and convert to unit of 10 ms */
		sync_create_param.timeout = (per_interval_us * PA_RETRY_COUNT) /
						(10 * USEC_PER_MSEC);
		sem_timeout_us = per_interval_us * PA_RETRY_COUNT;
		printk("CALL bt_le_per_adv_sync_create()\n");
		err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
		if (err) {
			printk("EXIT - ERR bt_le_per_adv_sync_create() failed (err %d)\n", err);
			return 0;
		}
		printk("EXIT bt_le_per_adv_sync_create()\n");

		printk("Waiting for periodic sync...\n");
		err = k_sem_take(&sem_per_sync, K_USEC(sem_timeout_us));
		if (err) {
			printk("failed (err %d)\n", err);

			printk("Deleting Periodic Advertising Sync...");
			printk("CALL bt_le_per_adv_sync_delete()\n");
			err = bt_le_per_adv_sync_delete(sync);
			if (err) {
				printk("EXIT - ERR bt_le_per_adv_sync_delete() failed (err %d)\n", err);
				return 0;
			}
			printk("EXIT bt_le_per_adv_sync_delete()\n");
			continue;
		}
		printk("Periodic sync established.\n");

		printk("Waiting for BIG info...\n");
		err = k_sem_take(&sem_per_big_info, K_USEC(sem_timeout_us));
		if (err) {
			printk("ERR k_sem_take(sem_per_big_info) failed (err %d)\n", err);

			if (per_adv_lost) {
				continue;
			}

			printk("Deleting Periodic Advertising Sync...");
			printk("CALL bt_le_per_adv_sync_delete()\n");
			err = bt_le_per_adv_sync_delete(sync);
			if (err) {
				printk("EXIT - ERR bt_le_per_adv_sync_delete() failed (err %d)\n", err);
				return 0;
			}
			printk("EXIT bt_le_per_adv_sync_delete()\n");
			continue;
		}
		printk("Periodic sync established.\n");
		big_synced=true;

big_sync_create:
		printk("\n\nCreate BIG Sync...\n");
		printk("CALL bt_iso_big_sync()\n");
		err = bt_iso_big_sync(sync, &big_sync_param, &big);
		if (err) {
			printk("EXIT - ERR bt_iso_big_sync() failed (err %d)\n", err);
			return 0;
		}
		printk("EXIT bt_iso_big_sync()\n");

		for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
			printk("Waiting for BIG sync chan %u...\n", chan);
			err = k_sem_take(&sem_big_sync, TIMEOUT_SYNC_CREATE);
			if (err) {
				printk("ERR k_sem_take(sem_big_sync) failed (err %d)\n", err);
				break;
			}
			printk("BIG sync chan %u successful.\n", chan);
		}
		if (err) {
			printk("for loop exit - failed (err %d)\n", err);

			printk("BIG Sync Terminate...");
			printk("CALL bt_iso_big_terminate()\n");
			err = bt_iso_big_terminate(big);
			if (err) {
				printk("ERR bt_iso_big_terminate() failed (err %d)\n", err);
				return 0;
			}
			printk("DONE bt_iso_big_terminate()\n");

			goto per_sync_lost_check;
		}
		printk("BIG sync established.\n");
		big_synced=true;

#if defined(ENABLE_SEND_ON_BIS2)
		/* try to send a packet from BIS2 */
		uint8_t chan_idx = 1;
		while (bis_connected[chan_idx]) {
			if(!bis_channel_send(1)){
			printk("Failed to send on BIS2\n");
			return 0;	
			}
		}
#endif

#ifdef CONFIG_ISO_BLINK_LED0
		printk("Stop blinking LED.\n");
		blink = false;
		/* If this fails, we'll exit early in the handler because blink
		 * is false.
		 */
		k_work_cancel_delayable(&blink_work);

		/* Keep LED on */
		led_is_on = true;
		gpio_pin_set_dt(&led_gpio, (int)led_is_on);
#endif /* CONFIG_ISO_BLINK_LED0 */

		for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
			printk("Waiting for BIG sync lost chan %u...\n", chan);
			err = k_sem_take(&sem_big_sync_lost, K_FOREVER);
			if (err) {
				printk("failed (err %d)\n", err);
				return 0;
			}
			printk("BIG sync lost chan %u.\n", chan);
		}
		printk("BIG sync lost.\n");

per_sync_lost_check:
		printk("Check for periodic sync lost...\n");
		err = k_sem_take(&sem_per_sync_lost, K_NO_WAIT);
		if (err) {
			/* Periodic Sync active, go back to creating BIG Sync */
			goto big_sync_create;
		}
		printk("Periodic sync lost.\n");
	} while (true);
}
