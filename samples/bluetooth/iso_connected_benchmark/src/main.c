/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/types.h>


#include <zephyr/console/console.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(iso_connected, LOG_LEVEL_DBG);

#define DEVICE_NAME	CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME))

enum benchmark_role {
	ROLE_CENTRAL,
	ROLE_PERIPHERAL,
	ROLE_QUIT
};

enum sdu_dir {
	DIR_C_TO_P,
	DIR_P_TO_C
};

#define DEFAULT_CIS_RTN         2
#define DEFAULT_CIS_INTERVAL_US 7500
#define DEFAULT_CIS_LATENCY_MS  40
#define DEFAULT_CIS_PHY         BT_GAP_LE_PHY_2M
#define DEFAULT_CIS_SDU_SIZE    CONFIG_BT_ISO_TX_MTU
#define DEFAULT_CIS_PACKING     0
#define DEFAULT_CIS_FRAMING     0
#define DEFAULT_CIS_COUNT       1U
#define DEFAULT_CIS_SEC_LEVEL   BT_SECURITY_L1

#if defined(CONFIG_BT_ISO_TEST_PARAMS)
#define DEFAULT_CIS_NSE          BT_ISO_NSE_MIN
#define DEFAULT_CIS_BN           BT_ISO_BN_MIN
#define DEFAULT_CIS_PDU_SIZE     CONFIG_BT_ISO_TX_MTU
#define DEFAULT_CIS_FT           BT_ISO_FT_MIN
#define DEFAULT_CIS_ISO_INTERVAL DEFAULT_CIS_INTERVAL_US / 1250U /* N * 1.25 ms */
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

#define BUFFERS_ENQUEUED 2 /* Number of buffers enqueue for each channel */

BUILD_ASSERT(BUFFERS_ENQUEUED * CONFIG_BT_ISO_MAX_CHAN <= CONFIG_BT_ISO_TX_BUF_COUNT,
	     "Not enough buffers to enqueue");

struct iso_recv_stats {
	uint32_t iso_recv_count;
	uint32_t iso_lost_count;
};

struct iso_chan_work {
	struct bt_iso_chan chan;
	struct k_work_delayable send_work;
	struct bt_iso_info info;
	uint16_t seq_num;
} iso_chans[CONFIG_BT_ISO_MAX_CHAN];

static enum benchmark_role role;
static struct bt_conn *default_conn;
static struct bt_iso_chan *cis[CONFIG_BT_ISO_MAX_CHAN];
static bool advertiser_found;
static bt_addr_le_t adv_addr;
static uint32_t last_received_counter;
static struct iso_recv_stats stats_current_conn;
static struct iso_recv_stats stats_overall;
static int64_t iso_conn_start_time;
static size_t total_iso_conn_count;
static uint32_t iso_send_count;
static struct bt_iso_cig *cig;

NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
static uint8_t iso_data[CONFIG_BT_ISO_TX_MTU];

static K_SEM_DEFINE(sem_adv, 0, 1);
static K_SEM_DEFINE(sem_iso_accept, 0, 1);
static K_SEM_DEFINE(sem_iso_connected, 0, CONFIG_BT_ISO_MAX_CHAN);
static K_SEM_DEFINE(sem_iso_disconnected, 0, CONFIG_BT_ISO_MAX_CHAN);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_disconnected, 0, 1);

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = DEFAULT_CIS_SDU_SIZE, /* bytes */
	.rtn = DEFAULT_CIS_RTN,
	.phy = DEFAULT_CIS_PHY,
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	.max_pdu = DEFAULT_CIS_PDU_SIZE,
	.burst_number = DEFAULT_CIS_BN,
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

static struct bt_iso_chan_io_qos iso_rx_qos = {
	.sdu = DEFAULT_CIS_SDU_SIZE, /* bytes */
	.rtn = DEFAULT_CIS_RTN,
	.phy = DEFAULT_CIS_PHY,
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	.max_pdu = DEFAULT_CIS_PDU_SIZE,
	.burst_number = DEFAULT_CIS_BN,
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

static struct bt_iso_chan_qos iso_qos = {
	.tx = &iso_tx_qos,
	.rx = &iso_rx_qos,
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	.num_subevents = DEFAULT_CIS_NSE,
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

static struct bt_iso_cig_param cig_create_param = {
	.c_to_p_interval = DEFAULT_CIS_INTERVAL_US, /* in microseconds */
	.p_to_c_interval = DEFAULT_CIS_INTERVAL_US, /* in microseconds */
	.c_to_p_latency = DEFAULT_CIS_LATENCY_MS, /* milliseconds */
	.p_to_c_latency = DEFAULT_CIS_LATENCY_MS, /* milliseconds */
	.sca = BT_GAP_SCA_UNKNOWN,
	.packing = DEFAULT_CIS_PACKING,
	.framing = DEFAULT_CIS_FRAMING,
	.cis_channels = cis,
	.num_cis = DEFAULT_CIS_COUNT,
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	.c_to_p_ft = DEFAULT_CIS_FT,
	.p_to_c_ft = DEFAULT_CIS_FT,
	.iso_interval = DEFAULT_CIS_ISO_INTERVAL,
#endif /* CONFIG_BT_ISO_TEST_PARAMS */
};

static enum benchmark_role device_role_select(void)
{
	char role_char;
	const char central_char = 'c';
	const char peripheral_char = 'p';
	const char quit_char = 'q';

