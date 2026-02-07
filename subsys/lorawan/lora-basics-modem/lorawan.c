/*
 * Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <errno.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/sys/slist.h>

#include <smtc_modem_hal.h>
#include <smtc_modem_hal_ext.h>
#include <smtc_modem_utilities.h>
#include <smtc_modem_api.h>
#include <lorawan_api.h>

#include "lw_priv.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan, CONFIG_LORAWAN_LOG_LEVEL);

#define LBM_THREAD_MAX_SLEEP_MS 60000
#define LBM_MODEM_RESET_TIMEOUT K_SECONDS(10)
#define SMTC_MODEM_REGION_MAX   14
#define REGION_NOT_SET          ((smtc_modem_region_t)0)
#define LBM_TIMEOUT             K_SECONDS(120)
#define STACK_ID                0

#define SMTC_MODEM_EVENT_(name) SMTC_MODEM_EVENT_##name
#define LOG_UNHANDLED_EVENT(name)                                                                  \
	case SMTC_MODEM_EVENT_(name):                                                              \
		LOG_INF("Event: [unhandled] " #name);                                              \
		break

static const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));

K_MSGQ_DEFINE(tx_msgq, sizeof(smtc_modem_event_txdone_status_t), 1, 4);
K_MSGQ_DEFINE(join_msgq, sizeof(bool), 1, 4);

static K_SEM_DEFINE(modem_reset_sem, 0, 1);

static lorawan_dr_changed_cb_t dr_changed_cb;
static uint8_t current_dr = UINT8_MAX;
static sys_slist_t dl_callbacks;

static struct {
	bool enabled[SMTC_MODEM_REGION_MAX];
	smtc_modem_region_t active;
} regions = {
	.enabled = {
			[SMTC_MODEM_REGION_AS_923_GRP1] = IS_ENABLED(CONFIG_LORAWAN_REGION_AS923),
			[SMTC_MODEM_REGION_AU_915] = IS_ENABLED(CONFIG_LORAWAN_REGION_AU915),
			[SMTC_MODEM_REGION_CN_470] = IS_ENABLED(CONFIG_LORAWAN_REGION_CN470),
			[SMTC_MODEM_REGION_EU_868] = IS_ENABLED(CONFIG_LORAWAN_REGION_EU868),
			[SMTC_MODEM_REGION_IN_865] = IS_ENABLED(CONFIG_LORAWAN_REGION_IN865),
			[SMTC_MODEM_REGION_KR_920] = IS_ENABLED(CONFIG_LORAWAN_REGION_KR920),
			[SMTC_MODEM_REGION_RU_864] = IS_ENABLED(CONFIG_LORAWAN_REGION_RU864),
			[SMTC_MODEM_REGION_US_915] = IS_ENABLED(CONFIG_LORAWAN_REGION_US915),
	},
};

static const char *smtc_modem_rc_to_str(smtc_modem_return_code_t rc)
{
	static const char *const rc_str[] = {
		[SMTC_MODEM_RC_OK] = "OK",
		[SMTC_MODEM_RC_NOT_INIT] = "NOT_INIT",
		[SMTC_MODEM_RC_INVALID] = "INVALID",
		[SMTC_MODEM_RC_BUSY] = "BUSY",
		[SMTC_MODEM_RC_FAIL] = "FAIL",
		[SMTC_MODEM_RC_NO_TIME] = "NO_TIME",
		[SMTC_MODEM_RC_INVALID_STACK_ID] = "INVALID_STACK_ID",
		[SMTC_MODEM_RC_NO_EVENT] = "NO_EVENT",
	};

	if (rc < ARRAY_SIZE(rc_str) && rc_str[rc] != NULL) {
		return rc_str[rc];
	}

	return "UNKNOWN";
}

static const char *lbm_region_to_str(smtc_modem_region_t region)
{
	static const char *const region_str[] = {
		[SMTC_MODEM_REGION_AS_923_GRP1] = "AS923",
		[SMTC_MODEM_REGION_US_915] = "US915",
		[SMTC_MODEM_REGION_CN_470] = "CN470",
		[SMTC_MODEM_REGION_EU_868] = "EU868",
		[SMTC_MODEM_REGION_IN_865] = "IN865",
		[SMTC_MODEM_REGION_KR_920] = "KR920",
		[SMTC_MODEM_REGION_RU_864] = "RU864",
		[SMTC_MODEM_REGION_AU_915] = "AU915",
	};

	if (region < ARRAY_SIZE(region_str) && region_str[region] != NULL) {
		return region_str[region];
	}

	return "UNKNOWN";
}

static void handle_reset(void)
{
	LOG_INF("Event: RESET");
	k_sem_give(&modem_reset_sem);
}

static void handle_joined(void)
{
	uint8_t dr;
	bool success = true;

	dr = lorawan_api_next_dr_get(STACK_ID);
	current_dr = dr;

	LOG_INF("Event: JOINED (DR%u)", dr);

	if (dr_changed_cb != NULL) {
		dr_changed_cb((enum lorawan_datarate)dr);
	}

	k_msgq_put(&join_msgq, &success, K_NO_WAIT);
}

static void handle_joinfail(void)
{
	bool success = false;

	LOG_INF("Event: JOINFAIL");
	k_msgq_put(&join_msgq, &success, K_NO_WAIT);
}

static void handle_txdone(smtc_modem_event_txdone_status_t status)
{
	static const char *const txdone_status_str[] = {
		[SMTC_MODEM_EVENT_TXDONE_NOT_SENT] = "not sent",
		[SMTC_MODEM_EVENT_TXDONE_SENT] = "sent",
		[SMTC_MODEM_EVENT_TXDONE_CONFIRMED] = "confirmed",
	};

	uint32_t fcnt_up;
	uint8_t nb_trans;
	uint8_t dr;

	fcnt_up = lorawan_api_fcnt_up_get(STACK_ID);
	nb_trans = lorawan_api_nb_trans_get(STACK_ID);

	LOG_INF("Event: TXDONE (%s) fcnt=%u nb_trans=%u",
		(status < ARRAY_SIZE(txdone_status_str)) ? txdone_status_str[status] : "unknown",
		fcnt_up, nb_trans);

	if (IS_ENABLED(CONFIG_LORAWAN_LBM_ACK_DOWNLINK_CB) &&
	    status == SMTC_MODEM_EVENT_TXDONE_CONFIRMED) {
		struct lorawan_downlink_cb *cb;

		SYS_SLIST_FOR_EACH_CONTAINER(&dl_callbacks, cb, node) {
			if ((cb->port == LW_RECV_PORT_ANY) || (cb->port == 0)) {
				cb->cb(0, 0, 0, 0, 0, NULL);
			}
		}
	}

	dr = lorawan_api_next_dr_get(STACK_ID);
	if (dr != current_dr) {
		LOG_INF("Datarate changed: DR%u -> DR%u", current_dr, dr);
		current_dr = dr;
		if (dr_changed_cb != NULL) {
			dr_changed_cb((enum lorawan_datarate)dr);
		}
	}

	k_msgq_put(&tx_msgq, &status, K_NO_WAIT);
}

static void handle_downdata(void)
{
	uint8_t rx_buf[SMTC_MODEM_MAX_LORAWAN_PAYLOAD_LENGTH];
	smtc_modem_dl_metadata_t dl_metadata;
	struct lorawan_downlink_cb *cb;
	uint8_t remaining;
	uint8_t rx_len;
	uint8_t flags = 0;

	if (smtc_modem_get_downlink_data(rx_buf, &rx_len, &dl_metadata, &remaining) !=
	    SMTC_MODEM_RC_OK) {
		LOG_INF("Event: DOWNDATA (no data available)");
		return;
	}

	LOG_INF("Event: DOWNDATA port=%u len=%u window=%u rssi=%d snr=%d dr=%u freq=%u",
		dl_metadata.fport, rx_len, dl_metadata.window, dl_metadata.rssi, dl_metadata.snr,
		dl_metadata.datarate, dl_metadata.frequency_hz);
	LOG_HEXDUMP_DBG(rx_buf, rx_len, "Payload");

	if (dl_metadata.fpending_bit) {
		flags |= LORAWAN_DATA_PENDING;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&dl_callbacks, cb, node) {
		if ((cb->port == LW_RECV_PORT_ANY) || (cb->port == dl_metadata.fport)) {
			cb->cb(dl_metadata.fport, flags, dl_metadata.rssi, dl_metadata.snr, rx_len,
			       rx_buf);
		}
	}
}

static void lbm_event_cb(void)
{
	smtc_modem_event_t current_event;
	smtc_modem_return_code_t rc;
	uint8_t event_pending_count;

	do {
		rc = smtc_modem_get_event(&current_event, &event_pending_count);
		if (rc != SMTC_MODEM_RC_OK) {
			LOG_ERR("Failed to get event: %s", smtc_modem_rc_to_str(rc));
			return;
		}

		switch (current_event.event_type) {
		case SMTC_MODEM_EVENT_RESET:
			handle_reset();
			break;
		case SMTC_MODEM_EVENT_JOINED:
			handle_joined();
			break;
		case SMTC_MODEM_EVENT_TXDONE:
			handle_txdone(current_event.event_data.txdone.status);
			break;
		case SMTC_MODEM_EVENT_DOWNDATA:
			handle_downdata();
			break;
		case SMTC_MODEM_EVENT_JOINFAIL:
			handle_joinfail();
			break;

		LOG_UNHANDLED_EVENT(ALARM);
		LOG_UNHANDLED_EVENT(ALCSYNC_TIME);
		LOG_UNHANDLED_EVENT(CLASS_B_PING_SLOT_INFO);
		LOG_UNHANDLED_EVENT(CLASS_B_STATUS);
		LOG_UNHANDLED_EVENT(LINK_CHECK);
		LOG_UNHANDLED_EVENT(LORAWAN_MAC_TIME);
		LOG_UNHANDLED_EVENT(LORAWAN_FUOTA_DONE);
		LOG_UNHANDLED_EVENT(NO_MORE_MULTICAST_SESSION_CLASS_C);
		LOG_UNHANDLED_EVENT(NO_MORE_MULTICAST_SESSION_CLASS_B);
		LOG_UNHANDLED_EVENT(NEW_MULTICAST_SESSION_CLASS_C);
		LOG_UNHANDLED_EVENT(NEW_MULTICAST_SESSION_CLASS_B);
		LOG_UNHANDLED_EVENT(FIRMWARE_MANAGEMENT);
		LOG_UNHANDLED_EVENT(STREAM_DONE);
		LOG_UNHANDLED_EVENT(UPLOAD_DONE);
		LOG_UNHANDLED_EVENT(DM_SET_CONF);
		LOG_UNHANDLED_EVENT(MUTE);
		LOG_UNHANDLED_EVENT(GNSS_SCAN_DONE);
		LOG_UNHANDLED_EVENT(GNSS_TERMINATED);
		LOG_UNHANDLED_EVENT(GNSS_ALMANAC_DEMOD_UPDATE);
		LOG_UNHANDLED_EVENT(WIFI_SCAN_DONE);
		LOG_UNHANDLED_EVENT(WIFI_TERMINATED);
		LOG_UNHANDLED_EVENT(RELAY_TX_DYNAMIC);
		LOG_UNHANDLED_EVENT(RELAY_TX_MODE);
		LOG_UNHANDLED_EVENT(RELAY_TX_SYNC);
		LOG_UNHANDLED_EVENT(RELAY_RX_RUNNING);
		LOG_UNHANDLED_EVENT(TEST_MODE);
		LOG_UNHANDLED_EVENT(REGIONAL_DUTY_CYCLE);
		LOG_UNHANDLED_EVENT(NO_DOWNLINK_THRESHOLD);

		default:
			LOG_WRN("Unknown event: %u", current_event.event_type);
			break;
		}
	} while (event_pending_count > 0);
}

static void lbm_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t sleep_time_ms;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	smtc_modem_set_radio_context(lora_dev);
	smtc_modem_hal_init(lora_dev);
	smtc_modem_init(lbm_event_cb);

	while (true) {
		sleep_time_ms = smtc_modem_run_engine();

		if (smtc_modem_is_irq_flag_pending()) {
			continue;
		}

		sleep_time_ms = MIN(sleep_time_ms, LBM_THREAD_MAX_SLEEP_MS);
		smtc_modem_hal_interruptible_msleep(K_MSEC(sleep_time_ms));
	}
}
K_THREAD_DEFINE(lbm_thread, CONFIG_LORAWAN_LBM_THREAD_STACK_SIZE, lbm_thread_entry, NULL, NULL,
		NULL, CONFIG_LORAWAN_LBM_THREAD_PRIORITY, 0, K_TICKS_FOREVER);

static int lorawan_join_otaa(const struct lorawan_join_config *config)
{
	smtc_modem_return_code_t rc;

	LOG_DBG("Configuring OTAA credentials");

	if (config->dev_eui != NULL) {
		LOG_HEXDUMP_DBG(config->dev_eui, 8, "DevEUI");
		rc = smtc_modem_set_deveui(STACK_ID, config->dev_eui);
		if (rc != SMTC_MODEM_RC_OK) {
			LOG_ERR("Failed to set DevEUI: %s", smtc_modem_rc_to_str(rc));
			return -EINVAL;
		}
	}

	if (config->otaa.join_eui != NULL) {
		LOG_HEXDUMP_DBG(config->otaa.join_eui, 8, "JoinEUI");
		rc = smtc_modem_set_joineui(STACK_ID, config->otaa.join_eui);
		if (rc != SMTC_MODEM_RC_OK) {
			LOG_ERR("Failed to set JoinEUI: %s", smtc_modem_rc_to_str(rc));
			return -EINVAL;
		}
	}

	if (config->otaa.nwk_key != NULL) {
		rc = smtc_modem_set_nwkkey(STACK_ID, config->otaa.nwk_key);
		if (rc != SMTC_MODEM_RC_OK) {
			LOG_ERR("Failed to set NwkKey: %s", smtc_modem_rc_to_str(rc));
			return -EINVAL;
		}
	}

	if (config->otaa.app_key != NULL) {
		rc = smtc_modem_set_appkey(STACK_ID, config->otaa.app_key);
		if (rc != SMTC_MODEM_RC_OK) {
			LOG_ERR("Failed to set AppKey: %s", smtc_modem_rc_to_str(rc));
			return -EINVAL;
		}
	}

	return 0;
}

int lorawan_join(const struct lorawan_join_config *config)
{
	smtc_modem_return_code_t rc;
	bool joined;
	int ret;

	if (config == NULL) {
		return -EINVAL;
	}

	/* ABP mode is not supported in lora-basics-modem backend */
	if (config->mode != LORAWAN_ACT_OTAA) {
		LOG_ERR("Invalid activation mode: %d", config->mode);
		return -EINVAL;
	}

	ret = lorawan_join_otaa(config);
	if (ret != 0) {
		return ret;
	}

	k_msgq_purge(&join_msgq);

	rc = smtc_modem_join_network(STACK_ID);
	if (rc != SMTC_MODEM_RC_OK) {
		LOG_ERR("Failed to start join: %s", smtc_modem_rc_to_str(rc));
		return (rc == SMTC_MODEM_RC_BUSY) ? -EBUSY : -EIO;
	}

	smtc_modem_hal_user_lbm_irq();

	LOG_DBG("Join request sent, waiting for response...");
	ret = k_msgq_get(&join_msgq, &joined, LBM_TIMEOUT);
	if (ret != 0) {
		LOG_ERR("Join timeout");
		return -ETIMEDOUT;
	}

	if (!joined) {
		LOG_ERR("Join failed");
		return -ETIMEDOUT;
	}

	return 0;
}

