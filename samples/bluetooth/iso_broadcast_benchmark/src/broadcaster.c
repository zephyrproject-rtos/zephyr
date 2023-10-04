/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdlib.h>
#include <zephyr/console/console.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(iso_broadcast_broadcaster, LOG_LEVEL_DBG);

#define DEFAULT_BIS_RTN         2
#define DEFAULT_BIS_INTERVAL_US 7500
#define DEFAULT_BIS_LATENCY_MS  10
#define DEFAULT_BIS_PHY         BT_GAP_LE_PHY_2M
#define DEFAULT_BIS_SDU         CONFIG_BT_ISO_TX_MTU
#define DEFAULT_BIS_PACKING     0
#define DEFAULT_BIS_FRAMING     0
#define DEFAULT_BIS_COUNT       CONFIG_BT_ISO_MAX_CHAN

NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static K_SEM_DEFINE(sem_big_complete, 0, 1);
static K_SEM_DEFINE(sem_big_term, 0, 1);
static struct k_work_delayable iso_send_work;
static uint32_t iso_send_count;
static uint8_t iso_data[CONFIG_BT_ISO_TX_MTU];
static uint8_t connected_bis;

static struct bt_iso_chan bis_iso_chans[CONFIG_BT_ISO_MAX_CHAN];
static struct bt_iso_chan *bis[CONFIG_BT_ISO_MAX_CHAN];
/* We use a single seq_num for all the BIS as they share the same SDU interval */
static uint16_t seq_num;
static struct bt_iso_big_create_param big_create_param = {
	.num_bis = DEFAULT_BIS_COUNT,
	.bis_channels = bis,
	.packing = DEFAULT_BIS_PACKING, /* 0 - sequential, 1 - interleaved */
	.framing = DEFAULT_BIS_FRAMING, /* 0 - unframed, 1 - framed */
	.interval = DEFAULT_BIS_INTERVAL_US, /* in microseconds */
	.latency = DEFAULT_BIS_LATENCY_MS, /* milliseconds */
};

static void iso_connected(struct bt_iso_chan *chan)
{
	LOG_INF("ISO Channel %p connected", chan);

	connected_bis++;
	if (connected_bis == big_create_param.num_bis) {
		seq_num = 0U;
		k_sem_give(&sem_big_complete);
	}
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	LOG_INF("ISO Channel %p disconnected with reason 0x%02x",
	       chan, reason);

	connected_bis--;
	if (connected_bis == big_create_param.num_bis) {
		k_sem_give(&sem_big_term);
	}
}

static struct bt_iso_chan_ops iso_ops = {
	.connected	= iso_connected,
	.disconnected	= iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = DEFAULT_BIS_SDU, /* bytes */
	.rtn = DEFAULT_BIS_RTN,
	.phy = DEFAULT_BIS_PHY,
};

static struct bt_iso_chan_qos bis_iso_qos = {
	.tx = &iso_tx_qos,
};

static size_t get_chars(char *buffer, size_t max_size)
{
	size_t pos = 0;

	while (pos < max_size) {
		char c = tolower(console_getchar());

		if (c == '\n' || c == '\r') {
			break;
		}
		printk("%c", c);
		buffer[pos++] = c;
	}
	printk("\n");
	buffer[pos] = '\0';

	return pos;
}

static int parse_rtn_arg(void)
{
	char buffer[4];
	size_t char_count;
	uint64_t rtn;

	printk("Set RTN (current %u, default %u)\n",
	       iso_tx_qos.rtn, DEFAULT_BIS_RTN);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_RTN;
	}

	rtn = strtoul(buffer, NULL, 0);
	if (rtn > BT_ISO_BROADCAST_RTN_MAX) {
		printk("Invalid RTN %llu", rtn);
		return -EINVAL;
	}

	return (int)rtn;
}