	while (true) {
		printk("Choose device role - type %c (central role) or %c (peripheral role), or %c to quit: ",
			central_char, peripheral_char, quit_char);

		role_char = tolower(console_getchar());

		printk("%c\n", role_char);

		if (role_char == central_char) {
			printk("Central role\n");
			return ROLE_CENTRAL;
		} else if (role_char == peripheral_char) {
			printk("Peripheral role\n");
			return ROLE_PERIPHERAL;
		} else if (role_char == quit_char) {
			printk("Quitting\n");
			return ROLE_QUIT;
		} else if (role_char == '\n' || role_char == '\r') {
			continue;
		}

		printk("Invalid role: %c\n", role_char);
	}
}

static void print_stats(char *name, struct iso_recv_stats *stats)
{
	uint32_t total_packets;

	total_packets = stats->iso_recv_count + stats->iso_lost_count;

	LOG_INF("%s: Received %u/%u (%.2f%%) - Total packets lost %u",
		name, stats->iso_recv_count, total_packets,
		(double)((float)stats->iso_recv_count * 100 / total_packets),
		stats->iso_lost_count);
}

static void iso_send(struct bt_iso_chan *chan)
{
	int ret;
	struct net_buf *buf;
	struct iso_chan_work *chan_work;
	uint32_t interval;

	chan_work = CONTAINER_OF(chan, struct iso_chan_work, chan);

	if (!chan_work->info.can_send) {
		return;
	}

	interval = (role == ROLE_CENTRAL) ?
		   cig_create_param.c_to_p_interval : cig_create_param.p_to_c_interval;

	buf = net_buf_alloc(&tx_pool, K_FOREVER);
	if (buf == NULL) {
		LOG_ERR("Could not allocate buffer");
		k_work_reschedule(&chan_work->send_work, K_USEC(interval));
		return;
	}

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	net_buf_add_mem(buf, iso_data, iso_tx_qos.sdu);

	ret = bt_iso_chan_send(chan, buf, chan_work->seq_num++);
	if (ret < 0) {
		LOG_ERR("Unable to send data: %d", ret);
		net_buf_unref(buf);
		k_work_reschedule(&chan_work->send_work, K_USEC(interval));
		return;
	}

	iso_send_count++;

	if ((iso_send_count % 100) == 0) {
		LOG_INF("Sending value %u", iso_send_count);
	}
}

static void iso_timer_timeout(struct k_work *work)
{
	struct bt_iso_chan *chan;
	struct iso_chan_work *chan_work;
	struct k_work_delayable *delayable = k_work_delayable_from_work(work);

	chan_work = CONTAINER_OF(delayable, struct iso_chan_work, send_work);
	chan = &chan_work->chan;

	iso_send(chan);
}

static void iso_sent(struct bt_iso_chan *chan)
{
	struct iso_chan_work *chan_work;

	chan_work = CONTAINER_OF(chan, struct iso_chan_work, chan);

	k_work_reschedule(&chan_work->send_work, K_MSEC(0));
}

static void iso_recv(struct bt_iso_chan *chan,
		     const struct bt_iso_recv_info *info,
		     struct net_buf *buf)
{
	uint32_t total_packets;
	static bool stats_latest_arr[1000];
	static size_t stats_latest_arr_pos;

	/* NOTE: The packets received may be on different CISes */

	if (info->flags & BT_ISO_FLAGS_VALID) {
		stats_current_conn.iso_recv_count++;
		stats_overall.iso_recv_count++;
		stats_latest_arr[stats_latest_arr_pos++] = true;
	} else {
		stats_current_conn.iso_lost_count++;
		stats_overall.iso_lost_count++;
		stats_latest_arr[stats_latest_arr_pos++] = false;
	}

	if (stats_latest_arr_pos == sizeof(stats_latest_arr)) {
		stats_latest_arr_pos = 0;
	}

	total_packets = stats_overall.iso_recv_count + stats_overall.iso_lost_count;

