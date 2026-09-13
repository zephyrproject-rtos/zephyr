/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/sys/atomic.h>

#include <smtc_modem_api.h>
#include <smtc_modem_hal_ext.h>

#include "lbm_lorawan.h"
#include "lbm_priv.h"

LOG_MODULE_DECLARE(lorawan_lbm, CONFIG_LORAWAN_LOG_LEVEL);

#define JOIN_TIMEOUT K_SECONDS(CONFIG_LORAWAN_LBM_JOIN_TIMEOUT_S)
#define TX_TIMEOUT   K_SECONDS(CONFIG_LORAWAN_LBM_TX_TIMEOUT_S)

enum {
	FLAG_STARTED,
	FLAG_JOINED,
};

static struct {
	struct k_mutex api_lock;
	struct k_sem reset_evt;
	struct k_sem join_evt;
	struct k_sem tx_evt;
	atomic_t flags;
	smtc_modem_event_txdone_status_t tx_status;
	enum lorawan_region region;
	bool region_set;
	struct lorawan_downlink_cb *dl_cb;
	lorawan_dr_changed_cb_t dr_cb;
	lorawan_link_check_ans_cb_t link_check_cb;
	lorawan_battery_level_cb_t battery_cb;
} lbm = {
	.api_lock = Z_MUTEX_INITIALIZER(lbm.api_lock),
	.reset_evt = Z_SEM_INITIALIZER(lbm.reset_evt, 0, 1),
	.join_evt = Z_SEM_INITIALIZER(lbm.join_evt, 0, 1),
	.tx_evt = Z_SEM_INITIALIZER(lbm.tx_evt, 0, 1),
};

static const struct {
	bool enabled;
	enum lorawan_region id;
} region_table[] = {
	{IS_ENABLED(CONFIG_LORAWAN_REGION_AS923), LORAWAN_REGION_AS923},
	{IS_ENABLED(CONFIG_LORAWAN_REGION_AU915), LORAWAN_REGION_AU915},
	{IS_ENABLED(CONFIG_LORAWAN_REGION_CN470), LORAWAN_REGION_CN470},
	{IS_ENABLED(CONFIG_LORAWAN_REGION_EU868), LORAWAN_REGION_EU868},
	{IS_ENABLED(CONFIG_LORAWAN_REGION_IN865), LORAWAN_REGION_IN865},
	{IS_ENABLED(CONFIG_LORAWAN_REGION_KR920), LORAWAN_REGION_KR920},
	{IS_ENABLED(CONFIG_LORAWAN_REGION_US915), LORAWAN_REGION_US915},
	{IS_ENABLED(CONFIG_LORAWAN_REGION_RU864), LORAWAN_REGION_RU864},
};