int lorawan_start(void)
{
	smtc_modem_return_code_t rc;

	if (regions.active == REGION_NOT_SET) {
		LOG_ERR("No active region set. Call lorawan_set_region() when multiple regions are "
			"enabled.");
		return -EINVAL;
	}

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s: device not ready.", lora_dev->name);
		return -ENODEV;
	}

	LOG_INF("LoRa device: %s", lora_dev->name);

	LOG_DBG("Starting LBM thread");
	k_thread_start(lbm_thread);

	LOG_DBG("Waiting for modem reset event...");
	if (k_sem_take(&modem_reset_sem, LBM_MODEM_RESET_TIMEOUT) != 0) {
		LOG_ERR("Timeout waiting for modem reset");
		return -ETIMEDOUT;
	}

	LOG_INF("Modem ready, setting region %s", lbm_region_to_str(regions.active));
	rc = smtc_modem_set_region(STACK_ID, regions.active);
	if (rc != SMTC_MODEM_RC_OK) {
		LOG_ERR("Failed to set region: %s", smtc_modem_rc_to_str(rc));
		return -EINVAL;
	}

	return 0;
}

int lorawan_send(uint8_t port, uint8_t *data, uint8_t len, enum lorawan_message_type type)
{
	smtc_modem_event_txdone_status_t tx_status;
	smtc_modem_return_code_t rc;
	bool confirmed = (type == LORAWAN_MSG_CONFIRMED);
	int ret;

	if (port == 0 || port > 223) {
		LOG_ERR("Invalid port: %u (must be 1-223)", port);
		return -EINVAL;
	}

	LOG_INF("Sending %s uplink on port %u (%u bytes)", confirmed ? "confirmed" : "unconfirmed",
		port, len);
	LOG_HEXDUMP_DBG(data, len, "Payload");

	k_msgq_purge(&tx_msgq);

	rc = smtc_modem_request_uplink(STACK_ID, port, confirmed, data, len);
	if (rc != SMTC_MODEM_RC_OK) {
		LOG_ERR("Failed to request uplink: %s", smtc_modem_rc_to_str(rc));
		return -EINVAL;
	}

	smtc_modem_hal_user_lbm_irq();

	LOG_DBG("Uplink request queued, waiting for TX completion...");
	ret = k_msgq_get(&tx_msgq, &tx_status, LBM_TIMEOUT);
	if (ret != 0) {
		LOG_ERR("TX timeout waiting for TXDONE");
		return -ETIMEDOUT;
	}

	return lorawan_txstatus2errno(tx_status, confirmed);
}