	if ((total_packets % 100) == 0) {
		struct iso_recv_stats stats_latest = { 0 };

		for (int i = 0; i < ARRAY_SIZE(stats_latest_arr); i++) {
			/* If we have not yet received 1000 packets, break
			 * early
			 */
			if (i == total_packets) {
				break;
			}

			if (stats_latest_arr[i]) {
				stats_latest.iso_recv_count++;
			} else {
				stats_latest.iso_lost_count++;
			}
		}

		print_stats("Overall     ", &stats_overall);
		print_stats("Current Conn", &stats_current_conn);
		print_stats("Latest 1000 ", &stats_latest);
		LOG_INF(""); /* Empty line to separate the stats */
	}
}

static void iso_connected(struct bt_iso_chan *chan)
{
	struct iso_chan_work *chan_work;
	int err;

	LOG_INF("ISO Channel %p connected", chan);

	chan_work = CONTAINER_OF(chan, struct iso_chan_work, chan);
	err = bt_iso_chan_get_info(chan, &chan_work->info);
	if (err != 0) {
		LOG_ERR("Could get info about chan %p: %d", chan, err);
	}

	/* If multiple CIS was created, this will be the value of the last
	 * created in the CIG
	 */
	iso_conn_start_time = k_uptime_get();

	chan_work = CONTAINER_OF(chan, struct iso_chan_work, chan);
	chan_work->seq_num = 0U;

	k_sem_give(&sem_iso_connected);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	/* Calculate cumulative moving average - Be aware that this may
	 * cause overflow at sufficiently large counts or durations
	 *
	 * This duration is calculated for each CIS disconnected from the time
	 * of the last created CIS.
	 */
	static int64_t average_duration;
	uint64_t iso_conn_duration;
	uint64_t total_duration;

	if (iso_conn_start_time > 0) {
		iso_conn_duration = k_uptime_get() - iso_conn_start_time;
	} else {
		iso_conn_duration = 0;
	}
	total_duration = iso_conn_duration + (total_iso_conn_count - 1) * average_duration;

	average_duration = total_duration / total_iso_conn_count;

	LOG_INF("ISO Channel %p disconnected with reason 0x%02x after "
		"%llu milliseconds (average duration %llu)",
		chan, reason, iso_conn_duration, average_duration);

	k_sem_give(&sem_iso_disconnected);
}

static struct bt_iso_chan_ops iso_ops = {
	.recv          = iso_recv,
	.connected     = iso_connected,
	.disconnected  = iso_disconnected,
	.sent          = iso_sent,
};

static int iso_accept(const struct bt_iso_accept_info *info,
		      struct bt_iso_chan **chan)
{
	LOG_INF("Incoming ISO request from %p", (void *)info->acl);

	for (int i = 0; i < ARRAY_SIZE(iso_chans); i++) {
		if (iso_chans[i].chan.state == BT_ISO_STATE_DISCONNECTED) {
			LOG_INF("Returning instance %d", i);
			*chan = &iso_chans[i].chan;
			cig_create_param.num_cis++;

			k_sem_give(&sem_iso_accept);
			return 0;
		}
	}

	LOG_ERR("Could not accept any more CIS");

	*chan = NULL;

	return -ENOMEM;
}

static struct bt_iso_server iso_server = {
#if defined(CONFIG_BT_SMP)
	.sec_level = DEFAULT_CIS_SEC_LEVEL,
#endif /* CONFIG_BT_SMP */
	.accept = iso_accept,
};

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
		__fallthrough;
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, DEVICE_NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static int start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err != 0) {
		LOG_ERR("Scan start failed: %d", err);
		return err;
	}

	LOG_INF("Scan started");

	return 0;
}

static int stop_scan(void)
{
	int err;

	err = bt_le_scan_stop();
	if (err != 0) {
		LOG_ERR("Scan stop failed: %d", err);
		return err;
	}

	LOG_INF("Scan stopped");

	return 0;
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[DEVICE_NAME_LEN];

	if (advertiser_found) {
		return;
	}

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	if (strncmp(DEVICE_NAME, name, strlen(DEVICE_NAME))) {
		return;
	}

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	LOG_INF("Found peripheral with address %s (RSSI %i)",
		le_addr, info->rssi);


	bt_addr_le_copy(&adv_addr, info->addr);
	advertiser_found = true;
	k_sem_give(&sem_adv);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0 && role == ROLE_CENTRAL) {
		LOG_INF("Failed to connect to %s: %u", addr, err);

		bt_conn_unref(default_conn);
		default_conn = NULL;
		return;
	} else if (role == ROLE_PERIPHERAL) {
		default_conn = bt_conn_ref(conn);
	}

	LOG_INF("Connected: %s", addr);

	k_sem_give(&sem_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;
	k_sem_give(&sem_disconnected);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
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

static int parse_rtn_arg(struct bt_iso_chan_io_qos *qos)
{
	char buffer[4];
	size_t char_count;
	uint64_t rtn;

	printk("Set RTN (current %u, default %u)\n",
	       qos->rtn, DEFAULT_CIS_RTN);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_RTN;
	}

	rtn = strtoul(buffer, NULL, 0);
	if (rtn > BT_ISO_CONNECTED_RTN_MAX) {
		printk("Invalid RTN %llu", rtn);
		return -EINVAL;
	}

	return (int)rtn;
}

