/*
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/****************************************************************************************
 * Group Chat Feature: Primary device
 * 
 *   See ../../README.rst for more details
 * 
 * --------------------------------------------------------------------------------------
 * 
 *   This file contains the source code for the primary device.
 *   The primary device is responsible for creating the Broadcast Isochronous Group (BIG)
 *   and will only send data in BIS1.
 * 
 *   This file used the main.c from the iso_broadcast sample as a reference.
 * 
 * --------------------------------------------------------------------------------------
 * 
 *   Testing: Use the Nordic nrf52840dk_nrf52840 board
 * 
****************************************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/settings/settings.h>

#include "../../common/common.h"


#ifndef CONFIG_BT_ISO_GROUPCHAT_COMMON
#error "CONFIG_BT_ISO_GROUPCHAT_COMMON=y has not been defined in proj.conf"
#endif
#ifndef CONFIG_BT_ISO_GROUPCHAT_PRIMARY
#error "CONFIG_BT_ISO_GROUPCHAT_PRIMARY=y has not been defined in proj.conf"
#endif

/* For normal operation, given the BIG creates 2 BISes, set the
   following mask to 0x3, otherwise have at least BIS1 to be transmitetd
   as that allows secondary devices to transmit in channels other than BIS1*/
#define GROUPCHAT_BIS_ENABLE_MASK  (0x01)

#define BIG_SDU_INTERVAL_US      (10000)
#define BUF_ALLOC_TIMEOUT_US     (BIG_SDU_INTERVAL_US * 2U) /* milliseconds */
#define BIG_TERMINATE_TIMEOUT_US (0 * 60 * 60 * USEC_PER_SEC) /* microseconds */
#define MAX_BUF_ALLOC_COUNT      (CONFIG_BT_ISO_MAX_CHAN * CONFIG_BT_ISO_TX_BUFS_PER_CHAN)

#define UNIQUE_NET_BUFS_PER_BIS  (1+1) /* +1 because PTO=1 in BIG - this will be assert checked
                                          after the BIG is created */
#define BIS_ISO_CHAN_COUNT       (2)
NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, 
                          BIS_ISO_CHAN_COUNT, /* _count = 2 */
			  			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU ) /* _data_size */,
			  			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, /* _ud_size = 16 */
						  NULL);

static K_SEM_DEFINE(sem_big_cmplt, 0, BIS_ISO_CHAN_COUNT);
static K_SEM_DEFINE(sem_big_term, 0, BIS_ISO_CHAN_COUNT);

/* Define a semaphore which when available implies that ALL the net_bufs 
   associated with that bis channel is free. It should be initialised to 
   a count of UNIQUE_NET_BUFS_PER_BIS */
static struct k_sem sem_chan_sent[CONFIG_BT_ISO_MAX_CHAN];

#define INITIAL_TIMEOUT_COUNTER (BIG_TERMINATE_TIMEOUT_US / BIG_SDU_INTERVAL_US)

struct app_src_context {
	struct app_bis_payload app_bis_payload[BIS_ISO_CHAN_COUNT];
	uint16_t 	seq_num;  /* HCI Packet Sequence number */
	uint32_t 	iso_frame_count;
	uint32_t    end_start_cycles; /* how many times it has been terminated and restarted*/
};

static struct app_src_context src_ctx;

/* forward declare callback functions*/
static void iso_big_connected(struct bt_iso_chan *chan);
static void iso_big_disconnected(struct bt_iso_chan *chan, uint8_t reason);
static void iso_bis_sent(struct bt_iso_chan *chan);

static struct bt_iso_chan_ops iso_ops = {
	.connected	    = iso_big_connected,
	.disconnected	= iso_big_disconnected,
	.sent           = iso_bis_sent,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = sizeof(struct app_bis_payload), /* bytes */
	.rtn = 1,
	.phy = BT_GAP_LE_PHY_2M,
};

static struct bt_iso_chan_qos bis_iso_qos = {
	.tx = &iso_tx_qos,
};

static struct bt_iso_chan bis_iso_chan[] = {
	{ .bis_idx=1, .ops = &iso_ops, .qos = &bis_iso_qos, },
	{ .bis_idx=2, .ops = &iso_ops, .qos = &bis_iso_qos, },
};