void lorawan_get_payload_sizes(uint8_t *max_next_payload_size, uint8_t *max_payload_size)
{
	uint8_t tx_max;
	smtc_modem_return_code_t rc;

	rc = smtc_modem_get_next_tx_max_payload(STACK_ID, &tx_max);
	if (rc != SMTC_MODEM_RC_OK) {
		LOG_WRN("Failed to get max payload size: %s", smtc_modem_rc_to_str(rc));
		*max_next_payload_size = 0;
		*max_payload_size = 0;
		return;
	}

	/*
	 * LBM only provides max payload for next TX which accounts for MAC overhead.
	 * Use same value for both - actual max at DR is region-specific and not
	 * directly exposed by LBM API.
	 */
	*max_next_payload_size = tx_max;
	*max_payload_size = tx_max;
}

int lorawan_set_region(enum lorawan_region region)
{
	static const smtc_modem_region_t region_map[] = {
		[LORAWAN_REGION_AS923] = SMTC_MODEM_REGION_AS_923_GRP1,
		[LORAWAN_REGION_AU915] = SMTC_MODEM_REGION_AU_915,
		[LORAWAN_REGION_CN470] = SMTC_MODEM_REGION_CN_470,
		[LORAWAN_REGION_CN779] = 0, /* Not supported */
		[LORAWAN_REGION_EU433] = 0, /* Not supported */
		[LORAWAN_REGION_EU868] = SMTC_MODEM_REGION_EU_868,
		[LORAWAN_REGION_KR920] = SMTC_MODEM_REGION_KR_920,
		[LORAWAN_REGION_IN865] = SMTC_MODEM_REGION_IN_865,
		[LORAWAN_REGION_US915] = SMTC_MODEM_REGION_US_915,
		[LORAWAN_REGION_RU864] = SMTC_MODEM_REGION_RU_864,
	};
	smtc_modem_region_t lbm_region;

	if (region >= ARRAY_SIZE(region_map)) {
		LOG_ERR("Unknown region: %d", region);
		return -EINVAL;
	}

	lbm_region = region_map[region];
	if (lbm_region == 0) {
		LOG_ERR("Region not supported by lora-basics-modem");
		return -ENOTSUP;
	}

	if (!regions.enabled[lbm_region]) {
		LOG_ERR("Region %s is not enabled in Kconfig", lbm_region_to_str(lbm_region));
		return -ENOTSUP;
	}

	regions.active = lbm_region;
	LOG_INF("Region %s configured", lbm_region_to_str(lbm_region));

	return 0;
}