static int parse_interval_arg(void)
{
	char buffer[9];
	size_t char_count;
	uint64_t interval;

	printk("Set interval (us) (current %u, default %u)\n",
	       big_create_param.interval, DEFAULT_BIS_INTERVAL_US);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_INTERVAL_US;
	}

	interval = strtoul(buffer, NULL, 0);
	if (interval < BT_ISO_SDU_INTERVAL_MIN || interval > BT_ISO_SDU_INTERVAL_MAX) {
		printk("Invalid interval %llu", interval);
		return -EINVAL;
	}

	return (int)interval;
}

static int parse_latency_arg(void)
{
	char buffer[6];
	size_t char_count;
	uint64_t latency;

	printk("Set latency (ms) (current %u, default %u)\n",
	       big_create_param.latency, DEFAULT_BIS_LATENCY_MS);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_LATENCY_MS;
	}

	latency = strtoul(buffer, NULL, 0);
	if (latency < BT_ISO_LATENCY_MIN || latency > BT_ISO_LATENCY_MAX) {
		printk("Invalid latency %llu", latency);
		return -EINVAL;
	}

	return (int)latency;
}

static int parse_phy_arg(void)
{
	char buffer[3];
	size_t char_count;
	uint64_t phy;

	printk("Set PHY (current %u, default %u) - %u = 1M, %u = 2M, %u = Coded\n",
	       iso_tx_qos.phy, DEFAULT_BIS_PHY, BT_GAP_LE_PHY_1M,
	       BT_GAP_LE_PHY_2M, BT_GAP_LE_PHY_CODED);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_PHY;
	}

	phy = strtoul(buffer, NULL, 0);
	if (phy != BT_GAP_LE_PHY_1M &&
	    phy != BT_GAP_LE_PHY_2M &&
	    phy != BT_GAP_LE_PHY_CODED) {
		printk("Invalid PHY %llu", phy);
		return -EINVAL;
	}

	return (int)phy;
}

static int parse_sdu_arg(void)
{
	char buffer[6];
	size_t char_count;
	uint64_t sdu;

	printk("Set SDU (current %u, default %u)\n",
	       iso_tx_qos.sdu, DEFAULT_BIS_SDU);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_SDU;
	}

	sdu = strtoul(buffer, NULL, 0);
	if (sdu > MIN(BT_ISO_MAX_SDU, sizeof(iso_data))) {
		printk("Invalid SDU %llu", sdu);
		return -EINVAL;
	}

	return (int)sdu;
}

static int parse_packing_arg(void)
{
	char buffer[3];
	size_t char_count;
	uint64_t packing;

	printk("Set packing (current %u, default %u)\n",
	       big_create_param.packing, DEFAULT_BIS_PACKING);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_PACKING;
	}

	packing = strtoul(buffer, NULL, 0);
	if (packing != BT_ISO_PACKING_SEQUENTIAL &&
	    packing != BT_ISO_PACKING_INTERLEAVED) {
		printk("Invalid packing %llu", packing);
		return -EINVAL;
	}

	return (int)packing;
}

static int parse_framing_arg(void)
{
	char buffer[3];
	size_t char_count;
	uint64_t framing;

	printk("Set framing (current %u, default %u)\n",
	       big_create_param.framing, DEFAULT_BIS_FRAMING);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_FRAMING;
	}

	framing = strtoul(buffer, NULL, 0);
	if (framing != BT_ISO_FRAMING_UNFRAMED &&
	    framing != BT_ISO_FRAMING_FRAMED) {
		printk("Invalid framing %llu", framing);
		return -EINVAL;
	}

	return (int)framing;
}

static int parse_bis_count_arg(void)
{
	char buffer[4];
	size_t char_count;
	uint64_t bis_count;

	printk("Set BIS count (current %u, default %u)\n",
	       big_create_param.num_bis, DEFAULT_BIS_COUNT);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_COUNT;
	}

	bis_count = strtoul(buffer, NULL, 0);
	if (bis_count > MAX(BT_ISO_MAX_GROUP_ISO_COUNT, CONFIG_BT_ISO_MAX_CHAN)) {
		printk("Invalid BIS count %llu", bis_count);
		return -EINVAL;
	}

	return (int)bis_count;
}