static int region_to_lbm(enum lorawan_region region, smtc_modem_region_t *out)
{
	switch (region) {
	case LORAWAN_REGION_AS923:
		*out = SMTC_MODEM_REGION_AS_923_GRP1;
		break;
	case LORAWAN_REGION_AU915:
		*out = SMTC_MODEM_REGION_AU_915;
		break;
	case LORAWAN_REGION_CN470:
		*out = SMTC_MODEM_REGION_CN_470;
		break;
	case LORAWAN_REGION_EU868:
		*out = SMTC_MODEM_REGION_EU_868;
		break;
	case LORAWAN_REGION_KR920:
		*out = SMTC_MODEM_REGION_KR_920;
		break;
	case LORAWAN_REGION_IN865:
		*out = SMTC_MODEM_REGION_IN_865;
		break;
	case LORAWAN_REGION_US915:
		*out = SMTC_MODEM_REGION_US_915;
		break;
	case LORAWAN_REGION_RU864:
		*out = SMTC_MODEM_REGION_RU_864;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void handle_downlink(void)
{
	uint8_t payload[SMTC_MODEM_MAX_LORAWAN_PAYLOAD_LENGTH];
	smtc_modem_dl_metadata_t meta;
	uint8_t len;
	uint8_t remaining;

	if (smtc_modem_get_downlink_data(payload, &len, &meta, &remaining) != SMTC_MODEM_RC_OK) {
		return;
	}

	if (lbm.dl_cb == NULL) {
		return;
	}

	if (lbm.dl_cb->port != LW_RECV_PORT_ANY && lbm.dl_cb->port != meta.fport) {
		return;
	}

	lbm.dl_cb->cb(meta.fport, meta.fpending_bit ? LORAWAN_DATA_PENDING : 0, meta.rssi - 64,
		      meta.snr / 4, len, payload);
}

static void handle_link_check(const smtc_modem_event_t *event)
{
	uint8_t margin;
	uint8_t gw_cnt;

	if (event->event_data.link_check.status == SMTC_MODEM_EVENT_MAC_REQUEST_NOT_ANSWERED) {
		LOG_DBG("link check not answered");
		return;
	}

	if (lbm.link_check_cb == NULL) {
		return;
	}

	if (smtc_modem_get_lorawan_link_check_data(LBM_STACK_ID, &margin, &gw_cnt) !=
	    SMTC_MODEM_RC_OK) {
		return;
	}

	lbm.link_check_cb(margin, gw_cnt);
}

void lbm_lorawan_event(const smtc_modem_event_t *event)
{
	LOG_DBG("event %u", event->event_type);

	switch (event->event_type) {
	case SMTC_MODEM_EVENT_RESET:
		k_sem_give(&lbm.reset_evt);
		break;
	case SMTC_MODEM_EVENT_JOINED:
		atomic_set_bit(&lbm.flags, FLAG_JOINED);
		k_sem_give(&lbm.join_evt);
		break;
	case SMTC_MODEM_EVENT_JOINFAIL:
		/* The modem keeps retrying, so let the caller keep waiting. */
		LOG_DBG("join attempt failed");
		break;
	case SMTC_MODEM_EVENT_TXDONE:
		lbm.tx_status = event->event_data.txdone.status;
		k_sem_give(&lbm.tx_evt);
		break;
	case SMTC_MODEM_EVENT_DOWNDATA:
		handle_downlink();
		break;
	case SMTC_MODEM_EVENT_LINK_CHECK:
		handle_link_check(event);
		break;
	default:
		LOG_DBG("unhandled event %u", event->event_type);
		break;
	}
}

static int lbm_lorawan_init(void)
{
	size_t count = 0;
	enum lorawan_region single = 0;

	for (size_t i = 0; i < ARRAY_SIZE(region_table); i++) {
		if (region_table[i].enabled) {
			single = region_table[i].id;
			count++;
		}
	}

	if (count == 1) {
		lbm.region = single;
		lbm.region_set = true;
	}

	return 0;
}
SYS_INIT(lbm_lorawan_init, POST_KERNEL, 0);

int lorawan_start(void)
{
	smtc_modem_region_t lbm_region;
	smtc_modem_return_code_t rc;
	int ret;

	if (atomic_test_and_set_bit(&lbm.flags, FLAG_STARTED)) {
		return -EALREADY;
	}

	if (!lbm.region_set) {
		ret = -EINVAL;
		LOG_ERR("no region set, call lorawan_set_region() first");
		goto fail;
	}

	ret = region_to_lbm(lbm.region, &lbm_region);
	if (ret < 0) {
		goto fail;
	}

	/* The engine thread reports RESET once the modem core is up. */
	if (k_sem_take(&lbm.reset_evt, K_SECONDS(CONFIG_LORAWAN_LBM_START_TIMEOUT_S)) < 0) {
		ret = -ETIMEDOUT;
		LOG_ERR("modem did not come up");
		goto fail;
	}

	rc = smtc_modem_set_region(LBM_STACK_ID, lbm_region);
	if (rc != SMTC_MODEM_RC_OK) {
		ret = lbm_rc2errno(rc);
		LOG_ERR("set_region failed: %d", rc);
		goto fail;
	}

	return 0;

fail:
	atomic_clear_bit(&lbm.flags, FLAG_STARTED);
	return ret;
}

int lorawan_set_region(enum lorawan_region region)
{
	smtc_modem_region_t unused;
	int ret;

	if (atomic_test_bit(&lbm.flags, FLAG_STARTED)) {
		return -EPERM;
	}

	ret = region_to_lbm(region, &unused);
	if (ret < 0) {
		return ret;
	}

	lbm.region = region;
	lbm.region_set = true;

	return 0;
}

int lorawan_join(const struct lorawan_join_config *config)
{
	smtc_modem_return_code_t rc;
	int ret;

	if (config == NULL) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&lbm.flags, FLAG_STARTED)) {
		return -EPERM;
	}

	if (config->mode != LORAWAN_ACT_OTAA) {
		LOG_ERR("only OTAA is supported");
		return -ENOTSUP;
	}

	if (config->dev_eui == NULL || config->otaa.join_eui == NULL ||
	    config->otaa.nwk_key == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lbm.api_lock, K_FOREVER);

	rc = smtc_modem_set_deveui(LBM_STACK_ID, config->dev_eui);
	if (rc == SMTC_MODEM_RC_OK) {
		rc = smtc_modem_set_joineui(LBM_STACK_ID, config->otaa.join_eui);
	}
	if (rc == SMTC_MODEM_RC_OK) {
		rc = smtc_modem_set_nwkkey(LBM_STACK_ID, config->otaa.nwk_key);
	}
	if (rc != SMTC_MODEM_RC_OK) {
		ret = lbm_rc2errno(rc);
		LOG_ERR("credentials rejected: %s", lbm_rc2str(rc));
		goto out;
	}

	atomic_clear_bit(&lbm.flags, FLAG_JOINED);
	k_sem_reset(&lbm.join_evt);

	rc = smtc_modem_join_network(LBM_STACK_ID);
	if (rc != SMTC_MODEM_RC_OK) {
		ret = lbm_rc2errno(rc);
		LOG_ERR("join_network failed: %s", lbm_rc2str(rc));
		goto out;
	}

	lbm_engine_notify();

	if (k_sem_take(&lbm.join_evt, JOIN_TIMEOUT) < 0) {
		ret = -ETIMEDOUT;
		goto out;
	}

out:
	k_mutex_unlock(&lbm.api_lock);
	return ret;
}

