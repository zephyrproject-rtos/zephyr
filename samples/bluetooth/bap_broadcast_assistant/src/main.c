/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <strings.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/sys/byteorder.h>

#define NAME_LEN 30
#define PA_SYNC_SKIP         5
#define PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO 20 /* Set the timeout relative to interval */
/* Broadcast IDs are 24bit, so this is out of valid range */
#define INVALID_BROADCAST_ID 0xFFFFFFFFU

static void scan_for_broadcast_sink(void);

/* Struct to collect information from scanning
 * for Broadcast Source or Sink
 */
struct scan_recv_info {
	char bt_name[NAME_LEN];
	char broadcast_name[NAME_LEN];
	uint32_t broadcast_id;
	bool has_bass;
	bool has_pacs;
};

static struct bt_conn *broadcast_sink_conn;
static uint32_t selected_broadcast_id;
static uint8_t selected_sid;
static uint16_t selected_pa_interval;
static bt_addr_le_t selected_addr;
static struct bt_le_per_adv_sync *pa_sync;
static uint8_t received_base[UINT8_MAX];
static size_t received_base_size;
static struct bt_bap_bass_subgroup
	bass_subgroups[CONFIG_BT_BAP_BASS_MAX_SUBGROUPS];

static bool scanning_for_broadcast_source;

static struct k_mutex base_store_mutex;
static K_SEM_DEFINE(sem_source_discovered, 0, 1);
static K_SEM_DEFINE(sem_sink_discovered, 0, 1);
static K_SEM_DEFINE(sem_sink_connected, 0, 1);
static K_SEM_DEFINE(sem_sink_disconnected, 0, 1);
static K_SEM_DEFINE(sem_security_updated, 0, 1);
static K_SEM_DEFINE(sem_bass_discovered, 0, 1);
static K_SEM_DEFINE(sem_pa_synced, 0, 1);
static K_SEM_DEFINE(sem_received_base_subgroups, 0, 1);

static bool device_found(struct bt_data *data, void *user_data)
{
	struct scan_recv_info *sr_info = (struct scan_recv_info *)user_data;
	struct bt_uuid_16 adv_uuid;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		memcpy(sr_info->bt_name, data->data, MIN(data->data_len, NAME_LEN - 1));
		return true;
	case BT_DATA_BROADCAST_NAME:
		memcpy(sr_info->broadcast_name, data->data, MIN(data->data_len, NAME_LEN - 1));
		return true;
	case BT_DATA_SVC_DATA16:
		/* Check for Broadcast ID */
		if (data->data_len < BT_UUID_SIZE_16 + BT_AUDIO_BROADCAST_ID_SIZE) {
			return true;
		}

		if (!bt_uuid_create(&adv_uuid.uuid, data->data, BT_UUID_SIZE_16)) {
			return true;
		}

		if (bt_uuid_cmp(&adv_uuid.uuid, BT_UUID_BROADCAST_AUDIO) != 0) {
			return true;
		}

		sr_info->broadcast_id = sys_get_le24(data->data + BT_UUID_SIZE_16);
		return true;
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		/* NOTE: According to the BAP 1.0.1 Spec,
		 * Section 3.9.2. Additional Broadcast Audio Scan Service requirements,
		 * If the Scan Delegator implements a Broadcast Sink, it should also
		 * advertise a Service Data field containing the Broadcast Audio
		 * Scan Service (BASS) UUID.
		 *
		 * However, it seems that this is not the case with the sinks available
		 * while developing this sample application.  Therefore, we instead,
		 * search for the existence of BASS and PACS in the list of service UUIDs,
		 * which does seem to exist in the sinks available.
		 */

		/* Check for BASS and PACS */
		if (data->data_len % sizeof(uint16_t) != 0U) {
			printk("UUID16 AD malformed\n");
			return true;
		}

		for (size_t i = 0; i < data->data_len; i += sizeof(uint16_t)) {
			const struct bt_uuid *uuid;
			uint16_t u16;

			memcpy(&u16, &data->data[i], sizeof(u16));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));

			if (bt_uuid_cmp(uuid, BT_UUID_BASS) == 0) {
				sr_info->has_bass = true;
				continue;
			}

			if (bt_uuid_cmp(uuid, BT_UUID_PACS) == 0) {
				sr_info->has_pacs = true;
				continue;
			}
		}
		return true;
	default:
		return true;
	}
}