static int parse_interval_arg(enum sdu_dir direction)
{
	char buffer[9];
	size_t char_count;
	uint64_t interval;

	interval = (direction == DIR_C_TO_P) ?
		   cig_create_param.c_to_p_interval : cig_create_param.p_to_c_interval;

	printk("Set %s interval (us) (current %llu, default %u)\n",
	       (direction == DIR_C_TO_P) ? "C to P" : "P to C",
	       interval, DEFAULT_CIS_INTERVAL_US);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_INTERVAL_US;
	}

	interval = strtoul(buffer, NULL, 0);
	if (interval < BT_ISO_SDU_INTERVAL_MIN || interval > BT_ISO_SDU_INTERVAL_MAX) {
		printk("Invalid interval %llu", interval);
		return -EINVAL;
	}

	return (int)interval;
}

static int parse_latency_arg(enum sdu_dir direction)
{
	char buffer[6];
	size_t char_count;
	uint64_t latency;

	latency = (direction == DIR_C_TO_P) ?
		  cig_create_param.c_to_p_latency : cig_create_param.p_to_c_latency;

	printk("Set %s latency (ms) (current %llu, default %u)\n",
	       (direction == DIR_C_TO_P) ? "C to P" : "P to C",
	       latency, DEFAULT_CIS_LATENCY_MS);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_LATENCY_MS;
	}

	latency = strtoul(buffer, NULL, 0);
	if (latency < BT_ISO_LATENCY_MIN || latency > BT_ISO_LATENCY_MAX) {
		printk("Invalid latency %llu", latency);
		return -EINVAL;
	}

	return (int)latency;
}

static int parse_phy_arg(struct bt_iso_chan_io_qos *qos)
{
	char buffer[3];
	size_t char_count;
	uint64_t phy;

	printk("Set PHY (current %u, default %u) - %u = 1M, %u = 2M, %u = Coded\n",
	       qos->phy, DEFAULT_CIS_PHY, BT_GAP_LE_PHY_1M,
	       BT_GAP_LE_PHY_2M, BT_GAP_LE_PHY_CODED);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_PHY;
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

static int parse_sdu_arg(struct bt_iso_chan_io_qos *qos)
{
	char buffer[6];
	size_t char_count;
	uint64_t sdu;

	printk("Set SDU (current %u, default %u)\n",
	       qos->sdu, DEFAULT_CIS_SDU_SIZE);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_SDU_SIZE;
	}

	sdu = strtoul(buffer, NULL, 0);
	if (sdu > MIN(BT_ISO_MAX_SDU, sizeof(iso_data)) ||
	    sdu < sizeof(uint32_t) /* room for the counter */) {
		printk("Invalid SDU %llu", sdu);
		return -EINVAL;
	}

	return (int)sdu;
}

#if defined(CONFIG_BT_ISO_TEST_PARAMS)
static int parse_c_to_p_ft_arg(void)
{
	char buffer[4];
	size_t char_count;
	uint64_t c_to_p_ft;

	printk("Set central to peripheral flush timeout (current %u, default %u)\n",
	       cig_create_param.c_to_p_ft, DEFAULT_CIS_FT);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_FT;
	}

	c_to_p_ft = strtoul(buffer, NULL, 0);
	if (!IN_RANGE(c_to_p_ft, BT_ISO_FT_MIN, BT_ISO_FT_MAX)) {
		printk("Invalid central to peripheral flush timeout %llu",
		       c_to_p_ft);

		return -EINVAL;
	}

	return (int)c_to_p_ft;
}

static int parse_p_to_c_ft_arg(void)
{
	char buffer[4];
	size_t char_count;
	uint64_t p_to_c_ft;

	printk("Set peripheral to central flush timeout (current %u, default %u)\n",
	       cig_create_param.p_to_c_ft, DEFAULT_CIS_FT);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_FT;
	}

	p_to_c_ft = strtoul(buffer, NULL, 0);
	if (!IN_RANGE(p_to_c_ft, BT_ISO_FT_MIN, BT_ISO_FT_MAX)) {
		printk("Invalid peripheral to central flush timeout %llu",
		       p_to_c_ft);

		return -EINVAL;
	}

	return (int)p_to_c_ft;
}

static int parse_iso_interval_arg(void)
{
	char buffer[8];
	size_t char_count;
	uint64_t iso_interval;

	printk("Set ISO interval (current %u, default %u)\n",
	       cig_create_param.iso_interval, DEFAULT_CIS_ISO_INTERVAL);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_ISO_INTERVAL;
	}

	iso_interval = strtoul(buffer, NULL, 0);
	if (IN_RANGE(iso_interval, BT_ISO_ISO_INTERVAL_MIN, BT_ISO_ISO_INTERVAL_MAX)) {
		printk("Invalid ISO interval %llu", iso_interval);

		return -EINVAL;
	}

	return (int)iso_interval;
}

