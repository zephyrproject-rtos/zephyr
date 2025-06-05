/*
 * Copyright (c) 2021-2025 Nordic Semiconductor ASA
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
 *   on BIS2 by enableing the defines CONFIGURE_BIS2_AS_SEND and ENABLE_SEND_ON_BIS2.
 * 
 * --------------------------------------------------------------------------------------
 * 
 *   Testing: Use the Nordic nrf52840dk_nrf52840 board 
 * 
 ***************************************************************************************/

/*==============================================================================

   There are 3 branches and if all good the lineage and order from  latest to 
   oldest will be:-

      'feature_iso_recv_send'
	  'feature_iso_recv_send_base'
	  'refactor_iso_broadcast__iso_receive'
	  'main'

   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   To ensure branch 'feature_iso_recv_send' is compiled with the latest 'main' 
   branch in the zephyr repo
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

a) Sync fork of the {origin} zephyr repo with {upstream} repo
b) Checkout branch 'origin:main'
c) Pull the latest 'origin:main'
d) Checkout branch 'refactor_iso_broadcast__iso_receive'
e) Rebase 'refactor_iso_broadcast__iso_receive' on top of latest 'main'
f) Delete existing Tag "ISO_SAMPLE_APPS_BASE_YYMMDD"
g) Add new tag to 'origin:main' called "ISO_SAMPLE_APPS_BASE_YYMMDD"
h) Test the changes and if fail, fix until all good in branch 
   'refactor_iso_broadcast__iso_receive' and commit the changes

i) If any files in sample app "iso_receive" have changed then
     i1) Checkout branch 'feature_iso_recv_send_base'
     i2) Delete content of sample app "iso_recv_send"
     i3) Copy the content of "temp_folder" to sample app folder "iso_recv_send"
     i4) Commit the branch 'feature_iso_recv_send_base'
j) Checkout branch 'feature_iso_recv_send_base'
k) Rebase 'feature_iso_recv_send_base' on top of branch 
   'refactor_iso_broadcast__iso_receive'
l) Checkout branch 'feature_iso_recv_send'
m) Rebase 'feature_iso_recv_send' ontop of branch 'feature_iso_recv_send_base'
n) Test the changes, fix until all good in branch 
   'feature_iso_recv_send' and commit the changes


   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   To make changes to sample app "iso_receive" and/or "iso_broadcast"
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1) If you wish to pull the latest 'main' from upstream repo then
     1a) Follow steps (a) to (h) inclusive
2) Checkout branch 'refactor_iso_broadcast__iso_receive'
3) Update sample apps "iso_receive" and/or "iso_broadcast"
4) Test the changes and if fail, fix until all good in branch 
   'refactor_iso_broadcast__iso_receive' and commit the changes
5) If any files in sample app "iso_receive" is changed then
	 5a) Checkout branch 'feature_iso_recv_send_base'
     5b) Delete content of sample app "iso_recv_send"
     5c) Copy the content of "temp_folder" to sample app folder "iso_recv_send"
     5d) Commit the branch 'feature_iso_recv_send_base'
6) Then follow all steps from step (i) above.

==============================================================================*/

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>

/*------------------------------------------------------------------------------
Sanity check configs for this app.
------------------------------------------------------------------------------*/
#if !defined(CONFIG_BT_ISO_BIS_RECV_SEND)
/*
   This sample app is only for the case where the controller is capable of
   sending and receiving on BIS1 and BIS2. If this is not the case, then the
   app will not work as expected.	
*/
#error "CONFIG_BT_ISO_BIS_RECV_SEND not defined"
#endif
#if !defined(CONFIG_BT_ISO_SYNC_RECEIVER) || !defined(CONFIG_BT_ISO_BROADCASTER)
/*
   If CONFIG_BT_ISO_BIS_RECV_SEND is defined, then both CONFIG_BT_ISO_SYNC_RECEIVER
   and CONFIG_BT_ISO_BROADCASTER must be defined. If this is not the case, then the
   app will not work as expected.
*/
#error "CONFIG_BT_ISO_SYNC_RECEIVER or CONFIG_BT_ISO_BROADCASTER not defined"
#endif
#if CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT != CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT
/*
   The number of sync iso streams and adv iso streams must be the same
   because the recv_send app has BIS1 as input and BIS2 as output and
   this implementation is a hard coded hack in the stack and we are 'hijacking'
   the adv iso stream to be used as a sync iso stream.
   In a proper implementation the stack would be able to handle
   different number of streams for sync and adv iso streams.
   If this is not the case, then the app will not work as expected.
*/
#error "CONFIG_BT_CTLR_SYNC_ISO_STREAM_COUNT != CONFIG_BT_CTLR_ADV_ISO_STREAM_COUNT"
#endif
/*------------------------------------------------------------------------------
End of sanity check of configs for this app.
------------------------------------------------------------------------------*/


