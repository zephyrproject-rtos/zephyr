/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <errno.h>
#include <zephyr/zephyr.h>
#include <stdio.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/console/console.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/direction.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_WRN);

// Define to simulate angle calculations running in some another thread.
//#define BURN_CPU

// Define to restart scanning and terminate all synced devices after defined number
// of devices are synced. Used to stress the BT controller.
// The lower number the more often scanning is restarted.
#define NUM_SYNC_TO_RESTART_SCAN 2

// Uncomment below if CTE sampling shall be enabled after successful sync.
#define SAMPLE_CTE

// Comment below if syncing without adv list (one tag at a time).
#define USE_PER_ADV_LIST

// Select either or both for tag state debugging

// +UUDFDBG:5293.2,16052.2,6514.2,3986.2,12064.2,1862.2,1627.2,4139.2,819.2,11194.2,7473.2,9394.2,9492.2,3799.2,15619.2,5571.2,11080.2,5432.2,5672.2,14029.2,
//#define PRINT_STATS_FOR_PLOT

// TAGS:0,0,sync...,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,-,
#define PRINT_STATS_FOR_READING

#ifdef USE_PER_ADV_LIST
#define SYNCED_DEVICE_QUEUE_SIZE CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST_SIZE
#else
#define SYNCED_DEVICE_QUEUE_SIZE CONFIG_BT_PER_ADV_SYNC_MAX
#endif

//#define DEBUG_ANCHOR_LOGS 1
#ifdef DEBUG_ANCHOR_LOGS
#define DF_LOG printk
#else
#define DF_LOG(...)
#endif

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define NAME_LEN        30
#define TIMEOUT_SYNC_CREATE K_SECONDS(5)
#define SYNC_LOST_TIMEOUT_10MS_UNIT	300

#define LIST_INDEX_NONE 0xFF

K_THREAD_STACK_DEFINE(threadStack, 1024);


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

static device_t sync_devices[SYNCED_DEVICE_QUEUE_SIZE];

static struct bt_le_per_adv_sync   *syncForList;

struct k_thread threadData;

static bool scan_enabled;
static int scan_enable(void);
static void scan_disable(void);
static bool is_syncing = false; // Can only sync to one at a time

static int num_tags;

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

static int findTagIndexInSyncedListByAddr(const bt_addr_le_t *addr, int *const freeIndex)
{
	if (freeIndex) {
        *freeIndex = LIST_INDEX_NONE;
    }
    for (int i = 0; i < SYNCED_DEVICE_QUEUE_SIZE; i++) {
        if (sync_devices[i].is_used &&
            bt_addr_le_cmp(addr, &sync_devices[i].per_addr) == 0) {
            return i;
        } else if (freeIndex != NULL && !sync_devices[i].is_used && *freeIndex == LIST_INDEX_NONE) {
            *freeIndex = i;
        }
    }

    return LIST_INDEX_NONE;
}

static void restart_per_adv_list_syncing(void)
{
    int err;
    struct bt_le_per_adv_sync_param sync_create_param;
    // Check if we need to create new sync
	LOG_INF("Creating Sync for tags in list");
	sync_create_param.options = BT_LE_PER_ADV_SYNC_OPT_USE_PER_ADV_LIST;
	sync_create_param.skip = 0;
	sync_create_param.sid = 0; // Not used but needs to be 0 otherwise error
	sync_create_param.timeout = SYNC_LOST_TIMEOUT_10MS_UNIT;
	memset(&sync_create_param.addr, 0xFF, sizeof(bt_addr_le_t));
	err = bt_le_per_adv_sync_create(&sync_create_param, &syncForList);
	__ASSERT(err == 0 || err == -EBUSY || err == -ENOMEM, "Failed enably sync: %d", err);
	if (err == -EBUSY) {
		LOG_WRN("Per syncing already enabled!");
	} else if (err == -ENOMEM) {
		LOG_WRN("Max synced tags reached!");
	}
}

