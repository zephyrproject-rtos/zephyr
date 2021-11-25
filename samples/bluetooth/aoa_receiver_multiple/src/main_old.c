/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <stdio.h>

#include <sys/printk.h>
#include <sys/util.h>

#include <console/console.h>
#include <drivers/uart.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/direction.h>
/*
RSSI
Channel
TAG_ID

*/
#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define NAME_LEN        30
#define TIMEOUT_SYNC_CREATE K_SECONDS(10)

static struct bt_le_per_adv_sync_param sync_create_param;
static struct bt_le_per_adv_sync *sync;
static bt_addr_le_t per_addr;
static bool per_adv_found;
static bool scan_enabled;
static uint8_t per_sid;

const struct device *uart_dev;

const struct uart_config uart_cfg = {
	.baudrate = 115200,
	.parity = UART_CFG_PARITY_NONE,
	.stop_bits = UART_CFG_STOP_BITS_1,
	.data_bits = UART_CFG_DATA_BITS_8,
	.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS
};

static char iq_output[1000] = {0};
static uint8_t* uart_tx_buf;
static uint32_t uart_bytes_sent;
static uint32_t uart_bytes_to_send;
volatile bool uart_tx_in_progress = false;

static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
const static uint8_t ant_patterns[] = { 0x0c, 0x0c, 0x0e, 0x0f, 0x04, 0x06, 0x0c, 0x0e, 0x0f, 0x04, 0x06, 0x0a, 0x08, 0x09, 0x02, 0x00, 0x0a, 0x08, 0x09, 0x02, 0x00 };


#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */

static bool data_cb(struct bt_data *datc, void *user_data);
static void create_sync(void);
static void scan_recv(const struct bt_le_scan_recv_info *infc,
		      struct net_buf_simple *buf);

static void sync_cb(struct bt_le_per_adv_sync *sync,
		    struct bt_le_per_adv_sync_synced_info *info);
static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info);
static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf);
static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf);
static void scan_disable(void);
static void cte_recv_cb(struct bt_le_per_adv_sync *sync,
			struct bt_df_per_adv_sync_iq_samples_report const *report);


static void uart_fifo_callback(const struct device *dev, void *user_data);
static int send_iq_data(uint8_t* data, uint32_t len);

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.recv = recv_cb,
	.cte_report_cb = cte_recv_cb,
};

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

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

static const char *cte_type2str(uint8_t type)
{
	switch (type) {
	case BT_DF_CTE_TYPE_AOA: return "AOA";
	case BT_DF_CTE_TYPE_AOD_1US: return "AOD 1 [us]";
	case BT_DF_CTE_TYPE_AOD_2US: return "AOD 2 [us]";
	case BT_DF_CTE_TYPE_NONE: return "";
	default: return "Unknown";
	}
}

static const char *pocket_status2str(uint8_t status)
{
	switch (status) {
	case BT_DF_CTE_CRC_OK: return "CRC OK";
	case BT_DF_CTE_CRC_ERR_CTE_BASED_TIME: return "CRC not OK, CTE Info OK";
	case BT_DF_CTE_CRC_ERR_CTE_BASED_OTHER: return "CRC not OK, Sampled other way";
	case BT_DF_CTE_INSUFFICIENT_RESOURCES: return "No resources";
	default: return "Unknown";
	}
}

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

static void sync_cb(struct bt_le_per_adv_sync *sync,
		    struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	       "Interval 0x%04x (%u ms), PHY %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr,
	       info->interval, info->interval * 5 / 4, phy2str(info->phy));

	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);

	k_sem_give(&sem_per_sync_lost);
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char data_str[129];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	bin2hex(buf->data, buf->len, data_str, sizeof(data_str));


	//printk("PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
	//       "RSSI %i, CTE %s, data length %u, data: %s\n",
	//       bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
	//       info->rssi, cte_type2str(info->cte_type), buf->len, data_str);
}