void lorawan_register_downlink_callback(struct lorawan_downlink_cb *cb)
{
	sys_slist_append(&dl_callbacks, &cb->node);
}

void lorawan_register_dr_changed_callback(lorawan_dr_changed_cb_t cb)
{
	dr_changed_cb = cb;
}

void lorawan_register_battery_level_callback(lorawan_battery_level_cb_t cb)
{
	ARG_UNUSED(cb);
}

int lorawan_set_class(enum lorawan_class dev_class)
{
	ARG_UNUSED(dev_class);
	return -ENOTSUP;
}

int lorawan_set_conf_msg_tries(uint8_t tries)
{
	ARG_UNUSED(tries);
	return -ENOTSUP;
}

void lorawan_enable_adr(bool enable)
{
	ARG_UNUSED(enable);
}

int lorawan_set_channels_mask(uint16_t *channels_mask, size_t channels_mask_size)
{
	ARG_UNUSED(channels_mask);
	ARG_UNUSED(channels_mask_size);
	return -ENOTSUP;
}

int lorawan_set_datarate(enum lorawan_datarate dr)
{
	ARG_UNUSED(dr);
	return -ENOTSUP;
}

enum lorawan_datarate lorawan_get_min_datarate(void)
{
	return LORAWAN_DR_0;
}

