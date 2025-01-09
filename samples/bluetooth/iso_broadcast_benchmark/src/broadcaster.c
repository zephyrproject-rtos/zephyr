/*
 * Copyright (c) 2021-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>

#include <zephyr/bluetooth/gap.h>
#include <zephyr/console/console.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
LOG_MODULE_REGISTER(iso_broadcast_broadcaster, LOG_LEVEL_DBG);

#define DEFAULT_BIS_RTN           2
#define DEFAULT_BIS_INTERVAL_US   7500
#define DEFAULT_BIS_LATENCY_MS    10
#define DEFAULT_BIS_PHY           BT_GAP_LE_PHY_2M
#define DEFAULT_BIS_SDU           CONFIG_BT_ISO_TX_MTU
#define DEFAULT_BIS_PACKING       0
#define DEFAULT_BIS_FRAMING       0
#define DEFAULT_BIS_COUNT         CONFIG_BT_ISO_MAX_CHAN
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
#define DEFAULT_BIS_NSE           BT_ISO_NSE_MIN
#define DEFAULT_BIS_BN            BT_ISO_BN_MIN
#define DEFAULT_BIS_PDU_SIZE      CONFIG_BT_ISO_TX_MTU
#define DEFAULT_BIS_IRC           BT_ISO_IRC_MIN
#define DEFAULT_BIS_PTO           BT_ISO_PTO_MIN
#define DEFAULT_BIS_ISO_INTERVAL  DEFAULT_BIS_INTERVAL_US / 1250U /* N * 10 ms */
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

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
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	.irc = DEFAULT_BIS_IRC,
	.pto = DEFAULT_BIS_PTO,
	.iso_interval = DEFAULT_BIS_ISO_INTERVAL,
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
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
	if (connected_bis == 0) {
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
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	.max_pdu = DEFAULT_BIS_PDU_SIZE,
	.burst_number = DEFAULT_BIS_BN,
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

static struct bt_iso_chan_qos bis_iso_qos = {
	.tx = &iso_tx_qos,
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	.num_subevents = DEFAULT_BIS_NSE,
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
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

#if defined(CONFIG_BT_ISO_TEST_PARAMS)
static int parse_irc_arg(void)
{
	size_t char_count;
	char buffer[4];
	uint64_t irc;

	printk("Set IRC (current %u, default %u)\n",
	       big_create_param.irc, DEFAULT_BIS_IRC);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_IRC;
	}

	irc = strtoul(buffer, NULL, 0);
	if (!IN_RANGE(irc, BT_ISO_IRC_MIN, BT_ISO_IRC_MAX)) {
		printk("Invalid IRC %llu", irc);

		return -EINVAL;
	}

	return (int)irc;
}

static int parse_pto_arg(void)
{
	size_t char_count;
	char buffer[4];
	uint64_t pto;

	printk("Set PTO (current %u, default %u)\n",
	       big_create_param.pto, DEFAULT_BIS_PTO);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_PTO;
	}

	pto = strtoul(buffer, NULL, 0);
	if (!IN_RANGE(pto, BT_ISO_PTO_MIN, BT_ISO_PTO_MAX)) {
		printk("Invalid PTO %llu", pto);

		return -EINVAL;
	}

	return (int)pto;
}

static int parse_iso_interval_arg(void)
{
	uint64_t iso_interval;
	size_t char_count;
	char buffer[8];

	printk("Set ISO interval (current %u, default %u)\n",
	       big_create_param.iso_interval, DEFAULT_BIS_ISO_INTERVAL);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_ISO_INTERVAL;
	}

	iso_interval = strtoul(buffer, NULL, 0);
	if (!IN_RANGE(iso_interval, BT_ISO_ISO_INTERVAL_MIN, BT_ISO_ISO_INTERVAL_MAX)) {
		printk("Invalid ISO interval %llu", iso_interval);

		return -EINVAL;
	}

	return (int)iso_interval;
}

static int parse_nse_arg(void)
{
	uint64_t num_subevents;
	size_t char_count;
	char buffer[4];

	printk("Set number of subevents (current %u, default %u)\n",
	       bis_iso_qos.num_subevents, DEFAULT_BIS_NSE);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_NSE;
	}

	num_subevents = strtoul(buffer, NULL, 0);
	if (!IN_RANGE(num_subevents, BT_ISO_NSE_MIN, BT_ISO_NSE_MAX)) {
		printk("Invalid number of subevents %llu", num_subevents);

		return -EINVAL;
	}

	return (int)num_subevents;
}

static int parse_max_pdu_arg(void)
{
	size_t char_count;
	uint64_t max_pdu;
	char buffer[6];

	printk("Set max PDU (current %u, default %u)\n",
	       iso_tx_qos.max_pdu, DEFAULT_BIS_PDU_SIZE);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_PDU_SIZE;
	}

	max_pdu = strtoul(buffer, NULL, 0);
	if (max_pdu > BT_ISO_PDU_MAX) {
		printk("Invalid max PDU %llu", max_pdu);

		return -EINVAL;
	}

	return (int)max_pdu;
}

static int parse_bn_arg(void)
{
	uint64_t burst_number;
	size_t char_count;
	char buffer[4];

	printk("Set burst number (current %u, default %u)\n",
	       iso_tx_qos.burst_number, DEFAULT_BIS_BN);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_BIS_PDU_SIZE;
	}

	burst_number = strtoul(buffer, NULL, 0);
	if (!IN_RANGE(burst_number, BT_ISO_BN_MIN, BT_ISO_BN_MAX)) {
		printk("Invalid burst number %llu", burst_number);

		return -EINVAL;
	}

	return (int)burst_number;
}
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

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
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	int num_subevents;
	int iso_interval;
	int burst_number;
	int max_pdu;
	int irc;
	int pto;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

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

