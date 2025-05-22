/*
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <errno.h>
#include <zephyr/lorawan/lorawan.h>

#include "lw_priv.h"

#include <LoRaMac.h>
#include <Region.h>
#include "nvm/lorawan_nvm.h"

#ifdef CONFIG_LORAMAC_REGION_AS923
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_AS923
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_AS923
#elif CONFIG_LORAMAC_REGION_AU915
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_AU915
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_AU915
#elif CONFIG_LORAMAC_REGION_CN470
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_CN470
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_CN470
#elif CONFIG_LORAMAC_REGION_CN779
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_CN779
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_CN779
#elif CONFIG_LORAMAC_REGION_EU433
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_EU433
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_EU433
#elif CONFIG_LORAMAC_REGION_EU868
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_EU868
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_EU868
#elif CONFIG_LORAMAC_REGION_KR920
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_KR920
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_KR920
#elif CONFIG_LORAMAC_REGION_IN865
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_IN865
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_IN865
#elif CONFIG_LORAMAC_REGION_US915
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_US915
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_US915
#elif CONFIG_LORAMAC_REGION_RU864
#define DEFAULT_LORAWAN_REGION             LORAMAC_REGION_RU864
#define DEFAULT_LORAWAN_CHANNELS_MASK_SIZE LORAWAN_CHANNELS_MASK_SIZE_RU864
#else
#error "At least one LoRaWAN region should be selected"
#endif

/* Use version 1.0.3.0 for ABP */
#define LORAWAN_ABP_VERSION 0x01000300

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan, CONFIG_LORAWAN_LOG_LEVEL);

K_SEM_DEFINE(mlme_confirm_sem, 0, 1);
K_SEM_DEFINE(mcps_confirm_sem, 0, 1);

K_MUTEX_DEFINE(lorawan_join_mutex);
K_MUTEX_DEFINE(lorawan_send_mutex);

/* lorawan flags: store lorawan states */
enum {
	LORAWAN_FLAG_ADR_ENABLE,
	LORAWAN_FLAG_DEVICETIME_UPDATED_ONCE,
	LORAWAN_FLAG_COUNT,
};

/* We store both the default datarate requested through lorawan_set_datarate
 * and the current datarate so that we can use the default datarate for all
 * join requests, even as the current datarate changes due to ADR.
 */
static enum lorawan_datarate default_datarate;
static enum lorawan_datarate current_datarate;
static ATOMIC_DEFINE(lorawan_flags, LORAWAN_FLAG_COUNT);

static sys_slist_t dl_callbacks;

static LoRaMacPrimitives_t mac_primitives;
static LoRaMacCallback_t mac_callbacks;

static LoRaMacEventInfoStatus_t last_mcps_confirm_status;
static LoRaMacEventInfoStatus_t last_mlme_confirm_status;
static LoRaMacEventInfoStatus_t last_mcps_indication_status;
static LoRaMacEventInfoStatus_t last_mlme_indication_status;

static LoRaMacRegion_t selected_region = DEFAULT_LORAWAN_REGION;

static enum lorawan_channels_mask_size region_channels_mask_size =
	DEFAULT_LORAWAN_CHANNELS_MASK_SIZE;

static lorawan_battery_level_cb_t battery_level_cb;
static lorawan_dr_changed_cb_t dr_changed_cb;
static lorawan_link_check_ans_cb_t link_check_cb;

/* implementation required by the soft-se (software secure element) */
void BoardGetUniqueId(uint8_t *id)
{
	/* Do not change the default value */
}

static uint8_t get_battery_level(void)
{
	if (battery_level_cb != NULL) {
		return battery_level_cb();
	} else {
		return 255;
	}
}

static void mac_process_notify(void)
{
	LoRaMacProcess();
}

static void datarate_observe(bool force_notification)
{
	MibRequestConfirm_t mib_req;

	mib_req.Type = MIB_CHANNELS_DATARATE;
	LoRaMacMibGetRequestConfirm(&mib_req);

	if ((mib_req.Param.ChannelsDatarate != current_datarate) ||
	    (force_notification)) {
		current_datarate = mib_req.Param.ChannelsDatarate;
		if (dr_changed_cb != NULL) {
			dr_changed_cb(current_datarate);
		}
		LOG_INF("Datarate changed: DR_%d", current_datarate);
	}
}