static int parse_args(void)
{
	int rtn;
	int interval;
	int latency;
	int phy;
	int sdu;
	int packing;
	int framing;
	int bis_count;

	printk("Follow the prompts. Press enter to use default values.\n");

	rtn = parse_rtn_arg();
	if (rtn < 0) {
		return -EINVAL;
	}

	interval = parse_interval_arg();
	if (interval < 0) {
		return -EINVAL;
	}

	latency = parse_latency_arg();
	if (latency < 0) {
		return -EINVAL;
	}

	phy = parse_phy_arg();
	if (phy < 0) {
		return -EINVAL;
	}

	sdu = parse_sdu_arg();
	if (sdu < 0) {
		return -EINVAL;
	}

	packing = parse_packing_arg();
	if (packing < 0) {
		return -EINVAL;
	}

	framing = parse_framing_arg();
	if (framing < 0) {
		return -EINVAL;
	}

	bis_count = parse_bis_count_arg();
	if (bis_count < 0) {
		return -EINVAL;
	}

	iso_tx_qos.rtn = rtn;
	iso_tx_qos.phy = phy;
	iso_tx_qos.sdu = sdu;
	big_create_param.interval = interval;
	big_create_param.latency = latency;
	big_create_param.packing = packing;
	big_create_param.framing = framing;
	big_create_param.num_bis = bis_count;

	return 0;
}

static void iso_timer_timeout(struct k_work *work)
{
	int ret;
	struct net_buf *buf;

	/* Reschedule as early as possible to reduce time skewing
	 * Use the ISO interval minus a few microseconds to keep the buffer
	 * full. This might occasionally skip a transmit, i.e. where the host
	 * calls `bt_iso_chan_send` but the controller only sending a single
	 * ISO packet.
	 */
	k_work_reschedule(&iso_send_work, K_USEC(big_create_param.interval - 100));

	for (int i = 0; i < big_create_param.num_bis; i++) {
		buf = net_buf_alloc(&bis_tx_pool, K_FOREVER);
		if (buf == NULL) {
			LOG_ERR("Could not allocate buffer");
			return;
		}

		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
		net_buf_add_mem(buf, iso_data, iso_tx_qos.sdu);
		ret = bt_iso_chan_send(&bis_iso_chans[i], buf, seq_num,
				       BT_ISO_TIMESTAMP_NONE);
		if (ret < 0) {
			LOG_ERR("Unable to broadcast data: %d", ret);
			net_buf_unref(buf);
			return;
		}
		iso_send_count++;

		if ((iso_send_count % 100) == 0) {
			LOG_INF("Sent %u packets", iso_send_count);
		}
	}

	seq_num++;
}

static int create_big(struct bt_le_ext_adv **adv, struct bt_iso_big **big)
{
	int err;

	/* Create a non-connectable non-scannable advertising set */
	LOG_INF("Creating Extended Advertising set");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, adv);
	if (err != 0) {
		LOG_ERR("Failed to create advertising set (err %d)", err);
		return err;
	}

	LOG_INF("Setting Periodic Advertising parameters");
	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_DEFAULT);
	if (err != 0) {
		LOG_ERR("Failed to set periodic advertising parameters (err %d)",
			err);
		return err;
	}

	/* Enable Periodic Advertising */
	LOG_INF("Starting Periodic Advertising");
	err = bt_le_per_adv_start(*adv);
	if (err != 0) {
		LOG_ERR("Failed to enable periodic advertising (err %d)", err);
		return err;
	}

	/* Start extended advertising */
	LOG_INF("Starting Extended Advertising set");
	err = bt_le_ext_adv_start(*adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		LOG_ERR("Failed to start extended advertising (err %d)", err);
		return err;
	}

	/* Prepare BIG */
	for (int i = 0; i < ARRAY_SIZE(bis_iso_chans); i++) {
		bis_iso_chans[i].ops = &iso_ops;
		bis_iso_chans[i].qos = &bis_iso_qos;
		bis[i] = &bis_iso_chans[i];
	}

	/* Create BIG */
	LOG_INF("Creating BIG");
	err = bt_iso_big_create(*adv, &big_create_param, big);
	if (err != 0) {
		LOG_ERR("Failed to create BIG (err %d)", err);
		return err;
	}

	LOG_INF("Waiting for BIG complete");
	err = k_sem_take(&sem_big_complete, K_FOREVER);
	if (err != 0) {
		LOG_ERR("failed to take sem_big_complete (err %d)", err);
		return err;
	}
	LOG_INF("BIG created");

	return 0;
}