#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	irc = parse_irc_arg();
	if (irc < 0) {
		return -EINVAL;
	}

	pto = parse_pto_arg();
	if (pto < 0) {
		return -EINVAL;
	}

	iso_interval = parse_iso_interval_arg();
	if (iso_interval < 0) {
		return -EINVAL;
	}

	num_subevents = parse_nse_arg();
	if (num_subevents < 0) {
		return -EINVAL;
	}

	max_pdu = parse_max_pdu_arg();
	if (max_pdu < 0) {
		return -EINVAL;
	}

	burst_number = parse_bn_arg();
	if (burst_number < 0) {
		return -EINVAL;
	}
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	iso_tx_qos.rtn = rtn;
	iso_tx_qos.phy = phy;
	iso_tx_qos.sdu = sdu;
	big_create_param.interval = interval;
	big_create_param.latency = latency;
	big_create_param.packing = packing;
	big_create_param.framing = framing;
	big_create_param.num_bis = bis_count;
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	bis_iso_qos.num_subevents = num_subevents;
	iso_tx_qos.max_pdu = max_pdu;
	iso_tx_qos.burst_number = burst_number;
	big_create_param.irc = irc;
	big_create_param.pto = pto;
	big_create_param.iso_interval = iso_interval;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

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
		buf = net_buf_alloc(&bis_tx_pool, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Could not allocate buffer");
			return;
		}

		net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
		net_buf_add_mem(buf, iso_data, iso_tx_qos.sdu);
		ret = bt_iso_chan_send(&bis_iso_chans[i], buf, seq_num);
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

static uint16_t calc_adv_interval(uint32_t sdu_interval_us)
{
	/* Default to 6 times the SDU interval and then reduce until we reach a reasonable maximum
	 * advertising interval (BT_GAP_PER_ADV_SLOW_INT_MAX)
	 * sdu_interval_us can never be larger than 1048ms (BT_ISO_SDU_INTERVAL_MAX)
	 * so a multiple of it will always be possible to keep under BT_GAP_PER_ADV_SLOW_INT_MAX
	 * (1200ms)
	 */

	/* Convert from microseconds to advertising interval units (0.625ms)*/
	const uint32_t interval = BT_GAP_US_TO_ADV_INTERVAL(sdu_interval_us);
	uint32_t adv_interval = interval * 6U;

	/* Ensure that the adv interval is between BT_GAP_PER_ADV_FAST_INT_MIN_1 and
	 * BT_GAP_PER_ADV_SLOW_INT_MAX for the sake of interopability
	 */
	while (adv_interval < BT_GAP_PER_ADV_FAST_INT_MIN_1) {
		adv_interval += interval;
	}

	while (adv_interval > BT_GAP_PER_ADV_SLOW_INT_MAX) {
		adv_interval -= interval;
	}

	/* If we cannot convert back then it's not a lossless conversion */
	if (big_create_param.framing == BT_ISO_FRAMING_UNFRAMED &&
	    BT_GAP_ADV_INTERVAL_TO_US(interval) != sdu_interval_us) {
		LOG_INF("Advertising interval of 0x04%x is not a multiple of the advertising "
			"interval unit (0.625 ms) and may be subpar. Suggest to adjust SDU "
			"interval %u to be a multiple of 0.625 ms",
			adv_interval, sdu_interval_us);
	}

	return adv_interval;
}

static int create_big(struct bt_le_ext_adv **adv, struct bt_iso_big **big)
{
	/* Some controllers work best when Extended Advertising interval is a multiple
	 * of the ISO Interval minus 10 ms (max. advertising random delay). This is
	 * required to place the AUX_ADV_IND PDUs in a non-overlapping interval with the
	 * Broadcast ISO radio events.
	 */
	const uint16_t adv_interval = calc_adv_interval(big_create_param.interval);
	const uint16_t ext_adv_interval = adv_interval - BT_GAP_MS_TO_ADV_INTERVAL(10U);
	const struct bt_le_adv_param *ext_adv_param =
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_EXT_ADV, ext_adv_interval, ext_adv_interval, NULL);
	const struct bt_le_per_adv_param *per_adv_param =
		BT_LE_PER_ADV_PARAM(adv_interval, adv_interval, BT_LE_PER_ADV_OPT_NONE);
	int err;

	/* Create a non-connectable advertising set */
	LOG_INF("Creating Extended Advertising set");
	err = bt_le_ext_adv_create(ext_adv_param, NULL, adv);
	if (err != 0) {
		LOG_ERR("Failed to create advertising set (err %d)", err);
		return err;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(*adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Failed to set advertising data (err %d)", err);
		return err;
	}

	LOG_INF("Setting Periodic Advertising parameters");
	/* Set periodic advertising parameters */
	err = bt_le_per_adv_set_param(*adv, per_adv_param);
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

	if (*big != NULL) {
		err = bt_iso_big_terminate(*big);
		if (err != 0) {
			LOG_ERR("Failed to terminate BIG (err %d)", err);
			return err;
		}
		err = k_sem_take(&sem_big_term, K_FOREVER);
		if (err != 0) {
			LOG_ERR("failed to take sem_big_term (err %d)", err);
			return err;
		}
		*big = NULL;
	}

	if (*adv != NULL) {
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
	}

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
		int del_err;

		LOG_ERR("Could not create BIG: %d", err);

		del_err = delete_big(&adv, &big);
		if (del_err) {
			LOG_ERR("Could not delete BIG: %d", del_err);
		}

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