static int parse_nse_arg(struct bt_iso_chan_qos *qos)
{
	char buffer[4];
	size_t char_count;
	uint64_t nse;

	printk("Set number of subevents (current %u, default %u)\n",
	       qos->num_subevents, DEFAULT_CIS_NSE);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_NSE;
	}

	nse = strtoul(buffer, NULL, 0);
	if (IN_RANGE(nse, BT_ISO_NSE_MIN, BT_ISO_NSE_MAX)) {
		printk("Invalid number of subevents %llu", nse);

		return -EINVAL;
	}

	return (int)nse;
}

static int parse_pdu_arg(const struct bt_iso_chan_io_qos *qos)
{
	char buffer[6];
	size_t char_count;
	uint64_t pdu;

	printk("Set PDU (current %u, default %u)\n",
	       qos->max_pdu, DEFAULT_CIS_PDU_SIZE);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_PDU_SIZE;
	}

	pdu = strtoul(buffer, NULL, 0);
	if (pdu > BT_ISO_PDU_MAX) {
		printk("Invalid PDU %llu", pdu);

		return -EINVAL;
	}

	return (int)pdu;
}

static int parse_bn_arg(const struct bt_iso_chan_io_qos *qos)
{
	char buffer[4];
	size_t char_count;
	uint64_t bn;

	printk("Set burst number (current %u, default %u)\n",
	       qos->burst_number, DEFAULT_CIS_BN);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_PDU_SIZE;
	}

	bn = strtoul(buffer, NULL, 0);
	if (!IN_RANGE(bn, BT_ISO_BN_MIN, BT_ISO_BN_MAX)) {
		printk("Invalid burst number %llu", bn);

		return -EINVAL;
	}

	return (int)bn;
}
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

static int parse_cis_count_arg(void)
{
	char buffer[4];
	size_t char_count;
	uint64_t cis_count;

	printk("Set CIS count (current %u, default %u)\n",
	       cig_create_param.num_cis, DEFAULT_CIS_COUNT);

	char_count = get_chars(buffer, sizeof(buffer) - 1);
	if (char_count == 0) {
		return DEFAULT_CIS_COUNT;
	}

	cis_count = strtoul(buffer, NULL, 0);
	if (cis_count > MAX(BT_ISO_MAX_GROUP_ISO_COUNT, CONFIG_BT_ISO_MAX_CHAN)) {
		printk("Invalid CIS count %llu", cis_count);
		return -EINVAL;
	}

	return (int)cis_count;
}

static int parse_cig_args(void)
{
	int c_to_p_interval;
	int p_to_c_interval;
	int c_to_p_latency;
	int p_to_c_latency;
	int cis_count;
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	int c_to_p_ft;
	int p_to_c_ft;
	int iso_interval;
	int num_subevents;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	printk("Follow the prompts. Press enter to use default values.\n");

	cis_count = parse_cis_count_arg();
	if (cis_count < 0) {
		return -EINVAL;
	}

	c_to_p_interval = parse_interval_arg(DIR_C_TO_P);
	if (c_to_p_interval < 0) {
		return -EINVAL;
	}

	p_to_c_interval = parse_interval_arg(DIR_P_TO_C);
	if (p_to_c_interval < 0) {
		return -EINVAL;
	}

	c_to_p_latency = parse_latency_arg(DIR_C_TO_P);
	if (c_to_p_latency < 0) {
		return -EINVAL;
	}

	p_to_c_latency = parse_latency_arg(DIR_P_TO_C);
	if (p_to_c_latency < 0) {
		return -EINVAL;
	}

#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	c_to_p_ft = parse_c_to_p_ft_arg();
	if (c_to_p_ft < 0) {
		return -EINVAL;
	}

	p_to_c_ft = parse_p_to_c_ft_arg();
	if (p_to_c_ft < 0) {
		return -EINVAL;
	}

	iso_interval = parse_iso_interval_arg();
	if (iso_interval < 0) {
		return -EINVAL;
	}

	num_subevents = parse_nse_arg(&iso_qos);
	if (num_subevents < 0) {
		return -EINVAL;
	}
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	cig_create_param.c_to_p_interval = c_to_p_interval;
	cig_create_param.p_to_c_interval = p_to_c_interval;
	cig_create_param.c_to_p_latency = c_to_p_latency;
	cig_create_param.p_to_c_latency = p_to_c_latency;
	cig_create_param.num_cis = cis_count;
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	cig_create_param.c_to_p_ft = c_to_p_ft;
	cig_create_param.p_to_c_ft = p_to_c_ft;
	cig_create_param.iso_interval = iso_interval;
	iso_qos.num_subevents = num_subevents;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	return 0;
}