static void sync_cb(struct bt_le_per_adv_sync *sync,
			struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	//printk("%dms\n", info->interval * 5 / 4);
	DF_LOG("(%d) PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	       "Interval 0x%04x (%u ms), PHY %s\n",
		   k_uptime_get_32(),
	       bt_le_per_adv_sync_get_index(sync), le_addr,
	       info->interval, info->interval * 5 / 4, phy2str(info->phy));

	if (sync == syncForList) {
		syncForList = NULL;
	}

#ifdef USE_PER_ADV_LIST
	uint8_t device_index = findTagIndexInSyncedListByAddr(info->addr, NULL);
	if (device_index == LIST_INDEX_NONE) {
		LOG_ERR("Got sync from none queued tag!");
		restart_per_adv_list_syncing();
		return;
	}
	sync_devices[device_index].sync = sync;
#else
	uint8_t device_index = bt_le_per_adv_sync_get_index(sync);
	k_work_cancel_delayable(&sync_devices[device_index].sync_timeout_timer);
#endif
	is_syncing = false;
	sync_devices[device_index].is_synced = true;
#ifdef USE_PER_ADV_LIST
	int err = bt_le_per_adv_list_remove(info->addr, info->sid);
	LOG_DBG("Removed tag from per adv list");
	if (err != 0) {
		LOG_ERR("Expected tag to be in controller adv_list. Is there a race?");
	}
	restart_per_adv_list_syncing();
#endif

#ifdef SAMPLE_CTE
	err = enable_cte_rx(&sync_devices[device_index]);
	if (err) {
		err = bt_le_per_adv_sync_delete(sync_devices[device_index].sync);
		if (err) {
			printk("sync_cb: Failed cancel sync\n");
		}
		printk("Clear device index: %d\n", device_index);
		memset(&sync_devices[device_index].per_addr, 0, sizeof(bt_addr_le_t));
		sync_devices[device_index].sync = NULL;
		sync_devices[device_index].is_used = false;
		sync_devices[device_index].is_synced = false;
	 }
#endif

}

static void term_cb(struct bt_le_per_adv_sync *sync,
			const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	DF_LOG("(%d) PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated reason: %d\n",
			k_uptime_get_32(),
	       bt_le_per_adv_sync_get_index(sync), le_addr, info->reason);
#ifdef USE_PER_ADV_LIST
	uint8_t device_index = findTagIndexInSyncedListByAddr(info->addr, NULL);
	if (device_index == LIST_INDEX_NONE) {
		LOG_ERR("Got sync_term from none queued tag!");
		//__ASSERT(false, "Got sync_term from none queued tag!");
		restart_per_adv_list_syncing();
		return;
	}
#else
	uint8_t device_index = bt_le_per_adv_sync_get_index(sync);
#endif

	if (sync_devices[device_index].is_synced) {
		LOG_DBG("Device was synced: ");
	} else if (sync_devices[device_index].is_used) {
		LOG_WRN("(%d)___Device not synced cancel sync tmo\n",  k_uptime_get_32());
		LOG_WRN("\ninfo->reason: %d\n", info->reason);
		is_syncing = false;
#ifndef USE_PER_ADV_LIST
		int err = k_work_cancel_delayable(&sync_devices[device_index].sync_timeout_timer);
		if (err) {
			DF_LOG("Failed k_work_cancel_delayable: %d\n", err);
		}
#else
	// term_cb for a none synced tag means sync to that tag failed and
	// that we need to restart syncing again.
	restart_per_adv_list_syncing();
#endif
	} else {
		LOG_ERR("Device unknown state: ");
	}
#ifdef USE_PER_ADV_LIST
	if (sync_devices[device_index].is_synced) {
		int err = bt_le_per_adv_list_add(info->addr, info->sid);
		if (err) {
			LOG_ERR("Failed adding to re-add bt_le_per_adv_list_add err= %d", err);
			return;
		} else {
			LOG_DBG("Removed tag from per adv list");
			sync_devices[device_index].is_used = true;
			sync_devices[device_index].is_synced = false;
			sync_devices[device_index].sync = NULL;
		}
	}
#else
	DF_LOG("term_cb: clear device index: %d\n", device_index);
	memset(&sync_devices[device_index].per_addr, 0, sizeof(bt_addr_le_t));
	sync_devices[device_index].sync = NULL;
	sync_devices[device_index].is_used = false;
	sync_devices[device_index].is_synced = false;
#endif
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
			const struct bt_le_per_adv_sync_recv_info *info,
			struct net_buf_simple *buf)
{

