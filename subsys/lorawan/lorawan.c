/*
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <errno.h>
#include <lorawan/lorawan.h>
#include <zephyr.h>

#include "lw_priv.h"

#include <LoRaMac.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_LORAMAC_REGION_UNKNOWN),
	     "Unknown region specified for LoRaWAN in Kconfig");

#ifdef CONFIG_LORAMAC_REGION_AS923
	#define LORAWAN_REGION LORAMAC_REGION_AS923
#elif CONFIG_LORAMAC_REGION_AU915
	#define LORAWAN_REGION LORAMAC_REGION_AU915
#elif CONFIG_LORAMAC_REGION_CN470
	#define LORAWAN_REGION LORAMAC_REGION_CN470
#elif CONFIG_LORAMAC_REGION_CN779
	#define LORAWAN_REGION LORAMAC_REGION_CN779
#elif CONFIG_LORAMAC_REGION_EU433
	#define LORAWAN_REGION LORAMAC_REGION_EU433
#elif CONFIG_LORAMAC_REGION_EU868
	#define LORAWAN_REGION LORAMAC_REGION_EU868
#elif CONFIG_LORAMAC_REGION_KR920
	#define LORAWAN_REGION LORAMAC_REGION_KR920
#elif CONFIG_LORAMAC_REGION_IN865
	#define LORAWAN_REGION LORAMAC_REGION_IN865
#elif CONFIG_LORAMAC_REGION_US915
	#define LORAWAN_REGION LORAMAC_REGION_US915
#elif CONFIG_LORAMAC_REGION_RU864
	#define LORAWAN_REGION LORAMAC_REGION_RU864
#else
	#error "At least one LoRaWAN region should be selected"
#endif

/* Use version 1.0.3.0 for ABP */
#define LORAWAN_ABP_VERSION 0x01000300

#define LOG_LEVEL CONFIG_LORAWAN_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(lorawan);

K_SEM_DEFINE(mlme_confirm_sem, 0, 1);
K_SEM_DEFINE(mcps_confirm_sem, 0, 1);

K_MUTEX_DEFINE(lorawan_join_mutex);
K_MUTEX_DEFINE(lorawan_send_mutex);

static enum lorawan_datarate lorawan_datarate = LORAWAN_DR_0;
static uint8_t lorawan_conf_msg_tries = 1;
static bool lorawan_adr_enable;



static LoRaMacPrimitives_t macPrimitives;
static LoRaMacCallback_t macCallbacks;

static LoRaMacEventInfoStatus_t last_mcps_confirm_status;
static LoRaMacEventInfoStatus_t last_mlme_confirm_status;
static LoRaMacEventInfoStatus_t last_mcps_indication_status;
static LoRaMacEventInfoStatus_t last_mlme_indication_status;

static void OnMacProcessNotify(void)
{
	LoRaMacProcess();
}

static void McpsConfirm(McpsConfirm_t *mcpsConfirm)
{
	LOG_DBG("Received McpsConfirm (for McpsRequest %d)",
		mcpsConfirm->McpsRequest);

	if (mcpsConfirm->Status != LORAMAC_EVENT_INFO_STATUS_OK) {
		LOG_ERR("McpsRequest failed : %s",
			lorawan_eventinfo2str(mcpsConfirm->Status));
	} else {
		LOG_DBG("McpsRequest success!");
	}

	last_mcps_confirm_status = mcpsConfirm->Status;
	/* mcps_confirm_sem is only blocked on in the MCPS_CONFIRMED case */
	if (mcpsConfirm->McpsRequest == MCPS_CONFIRMED) {
		k_sem_give(&mcps_confirm_sem);
	}
}

static void McpsIndication(McpsIndication_t *mcpsIndication)
{
	LOG_DBG("Received McpsIndication %d", mcpsIndication->McpsIndication);

	if (mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK) {
		LOG_ERR("McpsIndication failed : %s",
			lorawan_eventinfo2str(mcpsIndication->Status));
		return;
	}

	/* TODO: Check MCPS Indication type */
	if (mcpsIndication->RxData == true) {
		if (mcpsIndication->BufferSize != 0) {
			LOG_DBG("Rx Data: %s",
				log_strdup(mcpsIndication->Buffer));
		}
	}

	last_mcps_indication_status = mcpsIndication->Status;

	/* TODO: Compliance test based on FPort value*/
}