static void mcps_confirm_handler(McpsConfirm_t *mcps_confirm)
{
	LOG_DBG("Received McpsConfirm (for McpsRequest %d)",
		mcps_confirm->McpsRequest);

	if (mcps_confirm->Status != LORAMAC_EVENT_INFO_STATUS_OK) {
		LOG_ERR("McpsRequest failed : %s",
			lorawan_eventinfo2str(mcps_confirm->Status));
	} else {
		LOG_DBG("McpsRequest success!");
	}

	/* Datarate may have changed due to a missed ADRACK */
	if (atomic_test_bit(lorawan_flags, LORAWAN_FLAG_ADR_ENABLE)) {
		datarate_observe(false);
	}

	last_mcps_confirm_status = mcps_confirm->Status;
	k_sem_give(&mcps_confirm_sem);
}

static void mcps_indication_handler(McpsIndication_t *mcps_indication)
{
	struct lorawan_downlink_cb *cb;
	uint8_t flags = 0;

	LOG_DBG("Received McpsIndication %d", mcps_indication->McpsIndication);

	if (mcps_indication->Status != LORAMAC_EVENT_INFO_STATUS_OK) {
		LOG_ERR("McpsIndication failed : %s",
			lorawan_eventinfo2str(mcps_indication->Status));
		return;
	}

	/* Datarate can change as result of ADR command from server */
	if (atomic_test_bit(lorawan_flags, LORAWAN_FLAG_ADR_ENABLE)) {
		datarate_observe(false);
	}

	/* Save time has been updated at least once */
	if (!atomic_test_bit(lorawan_flags, LORAWAN_FLAG_DEVICETIME_UPDATED_ONCE) &&
	    mcps_indication->DeviceTimeAnsReceived) {
		atomic_set_bit(lorawan_flags, LORAWAN_FLAG_DEVICETIME_UPDATED_ONCE);
	}

	/* IsUplinkTxPending also indicates pending downlinks */
	flags |= (mcps_indication->IsUplinkTxPending == 1 ? LORAWAN_DATA_PENDING : 0);
	flags |= (mcps_indication->DeviceTimeAnsReceived ? LORAWAN_TIME_UPDATED : 0);

	/* Iterate over all registered downlink callbacks */
	SYS_SLIST_FOR_EACH_CONTAINER(&dl_callbacks, cb, node) {
		if ((cb->port == LW_RECV_PORT_ANY) ||
		    (cb->port == mcps_indication->Port)) {
			cb->cb(mcps_indication->Port, flags, mcps_indication->Rssi,
			       mcps_indication->Snr, mcps_indication->BufferSize,
			       mcps_indication->Buffer);
		}
	}

	last_mcps_indication_status = mcps_indication->Status;
}

static void mlme_confirm_handler(MlmeConfirm_t *mlme_confirm)
{
	MibRequestConfirm_t mib_req;

	LOG_DBG("Received MlmeConfirm (for MlmeRequest %d)",
		mlme_confirm->MlmeRequest);

	if (mlme_confirm->Status != LORAMAC_EVENT_INFO_STATUS_OK) {
		LOG_ERR("MlmeConfirm failed : %s",
			lorawan_eventinfo2str(mlme_confirm->Status));
		goto out_sem;
	}

	switch (mlme_confirm->MlmeRequest) {
	case MLME_JOIN:
		mib_req.Type = MIB_DEV_ADDR;
		LoRaMacMibGetRequestConfirm(&mib_req);
		LOG_INF("Joined network! DevAddr: %08x", mib_req.Param.DevAddr);
		break;
	case MLME_LINK_CHECK:
		if (link_check_cb != NULL) {
			link_check_cb(mlme_confirm->DemodMargin, mlme_confirm->NbGateways);
		}
		LOG_INF("Link check done");
		break;
	case MLME_DEVICE_TIME:
		LOG_INF("DevTimeReq done");
		break;
	default:
		break;
	}

out_sem:
	last_mlme_confirm_status = mlme_confirm->Status;
	k_sem_give(&mlme_confirm_sem);
}