static int parse_cis_args(struct bt_iso_chan_io_qos *qos)
{
	int rtn;
	int phy;
	int sdu;
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	int max_pdu;
	int burst_number;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	printk("Follow the prompts. Press enter to use default values.\n");

	rtn = parse_rtn_arg(qos);
	if (rtn < 0) {
		return -EINVAL;
	}

	phy = parse_phy_arg(qos);
	if (phy < 0) {
		return -EINVAL;
	}

	sdu = parse_sdu_arg(qos);
	if (sdu < 0) {
		return -EINVAL;
	}

#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	max_pdu = parse_pdu_arg(qos);
	if (max_pdu < 0) {
		return -EINVAL;
	}

	burst_number = parse_bn_arg(qos);
	if (burst_number < 0) {
		return -EINVAL;
	}
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	qos->rtn = rtn;
	qos->phy = phy;
	qos->sdu = sdu;
#if defined(CONFIG_BT_ISO_TEST_PARAMS)
	qos->max_pdu = max_pdu;
	qos->burst_number = burst_number;
#endif /* CONFIG_BT_ISO_TEST_PARAMS */

	return 0;
}

static int change_central_settings(void)
{
	char c;
	int err;

	printk("Change CIG settings (y/N)? (Current settings: cis_count=%u, "
	       "C to P interval=%u, P to C interval=%u "
	       "C to P latency=%u, P to C latency=%u)\n",
	       cig_create_param.num_cis, cig_create_param.c_to_p_interval,
	       cig_create_param.p_to_c_interval, cig_create_param.c_to_p_latency,
	       cig_create_param.p_to_c_latency);

	c = tolower(console_getchar());
	if (c == 'y') {
		err = parse_cig_args();
		if (err != 0) {
			return err;
		}

		printk("New settings: cis_count=%u, C to P interval=%u, "
		       "P TO C interval=%u, C to P latency=%u "
		       "P TO C latency=%u\n",
		       cig_create_param.num_cis, cig_create_param.c_to_p_interval,
		       cig_create_param.p_to_c_interval, cig_create_param.c_to_p_latency,
		       cig_create_param.p_to_c_latency);
	}

	printk("Change TX settings (y/N)? (Current settings: rtn=%u, "
	       "phy=%u, sdu=%u)\n",
	       iso_tx_qos.rtn, iso_tx_qos.phy, iso_tx_qos.sdu);

	c = tolower(console_getchar());
	if (c == 'y') {
		printk("Disable TX (y/N)?\n");
		c = tolower(console_getchar());
		if (c == 'y') {
			iso_qos.tx = NULL;
			printk("TX disabled\n");
		} else {
			iso_qos.tx = &iso_tx_qos;

			err = parse_cis_args(&iso_tx_qos);
			if (err != 0) {
				return err;
			}

			printk("New settings: rtn=%u, phy=%u, sdu=%u\n",
			       iso_tx_qos.rtn, iso_tx_qos.phy, iso_tx_qos.sdu);
		}
	}

	printk("Change RX settings (y/N)? (Current settings: rtn=%u, "
	       "phy=%u, sdu=%u)\n",
	       iso_rx_qos.rtn, iso_rx_qos.phy, iso_rx_qos.sdu);

	c = tolower(console_getchar());
	if (c == 'y') {
		printk("Disable RX (y/N)?\n");
		c = tolower(console_getchar());
		if (c == 'y') {
			if (iso_qos.tx == NULL) {
				LOG_ERR("Cannot disable both TX and RX\n");
				return -EINVAL;
			}

			iso_qos.rx = NULL;
			printk("RX disabled\n");
		} else {

			printk("Set RX settings to TX settings (Y/n)?\n");

			c = tolower(console_getchar());
			if (c == 'n') {
				err = parse_cis_args(&iso_rx_qos);
				if (err != 0) {
					return err;
				}

				printk("New settings: rtn=%u, phy=%u, sdu=%u\n",
				       iso_rx_qos.rtn, iso_rx_qos.phy,
				       iso_rx_qos.sdu);
			} else {
				(void)memcpy(&iso_rx_qos, &iso_tx_qos,
					     sizeof(iso_rx_qos));
			}
		}
	}

	return 0;
}

static int central_create_connection(void)
{
	int err;

	advertiser_found = false;

	err = start_scan();
	if (err != 0) {
		LOG_ERR("Could not start scan: %d", err);
		return err;
	}

	LOG_INF("Waiting for advertiser");
	err = k_sem_take(&sem_adv, K_FOREVER);
	if (err != 0) {
		LOG_ERR("failed to take sem_adv: %d", err);
		return err;
	}

	LOG_INF("Stopping scan");
	err = stop_scan();
	if (err != 0) {
		LOG_ERR("Could not stop scan: %d", err);
		return err;
	}

	LOG_INF("Connecting");
	err = bt_conn_le_create(&adv_addr, BT_CONN_LE_CREATE_CONN,
				BT_LE_CONN_PARAM_DEFAULT, &default_conn);
	if (err != 0) {
		LOG_ERR("Create connection failed: %d", err);
		return err;
	}

	err = k_sem_take(&sem_connected, K_FOREVER);
	if (err != 0) {
		LOG_ERR("failed to take sem_connected: %d", err);
		return err;
	}

	return 0;
}

