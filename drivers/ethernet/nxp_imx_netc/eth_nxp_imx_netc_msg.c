/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NXP NETC VSI-to-PSI mailbox messaging.
 *
 * A VSI cannot write the PSI's shared MAC/filter/link registers, so it requests
 * those operations from the PSI owner over the VSI-to-PSI message channel. This
 * file owns the mailbox transport and the per-operation encoders; the VSI
 * driver keeps the interface/lifecycle policy.
 */

#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(nxp_imx_eth_vsi);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/crc.h>

#include "eth_nxp_imx_netc_priv.h"
#include "eth_nxp_imx_netc_msg.h"
#include "fsl_netc_msg.h"

#define NETC_MSG_RESP_CLASS(r) ((uint16_t)(r) >> 8)
#define NETC_MSG_RESP_CODE(r)  (((uint16_t)(r) >> 4) & 0x0fU)
#define NETC_MSG_VSI_ALIGN     32U

/* Time to poll for the PSI to consume a VSI->PSI message before giving up. */
#define NETC_MSG_VSI_TIMEOUT_MS 100

/* MAC/VLAN filter message fields (match the ENETC VSI->PSI mailbox ABI). */
#define NETC_MAC_HASH_TABLE_SIZE_64 0U
#if defined(CONFIG_NET_VLAN)
#define NETC_VLAN_HASH_TABLE_SIZE_64 0U
#endif

/*
 * Send a control message to the PSI and wait for its reply.
 * Returns 0 if the expected reply is received, otherwise an error.
 */
static int netc_vsi_msg_send(const struct device *dev, uint8_t class_id, uint8_t cmd_id,
			     const void *payload, size_t payload_len, uint8_t want_class,
			     uint16_t *resp)
{
	struct netc_eth_data *data = dev->data;
	ep_handle_t *handle = &data->handle;
	netc_msg_header_t *hdr = (netc_msg_header_t *)data->msg_buff;
	size_t msg_len = ROUND_UP(sizeof(*hdr) + payload_len, NETC_MSG_VSI_ALIGN);
	netc_vsi_msg_tx_status_t status;
	k_timepoint_t deadline;
	uint16_t crc;
	int ret;

	if (msg_len > NETC_MSG_VSI_BUF_SIZE) {
		return -EINVAL;
	}

	/* Serialize access to the shared msg_buff and the single SI mailbox. */
	k_mutex_lock(&data->msg_lock, K_FOREVER);

	(void)memset(data->msg_buff, 0, msg_len);
	hdr->classId = class_id;
	hdr->cmdId = cmd_id;
	hdr->len = (uint8_t)(msg_len / NETC_MSG_VSI_ALIGN - 1U);
	if (payload_len != 0U) {
		(void)memcpy(data->msg_buff + sizeof(*hdr), payload, payload_len);
	}

	crc = crc16_itu_t(0xFFFFU, data->msg_buff + 2, msg_len - 2U);
	data->msg_buff[0] = (uint8_t)(crc >> 8);
	data->msg_buff[1] = (uint8_t)crc;

	if (EP_VsiSendMsg(handle, (uint64_t)(uintptr_t)data->msg_buff, msg_len) !=
	    kStatus_Success) {
		ret = -EBUSY;
		goto out;
	}

	deadline = sys_timepoint_calc(K_MSEC(NETC_MSG_VSI_TIMEOUT_MS));
	do {
		NETC_SIVsiCheckTxStatus(handle->hw.si, &status);
		if (!status.txBusy) {
			break;
		}
		if (sys_timepoint_expired(deadline)) {
			LOG_DBG("VSI msg (class 0x%02x) timed out; PSI not servicing messages?",
				class_id);
			ret = -ETIMEDOUT;
			goto out;
		}
		k_msleep(1);
	} while (true);

	if (status.isTxErr) {
		LOG_DBG("VSI msg (class 0x%02x) send error, status 0x%04x", class_id,
			status.msgCode);
		ret = -EIO;
		goto out;
	}

	*resp = status.msgCode;
	ret = (NETC_MSG_RESP_CLASS(*resp) == want_class) ? 0 : -EBADMSG;

out:
	k_mutex_unlock(&data->msg_lock);
	return ret;
}