/* When the BLE controller stack has been updated so that it is capable of sending or
   receiving on BIS2, then the following define should be enabled by removing the 'not'
   at the start of the line so that data is continuously sent on BIS2
*/
#define CONFIGURE_BIS2_AS_SEND
#define ENABLE_SEND_ON_BIS2

#define TIMEOUT_SYNC_CREATE K_SECONDS(10)
#define NAME_LEN            30
#define BIS_INDEX_INVALID   0
#define BIG_SDU_INTERVAL_US      (10000)
#define BUF_ALLOC_TIMEOUT_US     (BIG_SDU_INTERVAL_US * 2U) /* milliseconds */

#define BT_LE_SCAN_CUSTOM BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, \
					   BT_LE_SCAN_OPT_NONE, \
					   BT_GAP_SCAN_FAST_INTERVAL, \
					   BT_GAP_SCAN_FAST_WINDOW)

#define PA_RETRY_COUNT 6

#define BIS_ISO_CHAN_COUNT MIN(2U, CONFIG_BT_ISO_MAX_CHAN)

static bool         per_adv_found;
static bool         per_adv_lost;
static bt_addr_le_t per_addr;
static uint8_t      per_sid;
static uint32_t     per_interval_us;
static bool         print_adverts=true;

static uint32_t     iso_frame_count; /* incremented once per iso interval */
static uint32_t     iso_bis_lost_count[BIS_ISO_CHAN_COUNT];
static bool 	    bis_connected[BIS_ISO_CHAN_COUNT]={0};

/* This struct MUST be manually synced with the payload sent out in iso_broadcast
   If the size of the payload struct is increased, then increase the value
   of the following configs by **at least** the value it has increased by
   CONFIG_BT_ISO_TX_MTU  (in file proj.conf)
   CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE (in file overlay-bt_ll_sw_split.conf)
   CONFIG_BT_CTLR_ADV_ISO_PDU_LEN_MAX (in file overlay-bt_ll_sw_split.conf)
   */
struct __attribute__((packed)) app_bis_payload {
   uint32_t     src_send_count;
   uint8_t 	    src_bis_index;  /* 1..N */
   uint8_t      src_type_id;  /* 1=Primary,2=Mixer, 10..99=Secondary */
};

/* caches for the content of each bis that is sent */
static struct app_bis_payload app_bis_payload[BIS_ISO_CHAN_COUNT];
/* HCI Packet Sequence number */
static uint16_t               seq_num[BIS_ISO_CHAN_COUNT];  

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

/* create buffers for BIS transmissions */
NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, 
                          BIS_ISO_CHAN_COUNT, /* _count = 2 */
			  			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU ) /* _data_size */,
			  			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, /* _ud_size = 16 */
						  NULL);

/* converts iso chan pointer into to a bis_index in the range 1..N and o
   deemed invalid using the pointer values to search in the bis array */