static int central_create_cig(void)
{
	struct bt_iso_connect_param connect_param[CONFIG_BT_ISO_MAX_CHAN];
	int err;

	iso_conn_start_time = 0;

	LOG_INF("Creating CIG");

	err = bt_iso_cig_create(&cig_create_param, &cig);
	if (err != 0) {
		LOG_ERR("Failed to create CIG: %d", err);
		return err;
	}

	LOG_INF("Connecting ISO channels");

	for (int i = 0; i < cig_create_param.num_cis; i++) {
		connect_param[i].acl = default_conn;
		connect_param[i].iso_chan = &iso_chans[i].chan;
	}

	err = bt_iso_chan_connect(connect_param, cig_create_param.num_cis);
	if (err != 0) {
		LOG_ERR("Failed to connect iso: %d", err);
		return err;
	}
	total_iso_conn_count++;

	for (int i = 0; i < cig_create_param.num_cis; i++) {
		err = k_sem_take(&sem_iso_connected, K_FOREVER);
		if (err != 0) {
			LOG_ERR("failed to take sem_iso_connected: %d", err);
			return err;
		}
	}

	return 0;
}

static void reset_sems(void)
{
	(void)k_sem_reset(&sem_adv);
	(void)k_sem_reset(&sem_iso_accept);
	(void)k_sem_reset(&sem_iso_connected);
	(void)k_sem_reset(&sem_iso_disconnected);
	(void)k_sem_reset(&sem_connected);
	(void)k_sem_reset(&sem_disconnected);
}

static int cleanup(void)
{
	int err;

	for (size_t i = 0; i < cig_create_param.num_cis; i++) {
		(void)k_work_cancel_delayable(&iso_chans[i].send_work);
	}

	err = k_sem_take(&sem_disconnected, K_NO_WAIT);
	if (err != 0) {
		for (int i = 0; i < cig_create_param.num_cis; i++) {
			err = k_sem_take(&sem_iso_disconnected, K_NO_WAIT);
			if (err == 0) {
				err = bt_iso_chan_disconnect(&iso_chans[i].chan);
				if (err != 0) {
					LOG_ERR("Could not disconnect ISO[%d]: %d",
						i, err);
					break;
				}
			} /* else ISO already disconnected */
		}

		err = bt_conn_disconnect(default_conn,
					 BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err != 0) {
			LOG_ERR("Could not disconnect ACL: %d", err);
			return err;
		}

		err = k_sem_take(&sem_disconnected, K_FOREVER);
		if (err != 0) {
			LOG_ERR("failed to take sem_disconnected: %d", err);
			return err;
		}
	} /* else ACL already disconnected */

	if (cig) {
		err = bt_iso_cig_terminate(cig);
		if (err != 0) {
			LOG_ERR("Could not terminate CIG: %d", err);
			return err;
		}
		cig = NULL;
	}

	return err;
}

static int run_central(void)
{
	int err;
	char c;

	iso_conn_start_time = 0;
	last_received_counter = 0;
	memset(&stats_current_conn, 0, sizeof(stats_current_conn));
	reset_sems();

	printk("Change ISO settings (y/N)?\n");
	c = tolower(console_getchar());
	if (c == 'y') {
		err = change_central_settings();
		if (err != 0) {
			LOG_ERR("Failed to set parameters: %d", err);
			return err;
		}
	}

	err = central_create_connection();
	if (err != 0) {
		LOG_ERR("Failed to create connection: %d", err);
		return err;
	}

	err = central_create_cig();
	if (err != 0) {
		LOG_ERR("Failed to create CIG or connect CISes: %d", err);
		return err;
	}

	for (size_t i = 0; i < cig_create_param.num_cis; i++) {
		struct k_work_delayable *work = &iso_chans[i].send_work;

		k_work_init_delayable(work, iso_timer_timeout);

		for (int j = 0; j < BUFFERS_ENQUEUED; j++) {
			iso_send(&iso_chans[i].chan);
		}
	}

	err = k_sem_take(&sem_disconnected, K_FOREVER);
	if (err != 0) {
		LOG_ERR("failed to take sem_disconnected: %d", err);
		return err;
	}

	LOG_INF("Disconnected - Cleaning up");
	for (size_t i = 0; i < cig_create_param.num_cis; i++) {
		(void)k_work_cancel_delayable(&iso_chans[i].send_work);
	}

	for (int i = 0; i < cig_create_param.num_cis; i++) {
		err = k_sem_take(&sem_iso_disconnected, K_FOREVER);
		if (err != 0) {
			LOG_ERR("failed to take sem_iso_disconnected: %d", err);
			return err;
		}
	}

	err = bt_iso_cig_terminate(cig);
	if (err != 0) {
		LOG_ERR("Could not terminate CIG: %d", err);
		return err;
	}
	cig = NULL;

	return 0;
}