static void cte_recv_cb(struct bt_le_per_adv_sync *sync,
			struct bt_df_per_adv_sync_iq_samples_report const *report)
{
	struct bt_le_per_adv_sync_info info;
	if (uart_tx_in_progress) {
		printk("Abort\n");
		return;
	}
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_le_per_adv_sync_get_info(sync, &info);
	bt_addr_le_to_str(&info.addr, le_addr, sizeof(le_addr));
	memset(iq_output, 0, sizeof(iq_output));
	int index = 0; 
	index += sprintf(&iq_output[index], "{\"measurement\": {\"id\": \"%s\", \"rssi\": %d, \"seq_nbr\": %d, \"cfg\":{\"ch\": %d}, \"raw_data\":{\"iq_samples\": [", le_addr, report->rssi, report->per_evt_counter, report->chan_idx);
	for (int i = 0; i < report->sample_count; i++) {
		if (i == report->sample_count - 1) {
			index += sprintf(&iq_output[index], "%d, %d", report->sample[i].i, report->sample[i].q );
		} else {
			index += sprintf(&iq_output[index], "%d, %d, ", report->sample[i].i , report->sample[i].q );
		}
	}

	index += sprintf(&iq_output[index], "]}}}\r\n");
	//printk("%s", iq_output);
	int ret =  send_iq_data(iq_output, index);
	if (ret) {
		printk("UART TX failed: %d\n", ret);
	} else {
		//printk("UART TX OK\n");
	}
	//printk("RSSI: %d\n", report->rssi);

	//printk("CTE[%u]: samples count %d, cte type %s, slot durations: %u [us], "
	//       "packet status %s, RSSI %i\n",
	//       bt_le_per_adv_sync_get_index(sync), report->sample_count,
	//       cte_type2str(report->cte_type), report->slot_durations,
	//       pocket_status2str(report->packet_status), report->rssi);
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	static int counter = 0;
	if (counter % 500 == 0) {
		//printk("Scan running\n");
	}
	counter++;


	if (!per_adv_found && info->interval != 0) {
		//printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s C:%u S:%u "
		//	"D:%u SR:%u E:%u Prim: %s, Secn: %s, Interval: 0x%04x (%u ms), "
		//	"SID: %u\n",
		//	le_addr, info->adv_type, info->tx_power, info->rssi, name,
		//	(info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
		//	(info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
		//	(info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
		//	(info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
		//	(info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
		//	phy2str(info->primary_phy), phy2str(info->secondary_phy),
		//	info->interval, info->interval * 5 / 4, info->sid);
		per_adv_found = true;
		per_sid = info->sid;
		bt_addr_le_copy(&per_addr, info->addr);

		k_sem_give(&sem_per_adv);
	}
}

static void create_sync(void)
{
	int err;

	printk("Creating Periodic Advertising Sync...");
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = 0;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0x100;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success.\n");
}

static int delete_sync(void)
{
	int err;

	printk("Deleting Periodic Advertising Sync...");
	err = bt_le_per_adv_sync_delete(sync);
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return err;
	}
	printk("success\n");

	return 0;
}

static void enable_cte_rx(void)
{
	int err;

	const struct bt_df_per_adv_sync_cte_rx_param cte_rx_params = {
		.max_cte_count = 10,
#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
		.cte_type = BT_DF_CTE_TYPE_ALL,
		.slot_durations = 0x1,
		.num_ant_ids = ARRAY_SIZE(ant_patterns),
		.ant_ids = ant_patterns,
#else
		.cte_type = BT_DF_CTE_TYPE_AOD_1US | BT_DF_CTE_TYPE_AOD_2US,
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */
	};

	printk("Enable receiving of CTE...\n");
	err = bt_df_per_adv_sync_cte_rx_enable(sync, &cte_rx_params);
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("success. CTE receive enabled.\n");
}

static int scan_init(void)
{
	printk("Scan callbacks register...");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register...");
	bt_le_per_adv_sync_cb_register(&sync_callbacks);
	printk("success.\n");

	return 0;
}