uint8_t get_bis_idx(struct bt_iso_chan *chan);

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
	if(print_adverts) {
		char le_addr[BT_ADDR_LE_STR_LEN];
		char name[NAME_LEN];

		(void)memset(name, 0, sizeof(name));

		bt_data_parse(buf, data_cb, name);

		bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
		printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s "
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
	if(print_adverts) {
		char le_addr[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

		printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
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

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);

	per_adv_lost = true;
	k_sem_give(&sem_per_sync_lost);
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	if(print_adverts) {
		char le_addr[BT_ADDR_LE_STR_LEN];
		char data_str[129];

		bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
		bin2hex(buf->data, buf->len, data_str, sizeof(data_str));

		printk("PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
			"RSSI %i, CTE %u, data length %u, data: %s\n",
			bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
			info->rssi, info->cte_type, buf->len, data_str);
	}
}

static void biginfo_cb(struct bt_le_per_adv_sync *sync,
		       const struct bt_iso_biginfo *biginfo)
{
	if( print_adverts) {
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

static void iso_bis_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		struct net_buf *buf)
{
	/*
	  The following code is used to print the received data from all BIS
	  channels. 
	  The data is expected to be in the format:
 	   S d < n:[s,m,c,l] n:[s,m,c,l] n:[s,m,c,l] n:[s,m,c,l] ...
		 where for each BIS channel ...
		  d is the delta time in milliseconds since last print
		  n is the BIS index 1..N as per the callback chan pointer
		  s is the source id 1=Primary, 10..99=Secondary
		  m is the source bis index in the payload
		  c is the packet count provided by source in the payload
		  l is the packet lost count for this bis channel 'n' since last print
	*/
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx == BIS_INDEX_INVALID) {
		printk("Invalid BIS channel %p (index %u)\n", chan, bis_idx);
		return;
	}
	uint8_t index = bis_idx-1;

	/* Enable the following code to print the timestamps of the first 
	   few callbacks. Typical output will be 
	   Log: 1:0 1:10000 1:2 1:10000 1:4294967295 1:10000 1:0 1:10000 1:0 1:10000
	   where the 4294967295 implies that most likely the timestamp did not exist
	   */	
#if 0
	#define LOG_DIFF_ARRAY_SIZE 12  //must be >=8
	static bool log_enable = true;
	static uint32_t log_idx=0;
	static uint8_t log_bis_idx[LOG_DIFF_ARRAY_SIZE];
	static uint32_t log_time_diff[LOG_DIFF_ARRAY_SIZE];
	if(log_enable){
		if(log_idx<LOG_DIFF_ARRAY_SIZE) {
			/* log the bis_idx and time diff for this callback */
			log_bis_idx[log_idx] = bis_idx;
			//log_time_diff[log_idx] = k_uptime_get();
			log_time_diff[log_idx] = info->ts;
			log_idx++;
		} 
			
		if(log_idx==LOG_DIFF_ARRAY_SIZE) {
			/* if log is full then print it and reset */
			printk("\nLog: ");
			for(uint8_t i=1; i<LOG_DIFF_ARRAY_SIZE-1; i++) {
				printk("%u:%u ", log_bis_idx[i], log_time_diff[i]-log_time_diff[i-1]);
			}
			printk("\n");
			log_enable=false;
		}
	}
#endif

	/* Increment frame count if this is BIS1 as that is always the start of the frame*/
	if(bis_idx==1){
		iso_frame_count++;
	}

	/* if invalid packet then increment appropriate count*/
	if((info->flags & BT_ISO_FLAGS_VALID)==0) {
		iso_bis_lost_count[index]++;
	}

	if ((iso_frame_count % CONFIG_ISO_PRINT_INTERVAL) == 0) {
		/* Print this BIS in format n:[s,m,c,l] */
		uint8_t *payload = buf->data;
	    
		/* calculate the delta time since the last print */
		static k_ticks_t old_ms = 0;
		k_ticks_t now_ms = k_uptime_get();
		k_ticks_t delta_ms = now_ms - old_ms;
		old_ms = now_ms;

		/* Print the current BIS number for this callback and a newline also if BIS1*/
		if(bis_idx==1){
			printk("\nS %u < %u:", (uint32_t)delta_ms, bis_idx);
		} else {
			printk(" %u:", bis_idx);
		}
		/* print data if valid in this callback */
		if(info->flags & BT_ISO_FLAGS_VALID){
			uint32_t count = *(uint32_t *)&payload[0];
			printk("[%u,%u,%u,%u]", payload[4], payload[5], count, iso_bis_lost_count[index]);
		} else {
			printk("[-,-,-,%u]", iso_bis_lost_count[index]);
		}
		/* reset the lost count for this BIS channel */
		iso_bis_lost_count[index]=0;
	}
}

static void iso_bis_connected(struct bt_iso_chan *chan)
{
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx == BIS_INDEX_INVALID) {
		printk("Invalid BIS channel %p (index %u)\n", chan, bis_idx);
		return;
	}
	printk("ISO Channel %p (bix_index=%u) connected\n", chan, bis_idx);

	uint8_t i=bis_idx-1;
    iso_bis_lost_count[i]=0;
	bis_connected[i]=true;

	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};
	int err;
	uint8_t dir=BT_HCI_DATAPATH_DIR_CTLR_TO_HOST; /* assume rx bis */