static int run_peripheral(void)
{
	int err;
	static bool initialized;

	/* Reset */
	cig_create_param.num_cis = 0;
	iso_conn_start_time = 0;
	last_received_counter = 0;
	memset(&stats_current_conn, 0, sizeof(stats_current_conn));
	reset_sems();

	if (!initialized) {
		LOG_INF("Registering ISO server");
		err = bt_iso_server_register(&iso_server);
		if (err != 0) {
			LOG_ERR("ISO server register failed: %d", err);
			return err;
		}
		initialized = true;
	}

	LOG_INF("Starting advertising");
	err = bt_le_adv_start(
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_ONE_TIME | BT_LE_ADV_OPT_CONNECTABLE |
					BT_LE_ADV_OPT_USE_NAME |
					BT_LE_ADV_OPT_FORCE_NAME_IN_AD,
				BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL),
		NULL, 0, NULL, 0);
	if (err != 0) {
		LOG_ERR("Advertising failed to start: %d", err);
		return err;
	}

	LOG_INF("Waiting for ACL connection");
	err = k_sem_take(&sem_connected, K_FOREVER);
	if (err != 0) {
		LOG_ERR("failed to take sem_connected: %d", err);
		return err;
	}

	err = bt_le_adv_stop();
	if (err != 0) {
		LOG_ERR("Advertising failed to stop: %d", err);
		return err;
	}

	LOG_INF("Waiting for ISO connection");

	err = k_sem_take(&sem_iso_accept, K_SECONDS(2));
	if (err != 0) {
		return err;
	}

	for (int i = 0; i < cig_create_param.num_cis; i++) {
		err = k_sem_take(&sem_iso_connected, K_FOREVER);
		if (err != 0) {
			LOG_ERR("failed to take sem_iso_connected: %d", err);
			return err;
		}
	}
	total_iso_conn_count++;

	for (size_t i = 0; i < cig_create_param.num_cis; i++) {
		struct k_work_delayable *work = &iso_chans[i].send_work;

		k_work_init_delayable(work, iso_timer_timeout);

		for (int j = 0; j < BUFFERS_ENQUEUED; j++) {
			iso_send(&iso_chans[i].chan);
		}
	}

	/* Wait for disconnect */
	err = k_sem_take(&sem_disconnected, K_FOREVER);
	if (err != 0) {
		LOG_ERR("failed to take sem_disconnected: %d", err);
		return err;
	}

	for (int i = 0; i < cig_create_param.num_cis; i++) {
		err = k_sem_take(&sem_iso_disconnected, K_FOREVER);
		if (err != 0) {
			LOG_ERR("failed to take sem_iso_disconnected: %d", err);
			return err;
		}
	}

	LOG_INF("Disconnected - Cleaning up");
	for (size_t i = 0; i < cig_create_param.num_cis; i++) {
		(void)k_work_cancel_delayable(&iso_chans[i].send_work);
	}

	return 0;
}

int main(void)
{
	int err;

	LOG_INF("Starting Bluetooth Throughput example");

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Bluetooth init failed: %d", err);
		return 0;
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_le_scan_cb_register(&scan_callbacks);

	err = console_init();
	if (err != 0) {
		LOG_ERR("Console init failed: %d", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized");

	for (int i = 0; i < ARRAY_SIZE(iso_chans); i++) {
		iso_chans[i].chan.ops = &iso_ops;
		iso_chans[i].chan.qos = &iso_qos;
		cis[i] = &iso_chans[i].chan;
	}

	/* Init data */
	for (int i = 0; i < iso_tx_qos.sdu; i++) {
		if (i < sizeof(iso_send_count)) {
			continue;
		}
		iso_data[i] = (uint8_t)i;
	}

	while (true) {
		int cleanup_err;

		role = device_role_select();

		if (role == ROLE_CENTRAL) {
			err = run_central();
		} else if (role == ROLE_PERIPHERAL) {
			err = run_peripheral();
		} else {
			if (role != ROLE_QUIT) {
				LOG_INF("Invalid role %u", role);
				continue;
			} else {
				err = 0;
				break;
			}
		}

		if (err != 0) {
			cleanup_err = cleanup();
			if (cleanup_err != 0) {
				LOG_ERR("Could not clean up: %d", err);
			}
		}

		LOG_INF("Test complete: %d", err);
	}

	LOG_INF("Exiting");
	return 0;
}