static void mlme_indication_handler(MlmeIndication_t *mlme_indication)
{
	LOG_DBG("Received MlmeIndication %d", mlme_indication->MlmeIndication);
	last_mlme_indication_status = mlme_indication->Status;
}

static LoRaMacStatus_t lorawan_join_otaa(
	const struct lorawan_join_config *join_cfg)
{
	MlmeReq_t mlme_req;
	MibRequestConfirm_t mib_req;

	mlme_req.Type = MLME_JOIN;
	mlme_req.Req.Join.Datarate = default_datarate;
	mlme_req.Req.Join.NetworkActivation = ACTIVATION_TYPE_OTAA;

	if (IS_ENABLED(CONFIG_LORAWAN_NVM_NONE)) {
		/* Retrieve the NVM context to store device nonce */
		mib_req.Type = MIB_NVM_CTXS;
		if (LoRaMacMibGetRequestConfirm(&mib_req) !=
			LORAMAC_STATUS_OK) {
			LOG_ERR("Could not get NVM context");
			return -EINVAL;
		}
		mib_req.Param.Contexts->Crypto.DevNonce =
			join_cfg->otaa.dev_nonce;
	}

	mib_req.Type = MIB_DEV_EUI;
	mib_req.Param.DevEui = join_cfg->dev_eui;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_JOIN_EUI;
	mib_req.Param.JoinEui = join_cfg->otaa.join_eui;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_NWK_KEY;
	mib_req.Param.NwkKey = join_cfg->otaa.nwk_key;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_APP_KEY;
	mib_req.Param.AppKey = join_cfg->otaa.app_key;
	LoRaMacMibSetRequestConfirm(&mib_req);

	return LoRaMacMlmeRequest(&mlme_req);
}

static LoRaMacStatus_t lorawan_join_abp(
	const struct lorawan_join_config *join_cfg)
{
	MibRequestConfirm_t mib_req;

	mib_req.Type = MIB_ABP_LORAWAN_VERSION;
	mib_req.Param.AbpLrWanVersion.Value = LORAWAN_ABP_VERSION;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_NET_ID;
	mib_req.Param.NetID = 0;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_DEV_ADDR;
	mib_req.Param.DevAddr = join_cfg->abp.dev_addr;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_F_NWK_S_INT_KEY;
	mib_req.Param.FNwkSIntKey = join_cfg->abp.nwk_skey;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_S_NWK_S_INT_KEY;
	mib_req.Param.SNwkSIntKey = join_cfg->abp.nwk_skey;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_NWK_S_ENC_KEY;
	mib_req.Param.NwkSEncKey = join_cfg->abp.nwk_skey;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_APP_S_KEY;
	mib_req.Param.AppSKey = join_cfg->abp.app_skey;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_NETWORK_ACTIVATION;
	mib_req.Param.NetworkActivation = ACTIVATION_TYPE_ABP;
	LoRaMacMibSetRequestConfirm(&mib_req);

	return LORAMAC_STATUS_OK;
}

