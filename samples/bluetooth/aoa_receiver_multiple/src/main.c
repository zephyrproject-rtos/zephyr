/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <stdio.h>
#include <sys/__assert.h>

#include <sys/printk.h>
#include <sys/byteorder.h>
#include <sys/util.h>

#include <console/console.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/direction.h>

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define NAME_LEN        30
#define TIMEOUT_SYNC_CREATE K_SECONDS(5)

typedef struct device_t {
	struct k_work_delayable sync_timeout_timer;
	struct bt_le_per_adv_sync *sync;
	bt_addr_le_t per_addr;
	int8_t per_sid;
	bool is_synced;
	bool is_used;
	uint32_t num_cte;
} device_t;

static int enable_cte_rx(device_t *device);

static device_t sync_devices[CONFIG_BT_PER_ADV_SYNC_MAX];

static bool scan_enabled;
static bool is_syncing = false; // Can only sync to one at a time

#if defined(CONFIG_BT_CTLR_DF_ANT_SWITCH_RX)
const static uint8_t ant_patterns[] = { 0x0c, 0x0c, 0x0e, 0x0f, 0x04, 0x06, 0x0c, 0x0e, 0x0f, 0x04, 0x06, 0x0a, 0x08, 0x09, 0x02, 0x00, 0x0a, 0x08, 0x09, 0x02, 0x00 };
#endif /* CONFIG_BT_CTLR_DF_ANT_SWITCH_RX */

static bool data_cb(struct bt_data *data, void *user_data);
static int create_sync(const bt_addr_le_t *per_addr, uint8_t per_sid,
			struct bt_le_per_adv_sync **sync);
static void scan_recv(const struct bt_le_scan_recv_info *info,
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
static void cte_recv_cb(struct bt_le_per_adv_sync *sync,
			struct bt_df_per_adv_sync_iq_samples_report const *report);

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

	//printk("(%d) PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	//       "Interval 0x%04x (%u ms), PHY %s\n",
	//	   k_uptime_get_32(),
	//       bt_le_per_adv_sync_get_index(sync), le_addr,
	//       info->interval, info->interval * 5 / 4, phy2str(info->phy));

	uint8_t device_index = bt_le_per_adv_sync_get_index(sync);
	is_syncing = false;
	sync_devices[device_index].is_synced = true;
	k_work_cancel_delayable(&sync_devices[device_index].sync_timeout_timer);

	int err = enable_cte_rx(&sync_devices[device_index]);
	if (err) {
		err = bt_le_per_adv_sync_delete(sync_devices[device_index].sync);
		if (err) {
			printk("Failed cancel sync\n");
		}
		printk("Clear device index: %d\n", device_index);
		memset(&sync_devices[device_index].per_addr, 0, sizeof(bt_addr_le_t));
		sync_devices[device_index].sync = NULL;
		sync_devices[device_index].is_used = false;
		sync_devices[device_index].is_synced = false;
	}
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	//printk("(%d) PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	//		k_uptime_get_32(),
	//       bt_le_per_adv_sync_get_index(sync), le_addr);
	uint8_t device_index = bt_le_per_adv_sync_get_index(sync);
	printk("Clear device index: %d\n", device_index);
	memset(&sync_devices[device_index].per_addr, 0, sizeof(bt_addr_le_t));
	sync_devices[device_index].sync = NULL;
	sync_devices[device_index].is_used = false;
	sync_devices[device_index].is_synced = false;
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	/*
	char le_addr[BT_ADDR_LE_STR_LEN];
	char data_str[129];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	bin2hex(buf->data, buf->len, data_str, sizeof(data_str));

	printk("(%d) PER_ADV_SYNC[%u]:\n",
		   k_uptime_get_32(),
	       bt_le_per_adv_sync_get_index(sync));

	printk("(%d) PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
	       "RSSI %i, CTE %s, data length %u, data: %s\n",
		   k_uptime_get_32(),
	       bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
	       info->rssi, cte_type2str(info->cte_type), buf->len, data_str);
	*/
}