int lorawan_send(uint8_t port, uint8_t *data, uint8_t len, enum lorawan_message_type type)
{
	smtc_modem_return_code_t rc;
	int ret;

	if (data == NULL && len > 0) {
		return -EINVAL;
	}

	if (!atomic_test_bit(&lbm.flags, FLAG_JOINED)) {
		return -EPERM;
	}

	k_mutex_lock(&lbm.api_lock, K_FOREVER);

	k_sem_reset(&lbm.tx_evt);

	rc = smtc_modem_request_uplink(LBM_STACK_ID, port, type == LORAWAN_MSG_CONFIRMED, data,
				       len);
	if (rc != SMTC_MODEM_RC_OK) {
		ret = lbm_rc2errno(rc);
		goto out;
	}

	lbm_engine_notify();

	if (k_sem_take(&lbm.tx_evt, TX_TIMEOUT) < 0) {
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = lbm_txdone2errno(lbm.tx_status, type == LORAWAN_MSG_CONFIRMED);
	if (ret < 0) {
		LOG_WRN("uplink %s", lbm_txdone2str(lbm.tx_status));
	}

out:
	k_mutex_unlock(&lbm.api_lock);
	return ret;
}

int lorawan_set_class(enum lorawan_class dev_class)
{
	if (dev_class != LORAWAN_CLASS_A) {
		return -ENOTSUP;
	}

	return 0;
}

void lorawan_enable_adr(bool enable)
{
	ARG_UNUSED(enable);
}

int lorawan_set_datarate(enum lorawan_datarate dr)
{
	ARG_UNUSED(dr);

	return -ENOSYS;
}

enum lorawan_datarate lorawan_get_min_datarate(void)
{
	uint16_t datarates = 0;

	if (smtc_modem_get_enabled_datarates(LBM_STACK_ID, &datarates) != SMTC_MODEM_RC_OK ||
	    datarates == 0) {
		return LORAWAN_DR_0;
	}

	/* The mask carries one bit per datarate, so the lowest one set is the slowest. */
	return (enum lorawan_datarate)(find_lsb_set(datarates) - 1);
}

void lorawan_get_payload_sizes(uint8_t *max_next_payload_size, uint8_t *max_payload_size)
{
	uint8_t tx_max_payload = 0;

	(void)smtc_modem_get_next_tx_max_payload(LBM_STACK_ID, &tx_max_payload);

	*max_next_payload_size = tx_max_payload;
	*max_payload_size = tx_max_payload;
}

int lorawan_set_conf_msg_tries(uint8_t tries)
{
	ARG_UNUSED(tries);

	return -ENOSYS;
}

int lorawan_set_channels_mask(uint16_t *channels_mask, size_t channels_mask_size)
{
	ARG_UNUSED(channels_mask);
	ARG_UNUSED(channels_mask_size);

	return -ENOSYS;
}

uint8_t lbm_battery_level(void)
{
	if (lbm.battery_cb == NULL) {
		return 255;
	}

	return lbm.battery_cb();
}

void lorawan_register_battery_level_callback(lorawan_battery_level_cb_t cb)
{
	lbm.battery_cb = cb;
}

void lorawan_register_downlink_callback(struct lorawan_downlink_cb *cb)
{
	lbm.dl_cb = cb;
}

void lorawan_register_dr_changed_callback(lorawan_dr_changed_cb_t cb)
{
	lbm.dr_cb = cb;
}

void lorawan_register_link_check_ans_callback(lorawan_link_check_ans_cb_t cb)
{
	lbm.link_check_cb = cb;
}

int lorawan_request_device_time(bool force_request)
{
	ARG_UNUSED(force_request);

	return -ENOSYS;
}

int lorawan_device_time_get(uint32_t *gps_time)
{
	ARG_UNUSED(gps_time);

	return -ENOSYS;
}

int lorawan_request_link_check(bool force_request)
{
	smtc_modem_return_code_t rc;
	int ret = 0;

	if (!atomic_test_bit(&lbm.flags, FLAG_JOINED)) {
		return -EPERM;
	}

	k_mutex_lock(&lbm.api_lock, K_FOREVER);

	rc = smtc_modem_trig_lorawan_mac_request(LBM_STACK_ID,
						 SMTC_MODEM_LORAWAN_MAC_REQ_LINK_CHECK);
	if (rc != SMTC_MODEM_RC_OK) {
		ret = lbm_rc2errno(rc);
		goto out;
	}

	/* Without an uplink to carry it the request waits for the next one. */
	if (force_request) {
		rc = smtc_modem_request_empty_uplink(LBM_STACK_ID, false, 0, false);
		if (rc != SMTC_MODEM_RC_OK) {
			ret = lbm_rc2errno(rc);
			goto out;
		}
	}

	lbm_engine_notify();

out:
	k_mutex_unlock(&lbm.api_lock);
	return ret;
}