static bool base_store(struct bt_data *data, void *user_data)
{
	const struct bt_bap_base *base = bt_bap_base_get_base_from_ad(data);
	int base_size;
	int base_subgroup_count;

	/* Base is NULL if the data does not contain a valid BASE */
	if (base == NULL) {
		return true;
	}

	/* Can not fit all the received subgroups with the size CONFIG_BT_BAP_BASS_MAX_SUBGROUPS */
	base_subgroup_count = bt_bap_base_get_subgroup_count(base);
	if (base_subgroup_count < 0 || base_subgroup_count > CONFIG_BT_BAP_BASS_MAX_SUBGROUPS) {
		printk("Got invalid subgroup count: %d\n", base_subgroup_count);
		return true;
	}

	base_size = bt_bap_base_get_size(base);
	if (base_size < 0) {
		printk("BASE get size failed (%d)\n", base_size);

		return true;
	}

	/* Compare BASE and copy if different */
	k_mutex_lock(&base_store_mutex, K_FOREVER);
	if ((size_t)base_size != received_base_size ||
	    memcmp(base, received_base, (size_t)base_size) != 0) {
		(void)memcpy(received_base, base, base_size);
		received_base_size = (size_t)base_size;
	}
	k_mutex_unlock(&base_store_mutex);

	/* Stop parsing */
	k_sem_give(&sem_received_base_subgroups);
	return false;
}

static void pa_recv(struct bt_le_per_adv_sync *sync,
			 const struct bt_le_per_adv_sync_recv_info *info,
			 struct net_buf_simple *buf)
{
	bt_data_parse(buf, base_store, NULL);
}

static bool add_pa_sync_base_subgroup_bis_cb(const struct bt_bap_base_subgroup_bis *bis,
					     void *user_data)
{
	struct bt_bap_bass_subgroup *subgroup_param = user_data;

	subgroup_param->bis_sync |= BT_ISO_BIS_INDEX_BIT(bis->index);

	return true;
}

static bool add_pa_sync_base_subgroup_cb(const struct bt_bap_base_subgroup *subgroup,
					 void *user_data)
{
	struct bt_bap_broadcast_assistant_add_src_param *param = user_data;
	struct bt_bap_bass_subgroup *subgroup_param;
	uint8_t *data;
	int ret;

	ret = bt_bap_base_get_subgroup_codec_meta(subgroup, &data);
	if (ret < 0) {
		return false;
	}

	subgroup_param = param->subgroups;

	if (ret > ARRAY_SIZE(subgroup_param->metadata)) {
		printk("Cannot fit %d octets into subgroup param with size %zu", ret,
		       ARRAY_SIZE(subgroup_param->metadata));
		return false;
	}

	ret = bt_bap_base_subgroup_foreach_bis(subgroup, add_pa_sync_base_subgroup_bis_cb,
					       subgroup_param);
	if (ret < 0) {
		return false;
	}

	param->num_subgroups++;

	return true;
}

static bool is_substring(const char *substr, const char *str)
{
	const size_t str_len = strlen(str);
	const size_t sub_str_len = strlen(substr);

	if (sub_str_len > str_len) {
		return false;
	}

	for (size_t pos = 0; pos < str_len; pos++) {
		if (pos + sub_str_len > str_len) {
			return false;
		}

		if (strncasecmp(substr, &str[pos], sub_str_len) == 0) {
			return true;
		}
	}

	return false;
}

static uint16_t interval_to_sync_timeout(uint16_t pa_interval)
{
	uint16_t pa_timeout;

	if (pa_interval == BT_BAP_PA_INTERVAL_UNKNOWN) {
		/* Use maximum value to maximize chance of success */
		pa_timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;
	} else {
		uint32_t interval_ms;
		uint32_t timeout;

		/* Add retries and convert to unit in 10's of ms */
		interval_ms = BT_GAP_PER_ADV_INTERVAL_TO_MS(pa_interval);
		timeout = (interval_ms * PA_SYNC_INTERVAL_TO_TIMEOUT_RATIO) / 10;

		/* Enforce restraints */
		pa_timeout = CLAMP(timeout, BT_GAP_PER_ADV_MIN_TIMEOUT, BT_GAP_PER_ADV_MAX_TIMEOUT);
	}

	return pa_timeout;
}