static int scan_enable(void)
{
	struct bt_le_scan_param param = {
			.type       = BT_LE_SCAN_TYPE_ACTIVE,
			.options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
			.interval   = BT_GAP_SCAN_FAST_INTERVAL,
			.window     = BT_GAP_SCAN_FAST_WINDOW,
			.timeout    = 0U, };
	int err;

	if (!scan_enabled) {
		printk("Start scanning...");
		err = bt_le_scan_start(&param, NULL);
		if (err != 0) {
			printk("failed (err %d)\n", err);
			return err;
		}
		printk("success\n");
		scan_enabled = true;
	}

	return 0;
}

static void scan_disable(void)
{
	int err;

	printk("Scan disable...");
	err = bt_le_scan_stop();
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("Success.\n");

	scan_enabled = false;
}

void main(void)
{
	int err;

	printk("Starting Connectionless Locator Demo\n");

	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));
	int ret = 0;// uart_configure(uart_dev, &uart_cfg);

	if (ret == -ENOSYS) {
		printk("UART initialization failed\n");
	} else {
		printk("UART initializated asdf\n");
	}

	if (!device_is_ready(uart_dev)) {
		printk("UART device not ready\n");
		return;
	}
	memset(iq_output, 0, sizeof(iq_output));

	printk("Bluetooth initialization...");
	err = bt_enable(NULL);
	if (err != 0) {
		printk("failed (err %d)\n", err);
	}
	printk("success\n");

	scan_init();

	scan_enabled = false;
	do {
		scan_enable();

		printk("Waiting for periodic advertising...");
		per_adv_found = false;
		err = k_sem_take(&sem_per_adv, K_FOREVER);
		if (err != 0) {
			printk("failed (err %d)\n", err);
			return;
		}
		printk("success. Found periodic advertising.\n");

		create_sync();

		printk("Waiting for periodic sync...\n");
		err = k_sem_take(&sem_per_sync, TIMEOUT_SYNC_CREATE);
		if (err != 0) {
			printk("failed (err %d)\n", err);
			err = delete_sync();
			if (err != 0) {
				return;
			}
			continue;
		}
		printk("success. Periodic sync established.\n");

		enable_cte_rx();

		/* Disable scan to cleanup output */
		scan_disable();

		printk("Waiting for periodic sync lost...\n");
		err = k_sem_take(&sem_per_sync_lost, K_FOREVER);
		if (err != 0) {
			printk("failed (err %d)\n", err);
			return;
		}
		printk("Periodic sync lost.\n");
	} while (true);
}


static int send_iq_data(uint8_t* data, uint32_t len)
{
	if (uart_tx_in_progress) {
		return -1;
	}
	uart_tx_in_progress = true;
	uart_tx_buf = data;
	uart_bytes_sent = 0;
	uart_bytes_to_send = len;

	/* Verify uart_irq_callback_set() */
	uart_irq_callback_set(uart_dev, uart_fifo_callback);

	/* Enable Tx/Rx interrupt before using fifo */
	/* Verify uart_irq_tx_enable() */
	uart_irq_tx_enable(uart_dev);

	return 0;
}

static void uart_fifo_callback(const struct device *dev, void *user_data)
{
	static int tx_data_idx;
	static int len_to_send;
	ARG_UNUSED(user_data);

	/* Verify uart_irq_update() */
	if (!uart_irq_update(dev)) {
		printk("retval should always be 1\n");
		return;
	}

	/* Verify uart_irq_tx_ready() */
	/* Note that TX IRQ may be disabled, but uart_irq_tx_ready() may
	 * still return true when ISR is called for another UART interrupt,
	 * hence additional check for i < DATA_SIZE.
	 */
	if (uart_irq_tx_ready(dev) && tx_data_idx < uart_bytes_to_send) {
		/* We arrive here by "tx ready" interrupt, so should always
		 * be able to put at least one byte into a FIFO. If not,
		 * well, we'll fail test.
		 */
		len_to_send = uart_bytes_to_send - uart_bytes_sent;
		uart_bytes_sent += uart_fifo_fill(dev, (uint8_t *)&uart_tx_buf[uart_bytes_sent], len_to_send);

		if (uart_bytes_sent == uart_bytes_to_send) {
			/* If we transmitted everything, stop IRQ stream,
			 * otherwise main app might never run.
			 */
			uart_irq_tx_disable(dev);
			uart_tx_in_progress = false;
		}
	}
}