	/*
	char le_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	char data_str[129];
	bin2hex(buf->data, buf->len, data_str, sizeof(data_str));
	DF_LOG("(%d) PER_ADV_SYNC[%u]:\n",
		   k_uptime_get_32(),
		   bt_le_per_adv_sync_get_index(sync));

	DF_LOG("(%d) PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
		   "RSSI %i, CTE %s, data length %u, data: %s\n",
		   k_uptime_get_32(),
		   bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
		   info->rssi, cte_type2str(info->cte_type), buf->len, data_str);
	*/
}

static int num_cte_err = 0;
static int num_cte_ok = 0;
static void cte_recv_cb(struct bt_le_per_adv_sync *sync,
			struct bt_df_per_adv_sync_iq_samples_report const *report)
{
	if (report->sample_count == 0 || report->packet_status == BT_DF_CTE_INSUFFICIENT_RESOURCES) {
        LOG_ERR("No mem for CTE report");
        num_cte_err++;
    } else {
		num_cte_ok++;
	}
	if (report->packet_status != 0) {
		return;
	}
#ifdef USE_PER_ADV_LIST
	struct bt_le_per_adv_sync_info info;
	int err = bt_le_per_adv_sync_get_info(sync, &info);
	__ASSERT(err == 0, "Must find the sync here, otherwise something wrong");
	uint8_t device_index = findTagIndexInSyncedListByAddr(&info.addr, NULL);
	if (device_index == LIST_INDEX_NONE) {
		LOG_ERR("Got cte_recv_cb from none queued tag!");
		restart_per_adv_list_syncing();
		return;
	}
#else
	uint8_t device_index = bt_le_per_adv_sync_get_index(sync);
#endif

	__ASSERT(sync_devices[device_index].is_synced && sync_devices[device_index].is_used, "CTE from tag which is not synced!");

	sync_devices[device_index].num_cte++;
}
static int num_legacy = 0;
static int num_ext = 0;

static void scan_recv(const struct bt_le_scan_recv_info *info,
			  struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];
	struct bt_le_per_adv_sync *sync;
	int device_index;
	(void)memset(name, 0, sizeof(name));
	static int no_spam_counter = 0;

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	if (info->interval) {
		num_ext++;
	} else {
		num_legacy++;
	}

	if (no_spam_counter % 100 == 0) {
		printk("Scan is running...\n");
	}
	no_spam_counter++;

	if (info->interval) {
		LOG_DBG("Interval: %d", info->interval);
	}
	return; // Skip syncing, seems reproducable anyway

	if (info->interval && !is_syncing) {
		// Do not try to sync with device we already synced with.
		// Could probably also use bt_le_per_adv_sync_lookup_addr(...) or similar
#ifdef USE_PER_ADV_LIST
		// Check if already handled
		if (findTagIndexInSyncedListByAddr(info->addr, &device_index) != LIST_INDEX_NONE) {
			return;
		}
		int err = bt_le_per_adv_list_add(info->addr, info->sid);
		if (err) {
			LOG_ERR("Failed adding to bt_le_per_adv_list_add err= %d", err);
			return;
		} else {
			LOG_DBG("Added tag from per adv list");
			sync_devices[device_index].is_used = true;
			sync_devices[device_index].is_synced = false;
			sync_devices[device_index].per_sid = info->sid;
			bt_addr_le_copy(&sync_devices[device_index].per_addr, info->addr);
		}
#else
		for (int i = 0; i < SYNCED_DEVICE_QUEUE_SIZE; i++) {
			if (bt_addr_le_cmp(info->addr, &sync_devices[i].per_addr) == 0) {
				return;
			}
		}
		DF_LOG("(%d) Found device sending per adv: %s\n", k_uptime_get_32(), le_addr);
		int err = create_sync(info->addr, info->sid, &sync);
		if (err != 0) {
			return;
		}
		is_syncing = true;
		device_index = bt_le_per_adv_sync_get_index(sync);
		DF_LOG("Sync index: %d\n", device_index);
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
#endif
	}
}