static struct bt_iso_chan *bis[] = {
	&bis_iso_chan[0],
	&bis_iso_chan[1],
};

static struct bt_iso_big_create_param big_create_param = {
	.num_bis = BIS_ISO_CHAN_COUNT,
	.bis_channels = bis,
	.interval = BIG_SDU_INTERVAL_US, /* in microseconds */
	.latency = 10, /* in milliseconds */
	.packing = 0, /* 0 - sequential, 1 - interleaved */
	.framing = 0, /* 0 - unframed, 1 - framed */
};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

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

static void iso_big_connected(struct bt_iso_chan *chan)
{
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx == 0) {
		printk("Invalid BIS index %u\n", chan->bis_idx);
		return;
	}
	printk("BIS Index %u connected\n", bis_idx);
	src_ctx.seq_num = 0U;
	src_ctx.iso_frame_count=0U;
	k_sem_give(&sem_big_cmplt);
}

static void iso_big_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx == 0) {
		printk("Invalid BIS index %u\n", chan->bis_idx);
		return;
	}
	printk("BIS Index %u disconnected, reason=0x%02x\n",bis_idx, reason);
	k_sem_give(&sem_big_term);
}

static void iso_bis_sent(struct bt_iso_chan *chan)
{
	uint8_t bis_idx = get_bis_idx(chan);
	if(bis_idx == 0) {
		printk("Invalid BIS index %u\n", chan->bis_idx);
		return;
	}
	k_sem_give(&sem_chan_sent[bis_idx-1]);
}

static bool big_advertise(struct bt_le_ext_adv **adv,
			              const struct bt_data *ad, size_t ad_len,
					 	  const struct bt_le_adv_param *ext_adv_param,
						  const struct bt_le_per_adv_param *per_adv_param)
{
	int err;

	printk("Create BIG ext & pa adverts and start\n");

	/* Create a non-connectable advertising set */
	err = bt_le_ext_adv_create(ext_adv_param, NULL, adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return 0;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(*adv, ad, ad_len, NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return 0;
	}

	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, per_adv_param);
	if (err) {
		printk("Failed to set periodic advertising parameters (err %d)\n", err);
		return 0;
	}

	/* Enable Periodic Advertising */
	err = bt_le_per_adv_start(*adv);
	if (err) {
		printk("Failed to enable periodic advertising (err %d)\n", err);
		return 0;
	}

	/* Start extended advertising */
	err = bt_le_ext_adv_start(*adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return 0;
	}

	return true;
}

static uint8_t bis_unique_pdus_per_iso_interval(struct bt_iso_chan *chan)
{
	/* 
	The number of **unique** PDUs that will be sent in an ISO interval.
	Note these can be repeated in the same ISO interval to improve 
	robustness which means NSE (number of subevbts) will be at least
	BN times the number of times each unique PDU is repeated.

	It is depends on the following parameters of the bis channel which
	are available after the BIG is created:

	  BN  - Burst number
	  IRC - Immediate repetition count
	  NSE - Number of subevents 

	The formula is:
		Unique PDUs = BN + ((NSE - (BN * IRC)) / BN)

	The 3 values are created after the BIG is created and the values are
	available in the bis_iso_chan structure.

	Will return 0 if the values are not available.

	General Note:
	  Although the iso_interval for LE Audio is 7.5ms or 10ms, the 
	  SDU interval can be a sub-multiple of the iso_interval and still
	  get away with unframed BIGs. In that case for example the sdu
	  interval will be 10ms and the iso interval could be 30ms. This
	  means 3 SDU frames will need to be supplied for each iso interval.
	  Hence the burst number will be 3.
	  Conversely, the SDU packet could be much bigger than the max PDU
	  size allowed by the controller and in that case the controller
	  will have to fragment the SDU into multiple PDUs. Which means
	  that the BN will be at least the number of fragments required to
	  trasmit the SDU in the iso interval.
	  All this complexity can arise when ISO is used for non-audio and
	  custom codecs data streams.

	  Baseline is that BN is interpreted as **unique** packets that are 
	  sent in a single iso interval which may be transmitted multiple
	  times to improve robustness.
	*/

	struct bt_iso_info info;
	int err = bt_iso_chan_get_info(chan, &info);
	if (err) {
		printk("Failed to get iso chan info (err %d)\n", err);
		return 0;
	}

	int nse = info.max_subevent;
	int bn  = info.broadcaster.bn;
	int irc = info.broadcaster.irc;

	if((bn * irc)>nse){
		printk("Invalid BN(%u) or IRC(%u) or NSE(%u) values\n", bn, irc, nse);	
		return 0;
	}

	int unique_pdus = (bn + ((nse - (bn * irc)) / bn));
	if(unique_pdus<0) {
		return 0;
	}
	return unique_pdus;
}