#if defined(CONFIGURE_BIS2_AS_SEND)
	if(bis_idx>1){
		/* BIS2 and onwards are always TX */
		dir = BT_HCI_DATAPATH_DIR_HOST_TO_CTLR;
	} 
#endif	
	err = bt_iso_setup_data_path(chan, dir, &hci_path);
	if (err != 0) {
		if(dir == BT_HCI_DATAPATH_DIR_CTLR_TO_HOST) {
			printk("Failed to setup ISO RX data path: %d\n", err);
		} else {
			printk("Failed to setup ISO TX data path: %d\n", err);
		}
	} else {
			printk("  Set BIS%u datapath = ", bis_idx);
		if(dir == BT_HCI_DATAPATH_DIR_CTLR_TO_HOST) {
			printk("RECV\n");
		} else {
			printk("SEND\n");
		}
	}
	
	k_sem_give(&sem_big_sync);
}

static void iso_bis_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx == BIS_INDEX_INVALID) {
		printk("Invalid BIS channel %p (index %u)\n", chan, bis_idx);
		return;
	}

	printk("ISO Channel %p (bix_index=%u) disconnected with reason 0x%02x\n",
	       chan, bis_idx, reason);

	uint8_t i=bis_idx-1;
	bis_connected[i]=false;

	if (reason != BT_HCI_ERR_OP_CANCELLED_BY_HOST) {
		k_sem_give(&sem_big_sync_lost);
	}
}

static void iso_bis_sent(struct bt_iso_chan *chan)
{
#if defined(ENABLE_SEND_ON_BIS2)
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx == BIS_INDEX_INVALID) {
		printk("Invalid BIS channel %p (index %u)\n", chan, bis_idx);
		return;
	}

	/* In a secondary implementation we should only get a sent for BIS1*/
	if(bis_idx==1){
		printk("Unexpected tx of BIS channel %p (index %u)\n", chan, bis_idx);
		return;
	}

	k_sem_give(&sem_chan_sent[bis_idx-1]);
#endif	
}

static struct bt_iso_chan_ops iso_ops = {
	.recv		    = iso_bis_recv,
	.connected	    = iso_bis_connected,
	.disconnected	= iso_bis_disconnected,
	.sent           = iso_bis_sent,
};

#if defined(CONFIGURE_BIS2_AS_SEND)
#define BIS_ISO_RX_COUNT   (1)
#define BIS_ISO_TX_COUNT   (BIS_ISO_CHAN_COUNT - BIS_ISO_RX_COUNT)
#define SDU_PAYLOAD_BYTES  CONFIG_BT_ISO_TX_MTU

static struct bt_iso_chan_io_qos iso_qos_rx[BIS_ISO_RX_COUNT] = {
	0 /* will be updated when sync'd */
};
static struct bt_iso_chan_io_qos iso_qos_tx[BIS_ISO_TX_COUNT] = {
	{ .sdu = SDU_PAYLOAD_BYTES, .phy = BT_GAP_LE_PHY_2M, .rtn = 1, },
#if BIS_ISO_TX_COUNT > 1
#error "BIS_ISO_TX_COUNT > 1 so add more initialisers"
#endif		
};