int lorawan_set_region(enum lorawan_region region)
{
	switch (region) {

#if defined(CONFIG_LORAMAC_REGION_AS923)
	case LORAWAN_REGION_AS923:
		selected_region = LORAMAC_REGION_AS923;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_AS923;
		break;
#endif

#if defined(CONFIG_LORAMAC_REGION_AU915)
	case LORAWAN_REGION_AU915:
		selected_region = LORAMAC_REGION_AU915;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_AU915;
		break;
#endif

#if defined(CONFIG_LORAMAC_REGION_CN470)
	case LORAWAN_REGION_CN470:
		selected_region = LORAMAC_REGION_CN470;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_CN470;
		break;
#endif

#if defined(CONFIG_LORAMAC_REGION_CN779)
	case LORAWAN_REGION_CN779:
		selected_region = LORAMAC_REGION_CN779;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_CN779;
		break;
#endif

#if defined(CONFIG_LORAMAC_REGION_EU433)
	case LORAWAN_REGION_EU433:
		selected_region = LORAMAC_REGION_EU433;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_EU433;
		break;
#endif

#if defined(CONFIG_LORAMAC_REGION_EU868)
	case LORAWAN_REGION_EU868:
		selected_region = LORAMAC_REGION_EU868;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_EU868;
		break;
#endif

#if defined(CONFIG_LORAMAC_REGION_KR920)
	case LORAWAN_REGION_KR920:
		selected_region = LORAMAC_REGION_KR920;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_KR920;
		break;
#endif

#if defined(CONFIG_LORAMAC_REGION_IN865)
	case LORAWAN_REGION_IN865:
		selected_region = LORAMAC_REGION_IN865;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_IN865;
		break;
#endif

#if defined(CONFIG_LORAMAC_REGION_US915)
	case LORAWAN_REGION_US915:
		selected_region = LORAMAC_REGION_US915;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_US915;
		break;
#endif

#if defined(CONFIG_LORAMAC_REGION_RU864)
	case LORAWAN_REGION_RU864:
		selected_region = LORAMAC_REGION_RU864;
		region_channels_mask_size = LORAWAN_CHANNELS_MASK_SIZE_RU864;
		break;
#endif

	default:
		LOG_ERR("No support for region %d!", region);
		return -ENOTSUP;
	}

	LOG_DBG("Selected region %d", region);

	return 0;
}

int lorawan_request_link_check(bool force_request)
{
	int ret = 0;
	LoRaMacStatus_t status;
	MlmeReq_t mlme_req;

	mlme_req.Type = MLME_LINK_CHECK;
	status = LoRaMacMlmeRequest(&mlme_req);
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("LinkCheckReq failed: %s", lorawan_status2str(status));
		ret = lorawan_status2errno(status);
		return ret;
	}

	if (force_request) {
		ret = lorawan_send(0U, "", 0U, LORAWAN_MSG_UNCONFIRMED);
	}

	return ret;
}

int lorawan_request_device_time(bool force_request)
{
	int ret = 0;
	LoRaMacStatus_t status;
	MlmeReq_t mlme_req;

	mlme_req.Type = MLME_DEVICE_TIME;
	status = LoRaMacMlmeRequest(&mlme_req);
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("DeviceTime Req. failed: %s", lorawan_status2str(status));
		ret = lorawan_status2errno(status);
		return ret;
	}

	if (force_request) {
		ret = lorawan_send(0U, "", 0U, LORAWAN_MSG_UNCONFIRMED);
	}

	return ret;
}

int lorawan_device_time_get(uint32_t *gps_time)
{
	SysTime_t local_time;

	__ASSERT(gps_time != NULL, "gps_time parameter is required");

	if (!atomic_test_bit(lorawan_flags, LORAWAN_FLAG_DEVICETIME_UPDATED_ONCE)) {
		return -EAGAIN;
	}

	local_time = SysTimeGet();
	*gps_time = local_time.Seconds - UNIX_GPS_EPOCH_OFFSET;
	return 0;
}