static int pa_sync_create(void)
{
	struct bt_le_per_adv_sync_param create_params = {0};

	bt_addr_le_copy(&create_params.addr, &selected_addr);
	create_params.options = BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	create_params.sid = selected_sid;
	create_params.skip = PA_SYNC_SKIP;
	create_params.timeout = interval_to_sync_timeout(selected_pa_interval);

	return bt_le_per_adv_sync_create(&create_params, &pa_sync);
}

static void scan_recv_cb(const struct bt_le_scan_recv_info *info,
			 struct net_buf_simple *ad)
{
	int err;
	struct scan_recv_info sr_info = {0};

	if (scanning_for_broadcast_source) {
		/* Scan for and select Broadcast Source */

		sr_info.broadcast_id = INVALID_BROADCAST_ID;

		/* We are only interested in non-connectable periodic advertisers */
		if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0 ||
		    info->interval == 0) {
			return;
		}

		bt_data_parse(ad, device_found, (void *)&sr_info);

		if (sr_info.broadcast_id != INVALID_BROADCAST_ID) {
			printk("Broadcast Source Found:\n");
			printk("  BT Name:        %s\n", sr_info.bt_name);
			printk("  Broadcast Name: %s\n", sr_info.broadcast_name);
			printk("  Broadcast ID:   0x%06x\n\n", sr_info.broadcast_id);

#if defined(CONFIG_SELECT_SOURCE_NAME)
			if (strlen(CONFIG_SELECT_SOURCE_NAME) > 0U) {
				/* Compare names with CONFIG_SELECT_SOURCE_NAME */
				if (is_substring(CONFIG_SELECT_SOURCE_NAME, sr_info.bt_name) ||
				    is_substring(CONFIG_SELECT_SOURCE_NAME,
						 sr_info.broadcast_name)) {
					printk("Match found for '%s'\n", CONFIG_SELECT_SOURCE_NAME);
				} else {
					printk("'%s' not found in names\n\n",
					       CONFIG_SELECT_SOURCE_NAME);
					return;
				}
			}
#endif /* CONFIG_SELECT_SOURCE_NAME */

			err = bt_le_scan_stop();
			if (err != 0) {
				printk("bt_le_scan_stop failed with %d\n", err);
			}

			/* TODO: Add support for syncing to the PA and parsing the BASE
			 * in order to obtain the right subgroup information to send to
			 * the sink when adding a broadcast source (see in main function below).
			 */

			printk("Selecting Broadcast ID: 0x%06x\n", sr_info.broadcast_id);

			selected_broadcast_id = sr_info.broadcast_id;
			selected_sid = info->sid;
			selected_pa_interval = info->interval;
			bt_addr_le_copy(&selected_addr, info->addr);

			k_sem_give(&sem_source_discovered);

			printk("Attempting to PA sync to the broadcaster with id 0x%06X\n",
			       selected_broadcast_id);
		}
	} else {
		/* Scan for and connect to Broadcast Sink */

		/* We are only interested in connectable advertisers */
		if ((info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) == 0) {
			return;
		}

		bt_data_parse(ad, device_found, (void *)&sr_info);

		if (sr_info.has_bass && sr_info.has_pacs) {
			printk("Broadcast Sink Found:\n");
			printk("  BT Name:        %s\n", sr_info.bt_name);

			if (strlen(CONFIG_SELECT_SINK_NAME) > 0U) {
				/* Compare names with CONFIG_SELECT_SINK_NAME */
				if (is_substring(CONFIG_SELECT_SINK_NAME, sr_info.bt_name)) {
					printk("Match found for '%s'\n", CONFIG_SELECT_SINK_NAME);
				} else {
					printk("'%s' not found in names\n\n",
					       CONFIG_SELECT_SINK_NAME);
					return;
				}
			}

			err = bt_le_scan_stop();
			if (err != 0) {
				printk("bt_le_scan_stop failed with %d\n", err);
			}

			printk("Connecting to Broadcast Sink: %s\n", sr_info.bt_name);

			err = bt_conn_le_create(info->addr, BT_CONN_LE_CREATE_CONN,
						BT_LE_CONN_PARAM_DEFAULT,
						&broadcast_sink_conn);
			if (err != 0) {
				printk("Failed creating connection (err=%u)\n", err);
				scan_for_broadcast_sink();
			}

			k_sem_give(&sem_sink_discovered);
		}
	}
}