static void cte_recv_cb(struct bt_le_per_adv_sync *sync,
			struct bt_df_per_adv_sync_iq_samples_report const *report)
{
	uint8_t device_index = bt_le_per_adv_sync_get_index(sync);
	sync_devices[device_index].num_cte++;

}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];
	struct bt_le_per_adv_sync *sync;
	uint8_t device_index;
	(void)memset(name, 0, sizeof(name));
	static int no_spam_counter = 0;

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	if (no_spam_counter % 500 == 0) {
		printk("Scan is running...\n");
	}
	no_spam_counter++;
	// To much spam removed below
	/*
	printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s C:%u S:%u "
	       "D:%u SR:%u E:%u Prim: %s, Secn: %s, Interval: 0x%04x (%u ms), "
	       "SID: %u\n",
	       le_addr, info->adv_type, info->tx_power, info->rssi, name,
	       (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
	       phy2str(info->primary_phy), phy2str(info->secondary_phy),
	       info->interval, info->interval * 5 / 4, info->sid);
	*/
	if (info->interval != 0 && !is_syncing) {
		// Do not try to sync with device we already synced with.
		// Could probably also use bt_le_per_adv_sync_lookup_addr(...) or similar
		for (int i = 0; i < CONFIG_BT_PER_ADV_SYNC_MAX; i++) {
			if (bt_addr_le_cmp(info->addr, &sync_devices[i].per_addr) == 0) {
				return;
			}
		}

		printk("(%d) Found device sending per adv: %s\n", k_uptime_get_32(), le_addr);
		int err = create_sync(info->addr, info->sid, &sync);
		if (err != 0) {
			return;
		}
		is_syncing = true;
		device_index = bt_le_per_adv_sync_get_index(sync);
		printk("Sync index: %d\n", device_index);
		bt_addr_le_copy(&sync_devices[device_index].per_addr, info->addr);
		sync_devices[device_index].per_sid = info->sid;
		sync_devices[device_index].sync = sync;
		sync_devices[device_index].is_used = true;
		sync_devices[device_index].is_synced = false;
		err = k_work_schedule(&sync_devices[device_index].sync_timeout_timer, TIMEOUT_SYNC_CREATE);
		if (err != 1) {
			printk("Failed schedule sync timeout\n");
			bt_le_per_adv_sync_delete(sync_devices[device_index].sync);
		}
	}
}

static int create_sync(const bt_addr_le_t *per_addr, uint8_t per_sid, struct bt_le_per_adv_sync **sync)
{
	int err;
	struct bt_le_per_adv_sync_param sync_create_param;

	printk("(%d) Creating Periodic Advertising Sync...\n", k_uptime_get_32());
	bt_addr_le_copy(&sync_create_param.addr, per_addr);
	sync_create_param.options = 0;//BT_LE_PER_ADV_SYNC_OPT_SYNC_ONLY_CONST_TONE_EXT;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0x100;//0xa; // Increased timeout as otherwise seems to lose sync very often
	err = bt_le_per_adv_sync_create(&sync_create_param, sync);
	if (err == -EBUSY) {
		printk("Sync for some device already in progress.\n");
	} else if (err == -ENOMEM) {
		printk("No free adv sync availible\n");
	}

	if (err != 0) {
		printk("failed (err %d)\n", err);
		return err;
	}

	printk("success.\n");
	return 0;
}

static int enable_cte_rx(device_t *device)
{
	int err;

	const struct bt_df_per_adv_sync_cte_rx_param cte_rx_params = {
		.max_cte_count = 1,
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
	err = bt_df_per_adv_sync_cte_rx_enable(device->sync, &cte_rx_params);
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return err;
	}
	printk("(%d) success. CTE receive enabled.\n", k_uptime_get_32());
	return 0;
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
	int err;
	struct bt_le_scan_param param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE, //BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval   = 200U,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
		.timeout    = 0U,
	};

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

static void sync_create_timeout(struct k_work *work)
{
	int err;
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    device_t *device = CONTAINER_OF(dwork, device_t, sync_timeout_timer);
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&device->per_addr, le_addr, sizeof(le_addr));
	is_syncing = false;
	if (!device->is_synced) {
    	printk("(%d) sync_create_timeout for %s\n", k_uptime_get_32(), le_addr);
		err = bt_le_per_adv_sync_delete(device->sync);
        if (err) {
            printk("Failed cancel sync: %d\n", err);
        } else {
            printk("Sync deleted\n");
        }
		// Cleanup
		if (!err) {
			memset(&device->per_addr, 0, sizeof(bt_addr_le_t));
			device->sync = NULL;
			device->is_used = false;
			device->is_synced = false;
		}
	} else {
		printk("Guess we got a sync before delayed work was canceled. Ignore.\n");
	}
}


void print_stats()
{
	for (int i = 0; i < CONFIG_BT_PER_ADV_SYNC_MAX; i++) {
		printk("%d\t", i);
    }
	printk("\n");
    for (int i = 0; i < CONFIG_BT_PER_ADV_SYNC_MAX; i++) {
		if (sync_devices[i].is_used) {
			printk("%d\t", sync_devices[i].num_cte);
		} else {
			printk("-\t");
		}
		sync_devices[i].num_cte = 0;
    }
	printk("\n");
	printk("------------------------------------------------------------------------------------\n");
}


void main(void)
{
	int err;
	printk("Starting Connectionless Locator Demo\n");

	memset(sync_devices, 0, sizeof(sync_devices));

	for (int i = 0; i < CONFIG_BT_PER_ADV_SYNC_MAX; i++) {
        k_work_init_delayable(&sync_devices[i].sync_timeout_timer, sync_create_timeout);
    }

	printk("Bluetooth initialization...");

	err = bt_enable(NULL);
	if (err != 0) {
		printk("failed (err %d)\n", err);
	}
	printk("success\n");

	scan_init();
	scan_enable();

	printk("Waiting for periodic advertising...\n");

	while(true) {
		k_msleep(1000);
		print_stats();
		//scan_disable();
		//scan_enable();
	}
}