static bool big_create(struct bt_le_ext_adv *adv, struct bt_iso_big **out_big)
{
	int err;

	printk("Create BIG...\n");
	err = bt_iso_big_create(adv, &big_create_param, out_big);
	if (err) {
		printk("Failed to create BIG (err %d)\n", err);
		return false;
	}

	printk("Waiting for BIG complete with %u channels ...\n", CONFIG_BT_ISO_MAX_CHAN);
	for (uint8_t chan = 0U; chan < CONFIG_BT_ISO_MAX_CHAN; chan++) {
		err = k_sem_take(&sem_big_cmplt, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);
			return false;
		}
	}
	printk("BIG create completed\n");
	return true;
}

#if INITIAL_TIMEOUT_COUNTER>0
static bool big_terminate(struct bt_iso_big *big)
{
	printk("BIG Terminate...\n");

    /* To ensure all net_bufs are freed and  semaphores given, sleep for 
	   100 milliseconds to ensure all buffers are sent and freed. */
    k_msleep(100);

	int err = bt_iso_big_terminate(big);
	if (err) {
		printk("failed (err %d)\n", err);
		return false;
	}
	printk("done.\n");

	for (uint8_t chan = 0U; chan < CONFIG_BT_ISO_MAX_CHAN;
			chan++) {
		printk("Waiting for BIG terminate complete chan %u...\n", chan);
		err = k_sem_take(&sem_big_term, K_FOREVER);
		if (err) {
			printk("failed (err %d)\n", err);
			return false;
		}
		printk("BIG terminate complete chan %u.\n",chan);
	}

	return true;
}
#endif

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
	ret = bt_iso_chan_send(&bis_iso_chan[chan_idx], buf, src_ctx.seq_num);
	if (ret < 0) {
		printk("Unable to broadcast data on channel %u : %d", chan_idx, ret);
		net_buf_unref(buf);
		return 0;
	}
	return true;
}

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
	/* Some controllers work best while Extended Advertising interval to be a multiple
	 * of the ISO Interval minus 10 ms (max. advertising random delay). This is
	 * required to place the AUX_ADV_IND PDUs in a non-overlapping interval with the
	 * Broadcast ISO radio events.
	 * For 10ms SDU interval a extended advertising interval of 60 - 10 = 50 is suitable
	 */
	const uint16_t adv_interval_ms = 60U;
	const uint16_t ext_adv_interval_ms = adv_interval_ms - 10U;
	const struct bt_le_adv_param *ext_adv_param = BT_LE_ADV_PARAM(
		BT_LE_ADV_OPT_EXT_ADV, BT_GAP_MS_TO_ADV_INTERVAL(ext_adv_interval_ms),
		BT_GAP_MS_TO_ADV_INTERVAL(ext_adv_interval_ms), NULL);
	const struct bt_le_per_adv_param *per_adv_param = BT_LE_PER_ADV_PARAM(
		BT_GAP_MS_TO_PER_ADV_INTERVAL(adv_interval_ms),
		BT_GAP_MS_TO_PER_ADV_INTERVAL(adv_interval_ms), BT_LE_PER_ADV_OPT_NONE);
#if INITIAL_TIMEOUT_COUNTER>0
	uint32_t timeout_counter = INITIAL_TIMEOUT_COUNTER;