int lorawan_request_device_time(bool force_request)
{
	ARG_UNUSED(force_request);
	return -ENOTSUP;
}

int lorawan_device_time_get(uint32_t *gps_time)
{
	ARG_UNUSED(gps_time);
	return -ENOTSUP;
}

int lorawan_request_link_check(bool force_request)
{
	ARG_UNUSED(force_request);
	return -ENOTSUP;
}

void lorawan_register_link_check_ans_callback(lorawan_link_check_ans_cb_t cb)
{
	ARG_UNUSED(cb);
}

int lorawan_clock_sync_run(void)
{
	return -ENOTSUP;
}

int lorawan_clock_sync_get(uint32_t *gps_time)
{
	ARG_UNUSED(gps_time);
	return -ENOTSUP;
}

void lorawan_frag_transport_register_descriptor_callback(transport_descriptor_cb cb)
{
	ARG_UNUSED(cb);
}

int lorawan_frag_transport_run(void (*transport_finished_cb)(void))
{
	ARG_UNUSED(transport_finished_cb);
	return -ENOTSUP;
}

static int lorawan_init(void)
{
	size_t count = 0;
	smtc_modem_region_t single_region = REGION_NOT_SET;

	sys_slist_init(&dl_callbacks);

	for (size_t i = 1; i < SMTC_MODEM_REGION_MAX; i++) {
		if (regions.enabled[i]) {
			single_region = (smtc_modem_region_t)i;
			count++;
		}
	}

	LOG_DBG("Enabled regions: %zu", count);

	if (count == 1) {
		regions.active = single_region;
		LOG_DBG("Auto-selected region %s", lbm_region_to_str(single_region));
	}

	return 0;
}
SYS_INIT(lorawan_init, POST_KERNEL, 0);