static void MlmeConfirm(MlmeConfirm_t *mlmeConfirm)
{
	MibRequestConfirm_t mibGet;

	LOG_DBG("Received MlmeConfirm (for MlmeRequest %d)",
		mlmeConfirm->MlmeRequest);

	if (mlmeConfirm->Status != LORAMAC_EVENT_INFO_STATUS_OK) {
		LOG_ERR("MlmeConfirm failed : %s",
			lorawan_eventinfo2str(mlmeConfirm->Status));
		goto out_sem;
	}

	switch (mlmeConfirm->MlmeRequest) {
	case MLME_JOIN:
		mibGet.Type = MIB_DEV_ADDR;
		LoRaMacMibGetRequestConfirm(&mibGet);
		LOG_INF("Joined network! DevAddr: %08x", mibGet.Param.DevAddr);
		break;
	case MLME_LINK_CHECK:
		/* Not implemented */
		LOG_INF("Link check not implemented yet!");
		break;
	default:
		break;
	}

out_sem:
	last_mlme_confirm_status = mlmeConfirm->Status;
	k_sem_give(&mlme_confirm_sem);
}

static void MlmeIndication(MlmeIndication_t *mlmeIndication)
{
	LOG_DBG("Received MlmeIndication %d", mlmeIndication->MlmeIndication);
	last_mlme_indication_status = mlmeIndication->Status;
}

static LoRaMacStatus_t lorawan_join_otaa(
			const struct lorawan_join_config *join_cfg)
{
	MlmeReq_t mlme_req;
	MibRequestConfirm_t mib_req;

	mlme_req.Type = MLME_JOIN;
	mlme_req.Req.Join.Datarate = lorawan_datarate;

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
	mib_req.Param.JoinEui = join_cfg->otaa.app_key;
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

int lorawan_join(const struct lorawan_join_config *join_cfg)
{
	LoRaMacStatus_t status;
	int ret = 0;

	k_mutex_lock(&lorawan_join_mutex, K_FOREVER);

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
	k_mutex_unlock(&lorawan_join_mutex);
	return ret;
}

int lorawan_set_class(enum lorawan_class dev_class)
{
	LoRaMacStatus_t status;
	MibRequestConfirm_t mib_req;

	mib_req.Type = MIB_DEVICE_CLASS;

	switch (dev_class) {
	case LORAWAN_CLASS_A:
		mib_req.Param.Class = CLASS_A;
		break;
	case LORAWAN_CLASS_B:
	case LORAWAN_CLASS_C:
		LOG_ERR("Device class not supported yet!");
		return -ENOTSUP;
	default:
		return -EINVAL;
	};

	status = LoRaMacMibSetRequestConfirm(&mib_req);
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("Failed to set device class: %s",
			lorawan_status2str(status));
		return lorawan_status2errno(status);
	}

	return 0;
}

int lorawan_set_datarate(enum lorawan_datarate dr)
{
	/* Bail out if using ADR */
	if (lorawan_adr_enable) {
		return -EINVAL;
	}

	lorawan_datarate = dr;

	return 0;
}

void lorawan_enable_adr(bool enable)
{
	MibRequestConfirm_t mib_req;

	if (enable != lorawan_adr_enable) {
		lorawan_adr_enable = enable;

		mib_req.Type = MIB_ADR;
		mib_req.Param.AdrEnable = lorawan_adr_enable;
		LoRaMacMibSetRequestConfirm(&mib_req);
	}
}

int lorawan_set_conf_msg_tries(uint8_t tries)
{
	lorawan_conf_msg_tries = tries;

	return 0;
}