int lorawan_join(const struct lorawan_join_config *join_cfg)
{
	MibRequestConfirm_t mib_req;
	LoRaMacStatus_t status;
	int ret = 0;

	k_mutex_lock(&lorawan_join_mutex, K_FOREVER);

	/* MIB_PUBLIC_NETWORK powers on the radio and does not turn it off */
	mib_req.Type = MIB_PUBLIC_NETWORK;
	mib_req.Param.EnablePublicNetwork = IS_ENABLED(CONFIG_LORAWAN_PUBLIC_NETWORK);
	LoRaMacMibSetRequestConfirm(&mib_req);

	if (join_cfg->mode == LORAWAN_ACT_OTAA) {
		status = lorawan_join_otaa(join_cfg);
		if (status != LORAMAC_STATUS_OK) {
			LOG_ERR("OTAA join failed: %s",
				lorawan_status2str(status));
			ret = lorawan_status2errno(status);
			goto out;
		}

		LOG_DBG("Network join request sent!");

		/*
		 * We can be sure that the semaphore will be released for
		 * both success and failure cases after a specific time period.
		 * So we can use K_FOREVER and no need to check the return val.
		 */
		k_sem_take(&mlme_confirm_sem, K_FOREVER);
		if (last_mlme_confirm_status != LORAMAC_EVENT_INFO_STATUS_OK) {
			ret = lorawan_eventinfo2errno(last_mlme_confirm_status);
			goto out;
		}
	} else if (join_cfg->mode == LORAWAN_ACT_ABP) {
		status = lorawan_join_abp(join_cfg);
		if (status != LORAMAC_STATUS_OK) {
			LOG_ERR("ABP join failed: %s",
				lorawan_status2str(status));
			ret = lorawan_status2errno(status);
			goto out;
		}
	} else {
		ret = -EINVAL;
	}

out:
	/* If the join succeeded */
	if (ret == 0) {
		/*
		 * Several regions (AS923, AU915, US915) overwrite the
		 * datarate as part of the join process. Reset the datarate
		 * to the value requested (and validated) in
		 * lorawan_set_datarate so that the MAC layer is aware of the
		 * set datarate for LoRaMacQueryTxPossible. This is only
		 * performed when ADR is disabled as it the network servers
		 * responsibility to increase datarates when ADR is enabled.
		 */
		if (!atomic_test_bit(lorawan_flags, LORAWAN_FLAG_ADR_ENABLE)) {
			MibRequestConfirm_t mib_req2;

			mib_req2.Type = MIB_CHANNELS_DATARATE;
			mib_req2.Param.ChannelsDatarate = default_datarate;
			LoRaMacMibSetRequestConfirm(&mib_req2);
		}

		/*
		 * Force a notification of the datarate on network join as the
		 * user may not have explicitly set a datarate to use.
		 */
		datarate_observe(true);
	}

	k_mutex_unlock(&lorawan_join_mutex);
	return ret;
}

int lorawan_set_class(enum lorawan_class dev_class)
{
	MibRequestConfirm_t mib_req;
	DeviceClass_t current_class;
	LoRaMacStatus_t status;

	mib_req.Type = MIB_DEVICE_CLASS;
	LoRaMacMibGetRequestConfirm(&mib_req);
	current_class = mib_req.Param.Class;

	switch (dev_class) {
	case LORAWAN_CLASS_A:
		mib_req.Param.Class = CLASS_A;
		break;
	case LORAWAN_CLASS_B:
		LOG_ERR("Class B not supported yet!");
		return -ENOTSUP;
	case LORAWAN_CLASS_C:
		mib_req.Param.Class = CLASS_C;
		break;
	default:
		return -EINVAL;
	}

	if (mib_req.Param.Class != current_class) {
		status = LoRaMacMibSetRequestConfirm(&mib_req);
		if (status != LORAMAC_STATUS_OK) {
			LOG_ERR("Failed to set device class: %s",
				lorawan_status2str(status));
			return lorawan_status2errno(status);
		}
	}

	return 0;
}

int lorawan_set_channels_mask(uint16_t *channels_mask, size_t channels_mask_size)
{
	MibRequestConfirm_t mib_req;

	if ((channels_mask == NULL) || (channels_mask_size != region_channels_mask_size)) {
		return -EINVAL;
	}

	/* Notify MAC layer of the requested channel mask. */
	mib_req.Type = MIB_CHANNELS_MASK;
	mib_req.Param.ChannelsMask = channels_mask;

	if (LoRaMacMibSetRequestConfirm(&mib_req) != LORAMAC_STATUS_OK) {
		/* Channels mask is invalid for this region. */
		return -EINVAL;
	}

	return 0;
}