static void scan_timeout_cb(void)
{
	printk("Scan timeout\n");
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv_cb,
	.timeout = scan_timeout_cb,
};

static void scan_for_broadcast_source(void)
{
	int err;

	scanning_for_broadcast_source = true;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning for Broadcast Source successfully started\n");

	err = k_sem_take(&sem_source_discovered, K_FOREVER);
	if (err != 0) {
		printk("Failed to take sem_source_discovered (err %d)\n", err);
	}
}

static void scan_for_broadcast_sink(void)
{
	int err;

	scanning_for_broadcast_source = false;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning for Broadcast Sink successfully started\n");

	err = k_sem_take(&sem_sink_discovered, K_FOREVER);
	if (err != 0) {
		printk("Failed to take sem_sink_discovered (err %d)\n", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		printk("Failed to connect to %s (%u)\n", addr, err);

		bt_conn_unref(broadcast_sink_conn);
		broadcast_sink_conn = NULL;

		scan_for_broadcast_sink();
		return;
	}

	if (conn != broadcast_sink_conn) {
		return;
	}

	printk("Connected: %s\n", addr);
	k_sem_give(&sem_sink_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != broadcast_sink_conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	bt_conn_unref(broadcast_sink_conn);
	broadcast_sink_conn = NULL;

	k_sem_give(&sem_sink_disconnected);
}

static void security_changed_cb(struct bt_conn *conn, bt_security_t level,
				enum bt_security_err err)
{
	if (err == 0) {
		printk("Security level changed: %u\n", level);
		k_sem_give(&sem_security_updated);
	} else {
		printk("Failed to set security level: %u\n", err);
	}
}

static void bap_broadcast_assistant_discover_cb(struct bt_conn *conn, int err,
						uint8_t recv_state_count)
{
	if (err == 0) {
		printk("BASS discover done with %u recv states\n",
		       recv_state_count);
		k_sem_give(&sem_bass_discovered);
	} else {
		printk("BASS discover failed (%d)\n", err);
	}
}

static void bap_broadcast_assistant_add_src_cb(struct bt_conn *conn, int err)
{
	if (err == 0) {
		printk("BASS add source successful\n");
	} else {
		printk("BASS add source failed (%d)\n", err);
	}
}

static void pa_sync_synced_cb(struct bt_le_per_adv_sync *sync,
			      struct bt_le_per_adv_sync_synced_info *info)
{
	if (sync == pa_sync) {
		printk("PA sync %p synced for broadcast sink with broadcast ID 0x%06X\n", sync,
		       selected_broadcast_id);

		k_sem_give(&sem_pa_synced);
	}
}

static struct bt_bap_broadcast_assistant_cb ba_cbs = {
	.discover = bap_broadcast_assistant_discover_cb,
	.add_src = bap_broadcast_assistant_add_src_cb,
};

static struct bt_le_per_adv_sync_cb pa_synced_cb = {
	.synced = pa_sync_synced_cb,
	.recv = pa_recv,
};

static void reset(void)
{
	printk("\n\nReset...\n\n");

	broadcast_sink_conn = NULL;
	selected_broadcast_id = INVALID_BROADCAST_ID;
	selected_sid = 0;
	selected_pa_interval = 0;
	(void)memset(&selected_addr, 0, sizeof(selected_addr));

	k_sem_reset(&sem_source_discovered);
	k_sem_reset(&sem_sink_discovered);
	k_sem_reset(&sem_sink_connected);
	k_sem_reset(&sem_sink_disconnected);
	k_sem_reset(&sem_security_updated);
	k_sem_reset(&sem_bass_discovered);
	k_sem_reset(&sem_pa_synced);
	k_sem_reset(&sem_received_base_subgroups);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed_cb
};

int main(void)
{
	int err;
	struct bt_bap_broadcast_assistant_add_src_param param = { 0 };

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	bt_bap_broadcast_assistant_register_cb(&ba_cbs);
	bt_le_per_adv_sync_cb_register(&pa_synced_cb);
	bt_le_scan_cb_register(&scan_callbacks);

	k_mutex_init(&base_store_mutex);

	while (true) {
		scan_for_broadcast_sink();

		err = k_sem_take(&sem_sink_connected, K_FOREVER);
		if (err != 0) {
			printk("Failed to take sem_sink_connected (err %d)\n", err);
		}

		err = bt_bap_broadcast_assistant_discover(broadcast_sink_conn);
		if (err != 0) {
			printk("Failed to discover BASS on the sink (err %d)\n", err);
		}

		err = k_sem_take(&sem_security_updated, K_SECONDS(10));
		if (err != 0) {
			printk("Failed to take sem_security_updated (err %d), resetting\n", err);
			bt_conn_disconnect(broadcast_sink_conn, BT_HCI_ERR_AUTH_FAIL);

			if (k_sem_take(&sem_sink_disconnected, K_SECONDS(10)) != 0) {
				/* This should not happen */
				return -ETIMEDOUT;
			}

			reset();
			continue;
		}

		err = k_sem_take(&sem_bass_discovered, K_SECONDS(10));
		if (err != 0) {
			if (err == -EAGAIN) {
				printk("Failed to take sem_bass_discovered (err %d)\n", err);
			}
			bt_conn_disconnect(broadcast_sink_conn, BT_HCI_ERR_UNSUPP_REMOTE_FEATURE);

			if (k_sem_take(&sem_sink_disconnected, K_SECONDS(10)) != 0) {
				/* This should not happen */
				return -ETIMEDOUT;
			}

			reset();
			continue;
		}

		/* TODO: Discover and parse the PACS on the sink and use the information
		 * when discovering and adding a source to the sink.
		 * Also, before populating the parameters to sync to the broadcast source
		 * first, parse the source BASE and determine if the sink supports the source.
		 * If not, then look for another source.
		 */

		scan_for_broadcast_source();

		printk("Scan stopped, attempting to PA sync to the broadcaster with id 0x%06X\n",
		       selected_broadcast_id);
		err = pa_sync_create();
		if (err != 0) {
			printk("Could not create Broadcast PA sync: %d, resetting\n", err);
			return -ETIMEDOUT;
		}

		printk("Waiting for PA synced\n");
		err = k_sem_take(&sem_pa_synced, K_FOREVER);
		if (err != 0) {
			printk("sem_pa_synced timed out, resetting\n");
			return err;
		}

		memset(bass_subgroups, 0, sizeof(bass_subgroups));
		bt_addr_le_copy(&param.addr, &selected_addr);
		param.adv_sid = selected_sid;
		param.pa_interval = selected_pa_interval;
		param.broadcast_id = selected_broadcast_id;
		param.pa_sync = true;
		param.subgroups = bass_subgroups;

		/* Wait to receive subgroups */
		err = k_sem_take(&sem_received_base_subgroups, K_FOREVER);
		if (err != 0) {
			printk("Failed to take sem_received_base_subgroups (err %d)\n", err);
			return err;
		}

		k_mutex_lock(&base_store_mutex, K_FOREVER);
		err = bt_bap_base_foreach_subgroup((const struct bt_bap_base *)received_base,
						   add_pa_sync_base_subgroup_cb, &param);
		k_mutex_unlock(&base_store_mutex);

		if (err < 0) {
			printk("Could not add BASE to params %d\n", err);
			continue;
		}

		err = bt_bap_broadcast_assistant_add_src(broadcast_sink_conn, &param);
		if (err) {
			printk("Failed to add source: %d\n", err);
			bt_conn_disconnect(broadcast_sink_conn, BT_HCI_ERR_UNSUPP_REMOTE_FEATURE);

			if (k_sem_take(&sem_sink_disconnected, K_SECONDS(10)) != 0) {
				/* This should not happen */
				return -ETIMEDOUT;
			}

			reset();
			continue;
		}

		/* Reset if the sink disconnects */
		err = k_sem_take(&sem_sink_disconnected, K_FOREVER);
		if (err != 0) {
			printk("Failed to take sem_sink_disconnected (err %d)\n", err);
		}

		reset();
	}

	return 0;
}