int netc_vsi_msg_set_primary_mac(const struct device *dev)
{
	struct netc_eth_data *data = dev->data;
	uint16_t resp;
	int ret;
	struct {
		uint8_t count;
		uint8_t reserved[3];
		uint8_t mac[6];
	} req = {0};

	(void)memcpy(req.mac, data->mac_addr, sizeof(req.mac));

	ret = netc_vsi_msg_send(dev, (uint8_t)kNETC_MsgClassMacFilter,
				(uint8_t)kNETC_MsgMacFilterSetMacAddr, &req, sizeof(req),
				(uint8_t)kNETC_MsgClassDone, &resp);
	if (ret == -EBADMSG) {
		LOG_DBG("VSI MAC-filter request rejected by PSI: code 0x%04x", resp);
		return -EIO;
	}

	return ret;
}

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
int netc_vsi_msg_set_mac_promisc(const struct device *dev, uint8_t type_mask, bool enable)
{
	uint16_t resp;
	int ret;
	struct {
		uint8_t flush_macs: 1;
		uint8_t promisc_mode: 1;
		uint8_t reserved: 4;
		uint8_t type: 2;
	} req = {
		.promisc_mode = enable ? 1U : 0U,
		.type = type_mask,
	};

	ret = netc_vsi_msg_send(dev, (uint8_t)kNETC_MsgClassMacFilter,
				(uint8_t)kNETC_MsgMacFilterSetMacPromisc, &req, sizeof(req),
				(uint8_t)kNETC_MsgClassDone, &resp);
	if (ret == -EBADMSG) {
		LOG_DBG("VSI promisc request rejected by PSI: code 0x%04x", resp);
		return -EIO;
	}

	return ret;
}
#endif /* CONFIG_NET_PROMISCUOUS_MODE */

int netc_vsi_msg_set_mac_hash(const struct device *dev)
{
	struct netc_eth_data *data = dev->data;
	uint16_t resp;
	int ret;
	struct {
		uint8_t size_type;
		uint8_t reserved[7];
		uint64_t hash_tbl;
	} req = {
		.size_type = (uint8_t)((NETC_MAC_HASH_TABLE_SIZE_64 & 0x3fU) |
				       (uint8_t)(NETC_MAC_FILTER_TYPE_MC << 6)),
		.hash_tbl = data->mc_hash,
	};

	ret = netc_vsi_msg_send(dev, (uint8_t)kNETC_MsgClassMacFilter,
				(uint8_t)kNETC_MsgMacFilterSet, &req, sizeof(req),
				(uint8_t)kNETC_MsgClassDone, &resp);
	if (ret == -EBADMSG) {
		LOG_DBG("VSI multicast hash update rejected by PSI: code 0x%04x", resp);
		return -EIO;
	}

	return ret;
}

#if defined(CONFIG_NET_VLAN)
int netc_vsi_msg_set_vlan_hash(const struct device *dev)
{
	struct netc_eth_data *data = dev->data;
	uint16_t resp;
	int ret;
	struct {
		uint8_t size;
		uint8_t reserved[7];
		uint64_t hash_tbl;
	} req = {
		.size = NETC_VLAN_HASH_TABLE_SIZE_64,
		.hash_tbl = data->vlan_hash,
	};

	ret = netc_vsi_msg_send(dev, (uint8_t)kNETC_MsgClassVlanFilter,
				(uint8_t)kNETC_MsgVlanFilterSet, &req, sizeof(req),
				(uint8_t)kNETC_MsgClassDone, &resp);
	if (ret == -EBADMSG) {
		LOG_DBG("VSI VLAN hash update rejected by PSI: code 0x%04x", resp);
		return -EIO;
	}

	return ret;
}
#endif /* CONFIG_NET_VLAN */

int netc_vsi_msg_enable_link_notify(const struct device *dev)
{
	uint16_t resp;
	int ret;

	ret = netc_vsi_msg_send(dev, (uint8_t)kNETC_MsgClassLinkStatus,
				(uint8_t)kNETC_MsgLinkStatusEnableNotify, NULL, 0,
				(uint8_t)kNETC_MsgClassDone, &resp);
	if (ret == -EBADMSG) {
		LOG_WRN("VSI link-notify registration rejected by PSI: code 0x%04x", resp);
		return -EIO;
	}

	return ret;
}

enum netc_msg_link netc_vsi_msg_poll_link(const struct device *dev)
{
	struct netc_eth_data *data = dev->data;
	ep_handle_t *handle = &data->handle;
	enum netc_msg_link link = NETC_MSG_LINK_NONE;
	uint16_t msg = 0;

	if ((EP_VsiGetStatus(handle) & (uint32_t)kNETC_VsiMsgRxFlag) == 0U) {
		return NETC_MSG_LINK_NONE;
	}

	if (EP_VsiReceiveMsg(handle, &msg) == kStatus_Success &&
	    NETC_MSG_RESP_CLASS(msg) == (uint16_t)kNETC_MsgClassLinkStatus) {
		link = (NETC_MSG_RESP_CODE(msg) == (uint16_t)kNETC_MsgLinkStatusUp)
			       ? NETC_MSG_LINK_UP
			       : NETC_MSG_LINK_DOWN;
	}

	EP_VsiClearStatus(handle, (uint32_t)kNETC_VsiMsgRxFlag);

	return link;
}