int lorawan_set_datarate(enum lorawan_datarate dr)
{
	MibRequestConfirm_t mib_req;

	/* Bail out if using ADR */
	if (atomic_test_bit(lorawan_flags, LORAWAN_FLAG_ADR_ENABLE)) {
		return -EINVAL;
	}

	/* Notify MAC layer of the requested datarate */
	mib_req.Type = MIB_CHANNELS_DATARATE;
	mib_req.Param.ChannelsDatarate = dr;
	if (LoRaMacMibSetRequestConfirm(&mib_req) != LORAMAC_STATUS_OK) {
		/* Datarate is invalid for this region */
		return -EINVAL;
	}

	default_datarate = dr;
	current_datarate = dr;

	return 0;
}

void lorawan_get_payload_sizes(uint8_t *max_next_payload_size,
			       uint8_t *max_payload_size)
{
	LoRaMacTxInfo_t tx_info;

	/* QueryTxPossible cannot fail */
	(void) LoRaMacQueryTxPossible(0, &tx_info);

	*max_next_payload_size = tx_info.MaxPossibleApplicationDataSize;
	*max_payload_size = tx_info.CurrentPossiblePayloadSize;
}

enum lorawan_datarate lorawan_get_min_datarate(void)
{
	MibRequestConfirm_t mib_req;

	mib_req.Type = MIB_CHANNELS_MIN_TX_DATARATE;
	LoRaMacMibGetRequestConfirm(&mib_req);

	return mib_req.Param.ChannelsMinTxDatarate;
}

void lorawan_enable_adr(bool enable)
{
	MibRequestConfirm_t mib_req;

	if (enable != atomic_test_bit(lorawan_flags, LORAWAN_FLAG_ADR_ENABLE)) {
		atomic_set_bit_to(lorawan_flags, LORAWAN_FLAG_ADR_ENABLE, enable);

		mib_req.Type = MIB_ADR;
		mib_req.Param.AdrEnable = atomic_test_bit(lorawan_flags, LORAWAN_FLAG_ADR_ENABLE);
		LoRaMacMibSetRequestConfirm(&mib_req);
	}
}

int lorawan_set_conf_msg_tries(uint8_t tries)
{
	MibRequestConfirm_t mib_req;

	mib_req.Type = MIB_CHANNELS_NB_TRANS;
	mib_req.Param.ChannelsNbTrans = tries;
	if (LoRaMacMibSetRequestConfirm(&mib_req) != LORAMAC_STATUS_OK) {
		return -EINVAL;
	}

	return 0;
}

int lorawan_send(uint8_t port, uint8_t *data, uint8_t len,
		 enum lorawan_message_type type)
{
	LoRaMacStatus_t status;
	McpsReq_t mcps_req;
	LoRaMacTxInfo_t tx_info;
	int ret = 0;
	bool empty_frame = false;

	if (data == NULL && len > 0) {
		return -EINVAL;
	}

	k_mutex_lock(&lorawan_send_mutex, K_FOREVER);

	status = LoRaMacQueryTxPossible(len, &tx_info);
	if (status != LORAMAC_STATUS_OK) {
		/*
		 * If status indicates an error, then most likely the payload
		 * has exceeded the maximum possible length for the current
		 * region and datarate. We can't do much other than sending
		 * empty frame in order to flush MAC commands in stack and
		 * hoping the application to lower the payload size for
		 * next try.
		 */
		LOG_ERR("LoRaWAN Query Tx Possible Failed: %s",
			lorawan_status2str(status));
		empty_frame = true;
		mcps_req.Type = MCPS_UNCONFIRMED;
		mcps_req.Req.Unconfirmed.fBuffer = NULL;
		mcps_req.Req.Unconfirmed.fBufferSize = 0;
		mcps_req.Req.Unconfirmed.Datarate = DR_0;
	} else {
		switch (type) {
		case LORAWAN_MSG_UNCONFIRMED:
			mcps_req.Type = MCPS_UNCONFIRMED;
			break;
		case LORAWAN_MSG_CONFIRMED:
			mcps_req.Type = MCPS_CONFIRMED;
			break;
		}
		mcps_req.Req.Unconfirmed.fPort = port;
		mcps_req.Req.Unconfirmed.fBuffer = data;
		mcps_req.Req.Unconfirmed.fBufferSize = len;
		mcps_req.Req.Unconfirmed.Datarate = current_datarate;
	}