#endif

	printk("Starting GroupChat Primary\n");

	/* Check that the configuration is correct to cater for enough buffers being available*/
	__ASSERT_NO_MSG(CONFIG_BT_ISO_MAX_CHAN >= BIS_ISO_CHAN_COUNT);
	__ASSERT_NO_MSG(CONFIG_BT_ISO_MAX_CHAN <= sizeof(bis_iso_chan)/sizeof(bis_iso_chan[0]));
	__ASSERT_NO_MSG(CONFIG_BT_ISO_MAX_CHAN <= sizeof(bis)/sizeof(bis[0]));
	__ASSERT_NO_MSG(CONFIG_BT_ISO_TX_BUF_COUNT >= (UNIQUE_NET_BUFS_PER_BIS * BIS_ISO_CHAN_COUNT));
	__ASSERT_NO_MSG(CONFIG_BT_ISO_TX_MTU >= sizeof(struct app_bis_payload));
	__ASSERT_NO_MSG(CONFIG_BT_CTLR_ISO_TX_BUFFER_SIZE >= (sizeof(struct app_bis_payload)+8));
	__ASSERT_NO_MSG(CONFIG_BT_CTLR_ADV_ISO_PDU_LEN_MAX >= sizeof(struct app_bis_payload));

	init_app_context();

	/* create and initialise BIS net_buf related semaphores for each channel*/
	if(!bis_semaphores_create(CONFIG_BT_ISO_MAX_CHAN)){
		printk("Failed to create channel related semaphores\n");
		return 0;
	}

#if INITIAL_TIMEOUT_COUNTER==0
	printk("BIG Terminate on timeout - Disabled\n");
#endif

	/* Initialize the Bluetooth Subsystem */
	int err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	/* Make it so that the same bdaddr is used on power up to make it
	   more convenient when using a sniffer to check behvior */
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		settings_load();
	}

	/* Create BIG adverts and start advertising*/
	struct bt_le_ext_adv *adv;
	if( !big_advertise(&adv, ad, ARRAY_SIZE(ad),ext_adv_param,per_adv_param) ){
		return 0;
	}

	/* Create BIG */
	struct bt_iso_big *big;
	if( !big_create(adv, &big) ){
		return 0;
	}

	/* Get the number of unique PDUs that will be sent in an ISO interval 
	   and assert that the bis sent semaphores are set correctly.
	   The semaphore limit and initial value must be exactly this unique
	   calculated value otherwise the data pump will not work */
	int unique_pdus = bis_unique_pdus_per_iso_interval(&bis_iso_chan[0]);
	if(UNIQUE_NET_BUFS_PER_BIS != unique_pdus){
		printk("Unique PDUs per ISO interval is %d, but the semaphore is"
		" set to %d\n", unique_pdus, UNIQUE_NET_BUFS_PER_BIS);
		return 0;
	}

	while (true) {

		/* Send all appropriate BISes in the BIG */
		uint32_t bis_tx_enabled=1;
		bool lf_sent=false;
		for (uint8_t chan_idx = 0U; chan_idx < BIS_ISO_CHAN_COUNT; chan_idx++ ) {
			if( bis_tx_enabled & GROUPCHAT_BIS_ENABLE_MASK){
				/* only send in this bis channel if enabled */
				if(!bis_channel_send(chan_idx)){
					return 0;
				}
				/* print values */
				if ((src_ctx.iso_frame_count % CONFIG_ISO_PRINT_INTERVAL) == 0) {
					if(!lf_sent){
						printk("\nP(%u)> %u:[%u,%u]",BIS_ISO_CHAN_COUNT,
						                                (chan_idx+1),
														src_ctx.app_bis_payload[chan_idx].src_type_id, 
														src_ctx.app_bis_payload[chan_idx].src_send_count);
						lf_sent = true;
					} else {
						printk(", %u:[%u,%u]",(chan_idx+1),
											src_ctx.app_bis_payload[chan_idx].src_type_id, 
						                   	src_ctx.app_bis_payload[chan_idx].src_send_count);
					}
				}
			}
			bis_tx_enabled<<=1;
		}

		src_ctx.iso_frame_count++;
		src_ctx.seq_num++;

#if INITIAL_TIMEOUT_COUNTER>0
		timeout_counter--;
		if (!timeout_counter) {
			/* Terminate the big and restart -- happens every minute*/
			timeout_counter = INITIAL_TIMEOUT_COUNTER;
			if(!big_terminate(big)){
				return 0;
			}

			/* recreate the BIG */
			if(!big_create(adv, &big)){
				return 0;
			}
		}
#endif
	}
}