static struct bt_iso_chan_qos bis_iso_qos[] = {
	/* BIS1 is always rx see BIS_ISO_RX_COUNT*/
	{ .rx = &iso_qos_rx[0], .tx = NULL,},
#if BIS_ISO_RX_COUNT > 1
#error "BIS_ISO_RX_COUNT > 1 so add more initialisers"
#endif
	/* BIS2 .. BISN is always tx or silent*/
	{ .rx = NULL, .tx = &iso_qos_tx[0], },
#if BIS_ISO_TX_COUNT > 1
#error "BIS_ISO_TX_COUNT > 1 so add more initialisers"
#endif	
};

#else /* CONFIGURE_BIS2_AS_SEND */
static struct bt_iso_chan_io_qos iso_rx_qos[BIS_ISO_CHAN_COUNT];

static struct bt_iso_chan_qos bis_iso_qos[] = {
	{ .rx = &iso_rx_qos[0], .tx = NULL},
	{ .rx = &iso_rx_qos[1], .tx = NULL},
};

#endif /* CONFIGURE_BIS2_AS_SEND */

static struct bt_iso_chan bis_iso_chan[] = {
	{ .bis_index = 1, .ops = &iso_ops, .qos = &bis_iso_qos[0], },
	{ .bis_index = 2, .ops = &iso_ops, .qos = &bis_iso_qos[1], },
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

uint8_t get_bis_idx(struct bt_iso_chan *chan)
{
	/* valid bis index values are 1 .. CONFIG_BT_ISO_MAX_CHAN*/
#if 1  //MJT_HACK delete this when  group voive talk works
	if(chan) {
		if(chan->bis_index > 0 && chan->bis_index <= BIS_ISO_CHAN_COUNT) {
			return chan->bis_index;
		}
	}
#else
	for(int i=0; i<BIS_ISO_CHAN_COUNT;i++){
		if(bis[i] == chan){
			return i+1;
		}
	}
#endif		
	return BIS_INDEX_INVALID;
}

static void reset_semaphores(void)
{
	k_sem_reset(&sem_per_adv);
	k_sem_reset(&sem_per_sync);
	k_sem_reset(&sem_per_sync_lost);
	k_sem_reset(&sem_per_big_info);
	k_sem_reset(&sem_big_sync);
	k_sem_reset(&sem_big_sync_lost);
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
		return false;
	}

	/* Allocate a buffer for the ISO data - will not have to wait */
	buf = net_buf_alloc(&bis_tx_pool, K_USEC(BUF_ALLOC_TIMEOUT_US));
	if (!buf) {
		printk("Data buffer allocate timeout on channel %u\n", chan_idx);
		return false;
	}

	app_bis_payload[chan_idx].src_send_count++;
	app_bis_payload[chan_idx].src_type_id = 1; /* Primary */

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, &app_bis_payload[chan_idx], sizeof(struct app_bis_payload));  /* a memcpy will happen to put copy into the buffer */
	printk("bis_channel_send(%u): %d bytes of payload with PSN %u\n", chan_idx, sizeof(struct app_bis_payload), seq_num[chan_idx]);
	ret = bt_iso_chan_send(&bis_iso_chan[chan_idx], buf, seq_num[chan_idx]);
	if (ret < 0) {
		printk("Unable to broadcast data on BIS %u : %d\n", chan_idx+1, ret);
		net_buf_unref(buf);
		return false;
	}
	seq_num[chan_idx]++;
	return true;
}
#endif

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

static bool init_app_context()
{
	iso_frame_count = 0U;
	for (uint8_t chan_idx = 0U; chan_idx < BIS_ISO_CHAN_COUNT; chan_idx++) {
		app_bis_payload[chan_idx].src_send_count = 0U;
		app_bis_payload[chan_idx].src_bis_index = chan_idx + 1U;
        seq_num[chan_idx] = 0U;
	}

	/* create and initialise BIS net_buf related semaphores for each channel*/
	if(!bis_semaphores_create(CONFIG_BT_ISO_MAX_CHAN)){
		printk("Failed to create channel related semaphores\n");
		return false;
	}

	return true;
}