int lorawan_send(uint8_t port, uint8_t *data, uint8_t len, uint8_t flags)
{
	LoRaMacStatus_t status;
	McpsReq_t mcpsReq;
	LoRaMacTxInfo_t txInfo;
	int ret = 0;
	bool empty_frame = false;

	if (data == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&lorawan_send_mutex, K_FOREVER);

	status = LoRaMacQueryTxPossible(len, &txInfo);
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
		mcpsReq.Type = MCPS_UNCONFIRMED;
		mcpsReq.Req.Unconfirmed.fBuffer = NULL;
		mcpsReq.Req.Unconfirmed.fBufferSize = 0;
		mcpsReq.Req.Unconfirmed.Datarate = DR_0;
	} else {
		if (flags & LORAWAN_MSG_CONFIRMED) {
			mcpsReq.Type = MCPS_CONFIRMED;
			mcpsReq.Req.Confirmed.fPort = port;
			mcpsReq.Req.Confirmed.fBuffer = data;
			mcpsReq.Req.Confirmed.fBufferSize = len;
			mcpsReq.Req.Confirmed.NbTrials = lorawan_conf_msg_tries;
			mcpsReq.Req.Confirmed.Datarate = lorawan_datarate;
		} else {
			/* default message type */
			mcpsReq.Type = MCPS_UNCONFIRMED;
			mcpsReq.Req.Unconfirmed.fPort = port;
			mcpsReq.Req.Unconfirmed.fBuffer = data;
			mcpsReq.Req.Unconfirmed.fBufferSize = len;
			mcpsReq.Req.Unconfirmed.Datarate = lorawan_datarate;
		}
	}

	status = LoRaMacMcpsRequest(&mcpsReq);
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("LoRaWAN Send failed: %s", lorawan_status2str(status));
		ret = lorawan_status2errno(status);
		goto out;
	}

	/*
	 * Indicate to the application that the current packet is not sent and
	 * it has to resend the packet.
	 */
	if (empty_frame) {
		ret = -EAGAIN;
		goto out;
	}

	/* Wait for send confirmation */
	if (flags & LORAWAN_MSG_CONFIRMED) {
		/*
		 * We can be sure that the semaphore will be released for
		 * both success and failure cases after a specific time period.
		 * So we can use K_FOREVER and no need to check the return val.
		 */
		k_sem_take(&mcps_confirm_sem, K_FOREVER);

		if (last_mcps_confirm_status != LORAMAC_EVENT_INFO_STATUS_OK) {
			ret = lorawan_eventinfo2errno(last_mcps_confirm_status);
		}
	}

out:
	k_mutex_unlock(&lorawan_send_mutex);
	return ret;
}

int lorawan_start(void)
{
	LoRaMacStatus_t status;
	MibRequestConfirm_t mib_req;

	status = LoRaMacStart();
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("Failed to start the LoRaMAC stack: %s",
			lorawan_status2str(status));
		return -EINVAL;
	}

	/* TODO: Move these to a proper location */
	mib_req.Type = MIB_SYSTEM_MAX_RX_ERROR;
	mib_req.Param.SystemMaxRxError = CONFIG_LORAWAN_SYSTEM_MAX_RX_ERROR;
	LoRaMacMibSetRequestConfirm(&mib_req);

	mib_req.Type = MIB_PUBLIC_NETWORK;
	mib_req.Param.EnablePublicNetwork = true;
	LoRaMacMibSetRequestConfirm(&mib_req);

	return 0;
}

static int lorawan_init(const struct device *dev)
{
	LoRaMacStatus_t status;

	macPrimitives.MacMcpsConfirm = McpsConfirm;
	macPrimitives.MacMcpsIndication = McpsIndication;
	macPrimitives.MacMlmeConfirm = MlmeConfirm;
	macPrimitives.MacMlmeIndication = MlmeIndication;
	macCallbacks.GetBatteryLevel = NULL;
	macCallbacks.GetTemperatureLevel = NULL;
	macCallbacks.NvmContextChange = NULL;
	macCallbacks.MacProcessNotify = OnMacProcessNotify;

	status = LoRaMacInitialization(&macPrimitives, &macCallbacks,
				       LORAWAN_REGION);
	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("LoRaMacInitialization failed: %s",
			lorawan_status2str(status));
		return -EINVAL;
	}

	LOG_DBG("LoRaMAC Initialized");

	return 0;
}

SYS_INIT(lorawan_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