static int create_sync(const bt_addr_le_t *per_addr, uint8_t per_sid, struct bt_le_per_adv_sync **sync)
{
	int err;
	struct bt_le_per_adv_sync_param sync_create_param;

	DF_LOG("(%d) Creating Periodic Advertising Sync...\n", k_uptime_get_32());
	bt_addr_le_copy(&sync_create_param.addr, per_addr);
	sync_create_param.options = BT_LE_PER_ADV_SYNC_OPT_SYNC_ONLY_CONST_TONE_EXT;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 300; //0xa; // Increased timeout as otherwise seems to lose sync very often
	err = bt_le_per_adv_sync_create(&sync_create_param, sync);
	if (err == -EBUSY) {
		printk("Sync for some device already in progress.\n");
	} else if (err == -ENOMEM) {
		printk("No free adv sync availible\n");
	}

	if (err != 0) {
		return err;
	}

	DF_LOG("success.\n");
	return 0;
}

static int enable_cte_rx(device_t *device)
{
	int err;

	const struct bt_df_per_adv_sync_cte_rx_param cte_rx_params = {
		.max_cte_count = 1,
		.cte_types = BT_DF_CTE_TYPE_ALL,
		.slot_durations = 0x1,
		.num_ant_ids = ARRAY_SIZE(ant_patterns),
		.ant_ids = ant_patterns,
	};

	DF_LOG("Enable receiving of CTE...\n");
	err = bt_df_per_adv_sync_cte_rx_enable(device->sync, &cte_rx_params);
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return err;
	}
	DF_LOG("(%d) success. CTE receive enabled.\n", k_uptime_get_32());
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
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x10,
		.window     = 0x10,
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
	scan_enabled = false;
	err = bt_le_scan_stop();
	if (err != 0) {
		printk("failed (err %d)\n", err);
		return;
	}
	printk("Success.\n");

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
		DF_LOG("(%d) sync_create_timeout for %s\n", k_uptime_get_32(), le_addr);
		err = bt_le_per_adv_sync_delete(device->sync);
		if (err) {
			DF_LOG("sync_create_timeout: Failed cancel sync: %d\n", err);
		} else {
			DF_LOG("Sync deleted\n");
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

#define DEBUG_PRINT_LEN 500
static char debugBuf[DEBUG_PRINT_LEN];
typedef enum {
    TAG_IN_QUEUED,
    TAG_IN_PER_ADV_LIST,
    TAG_SYNCED
} tagState_t;

static int state_from_device(device_t *tag)
{
    if (!tag->is_synced) {
        return TAG_IN_PER_ADV_LIST;
    } else if (tag->is_synced) {
        return TAG_SYNCED;
    } else {
        return TAG_IN_QUEUED;
    }
}

static void print_stats_for_plot(void)
{
    int numQueued = 0;
	int numSynced = 0;
    memset(debugBuf, 0, sizeof(debugBuf));
    int index = 0;
    index += snprintf(debugBuf + index, sizeof(debugBuf) - index, "+UUDFDBG:");
    for (int i = 0; i < SYNCED_DEVICE_QUEUE_SIZE; i++) {
        if (sync_devices[i].is_used) {
			if (sync_devices[i].is_synced) {
				numSynced++;
			}
			numQueued++;
			// Use last 2 bytes from addr as the identifier.
			index += snprintf(debugBuf + index, sizeof(debugBuf) - index, "%d.%d,",
							*((uint16_t *)&sync_devices[i].per_addr.a.val[4]), state_from_device(&sync_devices[i]));
		}
    }
    index += snprintf(debugBuf + index, sizeof(debugBuf) - index, "\r\n");
    // Output an event so that we can plot the state of all tags.
#ifdef USE_PER_ADV_LIST
    printk("%sQueued: %d, Synced: %d\n", debugBuf, numQueued, numSynced);
#else
    printk("%sSynced: %d, Synced: %d\n", debugBuf, numQueued, numSynced);
#endif
}

// This data is input to python scripts that collects and plots per.adv.sync and CTE statistics.
static void print_stats_readable(void)
{
	int numQueued = 0;
	int numSynced = 0;
    memset(debugBuf, 0, sizeof(debugBuf));
    int index = 0;
    index += snprintf(debugBuf + index, sizeof(debugBuf) - index, "TAGS:");
	for (int i = 0; i < SYNCED_DEVICE_QUEUE_SIZE; i++) {
		if (sync_devices[i].is_synced) {
			index += snprintf(debugBuf + index, sizeof(debugBuf) - index, "%d,", sync_devices[i].num_cte);
			numSynced++;
		} else if (sync_devices[i].is_used) {
			index += snprintf(debugBuf + index, sizeof(debugBuf) - index, "sync...,");
			numQueued++;
		} else {
			index += snprintf(debugBuf + index, sizeof(debugBuf) - index, "-,");
		}
		sync_devices[i].num_cte = 0;
	}
	printk("%s\nQueued: %d Synced: %d Num err: %d/%d legacy: %d, ext: %d\n", debugBuf, numQueued, numSynced, num_cte_err, num_cte_ok, num_legacy, num_ext);
	num_cte_err = 0;
	num_cte_ok = 0;
	num_legacy = 0;
	num_ext = 0;
}

static void cancel_all_synced(void)
{
	int ret;
	char le_addr[BT_ADDR_LE_STR_LEN];

	for (int i = 0; i < SYNCED_DEVICE_QUEUE_SIZE; i++) {
        if (sync_devices[i].is_used) {
			if (sync_devices[i].is_synced) {
				ret = bt_le_per_adv_sync_delete(sync_devices[i].sync);
				if (ret != 0) {
					bt_addr_le_to_str(&sync_devices[i].per_addr, le_addr, sizeof(le_addr));
					LOG_WRN("Failed delete sync for %s", le_addr);
				}
				memset(&sync_devices[i], 0, sizeof(device_t));
			}
		}
    }
}

#ifdef NUM_SYNC_TO_RESTART_SCAN
static void check_restart_scanning_syncing(void)
{
	int ret;
	int numSynced = 0;
    for (int i = 0; i < SYNCED_DEVICE_QUEUE_SIZE; i++) {
        if (sync_devices[i].is_used) {
			if (sync_devices[i].is_synced) {
				numSynced++;
			}
		}
    }
	if (numSynced >= NUM_SYNC_TO_RESTART_SCAN) {
		LOG_WRN("RESTARTING SCAN AND SYNC...");

#ifdef USE_PER_ADV_LIST
		if (syncForList) {
			ret = bt_le_per_adv_sync_delete(syncForList);
			__ASSERT_NO_MSG(!ret);
		}
#endif
		scan_disable();
		cancel_all_synced();
		memset(sync_devices, 0, sizeof(sync_devices));
		ret = bt_le_per_adv_list_clear();
		__ASSERT_NO_MSG(ret == 0);
		k_msleep(200);
		scan_enable();
#ifdef USE_PER_ADV_LIST
		restart_per_adv_list_syncing();
#endif

		LOG_WRN("RESTARTED!");
	}
}
#endif
/*
* Simulate CPU doing other stuff than just periodic adv. scanning.
*/
static void burn_cpu(void *unused1, void *unused2, void *unused3)
{
	LOG_WRN("CPU burner started!");
	uint32_t micros_to_wait = 500;
	while (1) {
		uint32_t start_cycles = k_cycle_get_32();

		/* use 64-bit math to prevent overflow when multiplying */
		uint32_t cycles_to_wait = (uint32_t)(
			(uint64_t)micros_to_wait *
			(uint64_t)sys_clock_hw_cycles_per_sec() /
			(uint64_t)USEC_PER_SEC
		);

		for (;;) {
			uint32_t current_cycles = k_cycle_get_32();

			/* this handles the rollover on an unsigned 32-bit value */
			if ((current_cycles - start_cycles) >= cycles_to_wait) {
				break;
			}
		}
		k_msleep(2);
	}
}


void main(void)
{
	int err;
	printk("Starting Connectionless Locator Demo\n");

	memset(sync_devices, 0, sizeof(sync_devices));

	for (int i = 0; i < SYNCED_DEVICE_QUEUE_SIZE; i++) {
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

#ifdef USE_PER_ADV_LIST
	restart_per_adv_list_syncing();
#endif
	printk("Waiting for periodic advertising...\n");
#ifdef BURN_CPU
	k_thread_create(&threadData, threadStack,
		K_THREAD_STACK_SIZEOF(threadStack),
		burn_cpu,
		NULL, NULL, NULL,
		7, 0, K_NO_WAIT);
#endif
	while (true) {
		k_msleep(1000);
		printk("Running...\n");
#ifdef PRINT_STATS_FOR_PLOT
		print_stats_for_plot();
#endif
#ifdef PRINT_STATS_FOR_READING
		print_stats_readable();
#endif

#ifdef NUM_SYNC_TO_RESTART_SCAN
		check_restart_scanning_syncing();
#endif
	}
}