int main(void)
{
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync;
	struct bt_iso_big *big;
	uint32_t sem_timeout_us;
	int err;

	iso_frame_count = 0;

	printk("Starting iso_send_recv demo\n");

	if(!init_app_context()){
		printk("Failed to initialise app context\n");
		return 0;
	}

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

	k_work_init_delayable(&blink_work, blink_timeout);
#endif /* CONFIG_ISO_BLINK_LED0 */

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Scan callbacks register...");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register...");
	bt_le_per_adv_sync_cb_register(&sync_callbacks);
	printk("Success.\n");

	do {
		print_adverts=true;
		reset_semaphores();
		per_adv_lost = false;

		printk("Start scanning...");
		err = bt_le_scan_start(BT_LE_SCAN_CUSTOM, NULL);
		if (err) {
			printk("failed (err %d)\n", err);
			return 0;
		}
		printk("success.\n");

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
			printk("failed (err %d)\n", err);
			return 0;
		}
		printk("Found periodic advertising.\n");

		printk("Stop scanning...");
		err = bt_le_scan_stop();
		if (err) {
			printk("failed (err %d)\n", err);
			return 0;
		}
		printk("success.\n");

		printk("Creating Periodic Advertising Sync...");
		bt_addr_le_copy(&sync_create_param.addr, &per_addr);
		sync_create_param.options = 0;
		sync_create_param.sid = per_sid;
		sync_create_param.skip = 0;
		/* Multiple PA interval with retry count and convert to unit of 10 ms */
		sync_create_param.timeout = (per_interval_us * PA_RETRY_COUNT) /
						(10 * USEC_PER_MSEC);
		sem_timeout_us = per_interval_us * PA_RETRY_COUNT;
		err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
		if (err) {
			printk("failed (err %d)\n", err);
			return 0;
		}
		printk("success.\n");

		printk("Waiting for periodic adv (PA) sync...\n");
		err = k_sem_take(&sem_per_sync, K_USEC(sem_timeout_us));
		if (err) {
			printk("failed (err %d)\n", err);

			printk("Deleting Periodic Advertising Sync...");
			err = bt_le_per_adv_sync_delete(sync);
			if (err) {
				printk("failed (err %d)\n", err);
				return 0;
			}
			continue;
		}
		printk("Periodic Adv (PA) sync established.\n");

		printk("Waiting for BIG info...\n");
		err = k_sem_take(&sem_per_big_info, K_USEC(sem_timeout_us));
		if (err) {
			printk("failed (err %d)\n", err);

			if (per_adv_lost) {
				continue;
			}

			printk("Deleting Periodic Advertising Sync...");
			err = bt_le_per_adv_sync_delete(sync);
			if (err) {
				printk("failed (err %d)\n", err);
				return 0;
			}
			continue;
		}
		printk("Periodic sync established.\n");
		print_adverts=false;

big_sync_create:
		printk("Create BIG Sync...\n");
		err = bt_iso_big_sync(sync, &big_sync_param, &big);
		if (err) {
			printk("bt_iso_big_sync() failed (err %d)\n", err);
			return 0;
		}
		printk("success.\n");

		for (uint8_t chan = 0U; chan < BIS_ISO_CHAN_COUNT; chan++) {
			printk("Waiting for BIG sync chan %u...\n", chan);
			err = k_sem_take(&sem_big_sync, TIMEOUT_SYNC_CREATE);
			if (err) {
				break;
			}
			printk("BIG sync chan %u successful.\n", chan);
		}
		if (err) {
			printk("failed (err %d)\n", err);

			printk("BIG Sync Terminate...");
			err = bt_iso_big_terminate(big);
			if (err) {
				printk("failed (err %d)\n", err);
				return 0;
			}
			printk("done.\n");

			goto per_sync_lost_check;
		}
		printk("BIG sync established.\n");

#if defined(ENABLE_SEND_ON_BIS2)
		/* try to send a packet from BIS2 onwards, hence for loop starts at 1 */
		for (uint8_t chan_idx = 1U; chan_idx < BIS_ISO_CHAN_COUNT; chan_idx++) {
			while (bis_connected[chan_idx]) {
				if(!bis_channel_send(chan_idx)){
					printk("Send FAIL on BIS %u\n", chan_idx+1);
					return 0; //REMOVE THIS ONCE SEND ON BIS2 WORKING
				}
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