	status = LoRaMacMcpsRequest(&mcps_req);
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("LoRaWAN Send failed: %s", lorawan_status2str(status));
		ret = lorawan_status2errno(status);
		goto out;
	}

	/*
	 * Always wait for MAC operations to complete.
	 * We can be sure that the semaphore will be released for
	 * both success and failure cases after a specific time period.
	 * So we can use K_FOREVER and no need to check the return val.
	 */
	k_sem_take(&mcps_confirm_sem, K_FOREVER);
	if (last_mcps_confirm_status != LORAMAC_EVENT_INFO_STATUS_OK) {
		ret = lorawan_eventinfo2errno(last_mcps_confirm_status);
	}

	/*
	 * Indicate to the application that the provided data was not sent and
	 * it has to resend the packet.
	 */
	if (empty_frame) {
		ret = -EAGAIN;
	}

out:
	k_mutex_unlock(&lorawan_send_mutex);
	return ret;
}

void lorawan_register_battery_level_callback(lorawan_battery_level_cb_t cb)
{
	battery_level_cb = cb;
}

void lorawan_register_downlink_callback(struct lorawan_downlink_cb *cb)
{
	sys_slist_append(&dl_callbacks, &cb->node);
}

void lorawan_register_dr_changed_callback(lorawan_dr_changed_cb_t cb)
{
	dr_changed_cb = cb;
}

void lorawan_register_link_check_ans_callback(lorawan_link_check_ans_cb_t cb)
{
	link_check_cb = cb;
}

int lorawan_start(void)
{
	LoRaMacStatus_t status;
	MibRequestConfirm_t mib_req;
	GetPhyParams_t phy_params;
	PhyParam_t phy_param;

	status = LoRaMacInitialization(&mac_primitives, &mac_callbacks,
				       selected_region);
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("LoRaMacInitialization failed: %s",
			lorawan_status2str(status));
		return -EINVAL;
	}

	LOG_DBG("LoRaMAC Initialized");

	if (!IS_ENABLED(CONFIG_LORAWAN_NVM_NONE)) {
		lorawan_nvm_init();
		lorawan_nvm_data_restore();
	}

	status = LoRaMacStart();
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("Failed to start the LoRaMAC stack: %s",
			lorawan_status2str(status));
		return -EINVAL;
	}

	/* Retrieve the default TX datarate for selected region */
	phy_params.Attribute = PHY_DEF_TX_DR;
	phy_param = RegionGetPhyParam(selected_region, &phy_params);
	default_datarate = phy_param.Value;
	current_datarate = default_datarate;

	/* TODO: Move these to a proper location */
	mib_req.Type = MIB_SYSTEM_MAX_RX_ERROR;
	mib_req.Param.SystemMaxRxError = CONFIG_LORAWAN_SYSTEM_MAX_RX_ERROR;
	LoRaMacMibSetRequestConfirm(&mib_req);

	return 0;
}

static int lorawan_init(void)
{

	sys_slist_init(&dl_callbacks);

	mac_primitives.MacMcpsConfirm = mcps_confirm_handler;
	mac_primitives.MacMcpsIndication = mcps_indication_handler;
	mac_primitives.MacMlmeConfirm = mlme_confirm_handler;
	mac_primitives.MacMlmeIndication = mlme_indication_handler;
	mac_callbacks.GetBatteryLevel = get_battery_level;
	mac_callbacks.GetTemperatureLevel = NULL;

	if (IS_ENABLED(CONFIG_LORAWAN_NVM_NONE)) {
		mac_callbacks.NvmDataChange = NULL;
	} else {
		mac_callbacks.NvmDataChange = lorawan_nvm_data_mgmt_event;
	}

	mac_callbacks.MacProcessNotify = mac_process_notify;

	return 0;
}

SYS_INIT(lorawan_init, POST_KERNEL, 0);