static int delete_big(struct bt_le_ext_adv **adv, struct bt_iso_big **big)
{
	int err;

	err = bt_iso_big_terminate(*big);
	if (err != 0) {
		LOG_ERR("Failed to terminate BIG (err %d)", err);
		return err;
	}
	*big = NULL;

	err = bt_le_per_adv_stop(*adv);
	if (err != 0) {
		LOG_ERR("Failed to stop periodic advertising (err %d)", err);
		return err;
	}

	err = bt_le_ext_adv_stop(*adv);
	if (err != 0) {
		LOG_ERR("Failed to stop advertising (err %d)", err);
		return err;
	}

	err = bt_le_ext_adv_delete(*adv);
	if (err != 0) {
		LOG_ERR("Failed to delete advertiser (err %d)", err);
		return err;
	}

	*adv = NULL;

	return 0;
}

static void reset_sems(void)
{
	(void)k_sem_reset(&sem_big_complete);
	(void)k_sem_reset(&sem_big_term);
}

int test_run_broadcaster(void)
{
	struct bt_le_ext_adv *adv;
	struct bt_iso_big *big;
	int err;
	char c;
	static bool data_initialized;

	reset_sems();

	printk("Change settings (y/N)? (Current settings: rtn=%u, interval=%u, "
	       "latency=%u, phy=%u, sdu=%u, packing=%u, framing=%u, "
	       "bis_count=%u)\n", iso_tx_qos.rtn, big_create_param.interval,
	       big_create_param.latency, iso_tx_qos.phy, iso_tx_qos.sdu,
	       big_create_param.packing, big_create_param.framing,
	       big_create_param.num_bis);

	c = tolower(console_getchar());
	if (c == 'y') {
		err = parse_args();
		if (err != 0) {
			LOG_ERR("Could not parse args: %d", err);
			return err;
		}

		printk("New settings: rtn=%u, interval=%u, latency=%u, "
		       "phy=%u, sdu=%u, packing=%u, framing=%u, bis_count=%u\n",
		       iso_tx_qos.rtn, big_create_param.interval,
		       big_create_param.latency, iso_tx_qos.phy, iso_tx_qos.sdu,
		       big_create_param.packing, big_create_param.framing,
		       big_create_param.num_bis);
	}

	err = create_big(&adv, &big);
	if (err) {
		LOG_ERR("Could not create BIG: %d", err);
		return err;
	}

	iso_send_count = 0;

	if (!data_initialized) {
		/* Init data */
		for (int i = 0; i < iso_tx_qos.sdu; i++) {
			iso_data[i] = (uint8_t)i;
		}

		data_initialized = true;
	}

	k_work_init_delayable(&iso_send_work, iso_timer_timeout);
	k_work_schedule(&iso_send_work, K_MSEC(0));

	while (true) {
		printk("Press 'q' to end the broadcast\n");
		c = tolower(console_getchar());
		if (c == 'q') {
			break;
		}
	}

	LOG_INF("Ending broadcast");
	(void)k_work_cancel_delayable(&iso_send_work);

	err = delete_big(&adv, &big);
	if (err) {
		LOG_ERR("Could not delete BIG: %d", err);
		return err;
	}

	return 0;
}
