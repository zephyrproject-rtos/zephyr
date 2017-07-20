/*
 * Copyright (c) 2017 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "ieee802154_fsci"
#include <logging/sys_log.h>

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <device.h>
#include <drivers/console/uart_pipe.h>
#include <init.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/ieee802154_radio.h>

#include "../../subsys/net/ip/l2/ieee802154/ieee802154_frame.h"

#define gFsciIncluded_c			1
#define gFsciHost_802_15_4_c		1
#define gMacUsePackedStructs_d		1
#define gPHY_802_15_4g_d		0

#include "MacTypes.h"
#include "FsciAspCommands.h"
#include "FsciCommands.h"
#include "FsciInterface.h"
#include "FsciMacCommands.h"

#define FSCI_TX_POWER_MIN		(-19)

enum {
	FSCI_RX_STATE_SYNC,
	FSCI_RX_STATE_OG,
	FSCI_RX_STATE_OC,
	FSCI_RX_STATE_LEN,
	FSCI_RX_STATE_PAYLOAD,
	FSCI_RX_STATE_CRC,
};

/* Lookup table for TX_POWER */
static const u8_t tx_power_lt[22] = {
	2, 2, 2, 2, 2, 2,	/* -19:-14 dBm */
	4, 4, 4,		/* -13:-11 dBm */
	6, 6, 6,		/* -10:-8 dBm */
	8, 8,			/* -7:-6 dBm */
	10, 10,			/* -5:-4 dBm */
	12,			/* -3 dBm */
	14, 14,			/* -2:-1 dBm */
	18, 18,			/* 0:1 dBm */
	24			/* 2 dBm */
};

struct fsci_context {
	struct net_if *iface;
	struct k_sem req_sem;
	struct k_mutex req_mutex;
	bool req_started;

	u8_t mac_addr[8];
	u8_t lqi;
	u16_t pan_id;
	u8_t channel;

	u8_t pipe_buf[1];
	u8_t rx_state;
	u8_t rx_off;
	u8_t rx_buf[gFsciMaxPayloadLen_c];
	clientPacket_t fsci_pkt;
};

static struct fsci_context fsci_context_data;

static void fsci_rx_data(void)
{
	struct fsci_context *fsci = &fsci_context_data;
	mcpsDataInd_t *data = (mcpsDataInd_t *)fsci_context_data.rx_buf;
	struct net_pkt *pkt = NULL;
	struct net_buf *frag = NULL;
	struct ieee802154_address_field *addr_field;
	struct ieee802154_fcf_seq *fs;
	u8_t *msdu;
	u8_t *buf;

	fsci_context_data.lqi = data->mpduLinkQuality;

	pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
	if (!pkt) {
		SYS_LOG_ERR("No buf available");
		goto out;
	}

	frag = net_pkt_get_frag(pkt, K_NO_WAIT);
	if (!frag) {
		SYS_LOG_ERR("No frag available");
		goto out;
	}

	net_pkt_frag_insert(pkt, frag);

	buf = frag->data;
	fs = (struct ieee802154_fcf_seq *)buf;
	memset(fs, 0, sizeof(struct ieee802154_fcf_seq));
	buf += sizeof(struct ieee802154_fcf_seq);

	fs->fc.frame_type = IEEE802154_FRAME_TYPE_DATA;
	fs->fc.frame_version = IEEE802154_VERSION_802154_2006;
	fs->fc.pan_id_comp = 1;
	fs->sequence = data->dsn;

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (data->securityLevel != gMacSecurityNone_c) {
		SYS_LOG_ERR("rx: MAC security is not supported");
		goto out;
	}
#endif

	addr_field = (struct ieee802154_address_field *)buf;
	fs->fc.dst_addr_mode = data->dstAddrMode;
	addr_field->plain.pan_id = data->dstPanId;
	buf += IEEE802154_PAN_ID_LENGTH;

	if (data->dstAddrMode == gAddrModeShortAddress_c) {
		addr_field->plain.addr.short_addr = data->dstAddr;
		buf += IEEE802154_SHORT_ADDR_LENGTH;
	} else { /* gAddrModeExtendedAddress_c */
		memcpy(addr_field->plain.addr.ext_addr, (u8_t *)&data->dstAddr,
		       IEEE802154_EXT_ADDR_LENGTH);
		buf += IEEE802154_EXT_ADDR_LENGTH;
	}

	addr_field = (struct ieee802154_address_field *)buf;
	fs->fc.src_addr_mode = data->srcAddrMode;

	if (!fs->fc.pan_id_comp) {
		addr_field->plain.pan_id = data->srcPanId;
		buf += IEEE802154_PAN_ID_LENGTH;
	}

	if (data->srcAddrMode == gAddrModeShortAddress_c) {
		if (fs->fc.pan_id_comp) {
			addr_field->comp.addr.short_addr = data->srcAddr;
		} else {
			addr_field->plain.addr.short_addr = data->srcAddr;
		}

		buf += IEEE802154_SHORT_ADDR_LENGTH;
	} else { /* gAddrModeExtendedAddress_c */
		if (fs->fc.pan_id_comp) {
			memcpy(addr_field->comp.addr.ext_addr,
			       (u8_t *)&data->srcAddr,
			       IEEE802154_EXT_ADDR_LENGTH);
		} else {
			memcpy(addr_field->plain.addr.ext_addr,
			       (u8_t *)&data->srcAddr,
			       IEEE802154_EXT_ADDR_LENGTH);
		}

		buf += IEEE802154_EXT_ADDR_LENGTH;
	}

	msdu = fsci_context_data.rx_buf + sizeof(mcpsDataInd_t) -
	       sizeof(u8_t *);
	memcpy(buf, msdu, data->msduLength);
	buf += data->msduLength;

	net_buf_add(frag, (u8_t)(buf - frag->data));

	if (ieee802154_radio_handle_ack(fsci->iface, pkt) == NET_OK) {
		SYS_LOG_DBG("ACK packet handled");
		goto out;
	}

	if (net_recv_data(fsci->iface, pkt) < 0) {
		SYS_LOG_DBG("Packet dropped by NET stack");
		goto out;
	}

	return;
out:
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

static void fsci_rx_resp(void)
{
	memcpy(fsci_context_data.fsci_pkt.structured.payload,
	       fsci_context_data.rx_buf,
	       fsci_context_data.fsci_pkt.structured.header.len);

	fsci_context_data.req_started = false;
	k_sem_give(&fsci_context_data.req_sem);
}

static u8_t *fsci_rx(u8_t *buf, size_t *off)
{
	clientPacketStructured_t *pkt =
		 &fsci_context_data.fsci_pkt.structured;

	switch (fsci_context_data.rx_state) {
	case FSCI_RX_STATE_SYNC:
		if (*buf == 0x02) {
			pkt->header.startMarker = *buf;
			fsci_context_data.rx_state = FSCI_RX_STATE_OG;
		}

		break;
	case FSCI_RX_STATE_OG:
		pkt->header.opGroup = *buf;
		fsci_context_data.rx_state = FSCI_RX_STATE_OC;
		break;
	case FSCI_RX_STATE_OC:
		pkt->header.opCode = *buf;
		fsci_context_data.rx_state = FSCI_RX_STATE_LEN;
		break;
	case FSCI_RX_STATE_LEN:
		if (*buf > gFsciMaxPayloadLen_c) {
			SYS_LOG_ERR("Payload buffer size not big enough! "
				    " Needed size is %d", *buf);
		}

		pkt->header.len = *buf;
		fsci_context_data.rx_off = 0;
		fsci_context_data.rx_state = FSCI_RX_STATE_PAYLOAD;

		break;
	case FSCI_RX_STATE_PAYLOAD:
		if (fsci_context_data.rx_off < gFsciMaxPayloadLen_c) {
			fsci_context_data.rx_buf[
				fsci_context_data.rx_off] = *buf;
		}

		fsci_context_data.rx_off++;

		if (fsci_context_data.rx_off == pkt->header.len) {
			fsci_context_data.rx_state = FSCI_RX_STATE_CRC;
		}

		break;
	case FSCI_RX_STATE_CRC:
		pkt->payload[pkt->header.len] = *buf;

		if (pkt->header.opGroup == gFSCI_McpsNwkOpcodeGroup_c &&
		    pkt->header.opCode == mFsciMcpsNwkDataInd_c) {
			fsci_rx_data();
		} else {
			if (!fsci_context_data.req_started) {
				SYS_LOG_ERR("Unexpected packet type: "
					    "og=0x%02X oc=0x%02X",
					    pkt->header.opGroup,
					    pkt->header.opCode);
			}

			fsci_rx_resp();
		}

		fsci_context_data.rx_state = FSCI_RX_STATE_SYNC;
		break;
	}

	*off = 0;
	return buf;
}

static void fsci_fill_crc(clientPacket_t *pkt)
{
	u8_t crc, i;

	crc = pkt->structured.header.opGroup ^
	      pkt->structured.header.opCode ^
	      pkt->structured.header.len;

	for (i = 0; i < pkt->structured.header.len; i++) {
		crc ^= pkt->structured.payload[i];
	}

	pkt->structured.payload[pkt->structured.header.len] = crc;
}

static clientPacket_t *fsci_send_payload(clientPacket_t *pkt)
{
	pkt->structured.header.startMarker = 0x02;
	fsci_fill_crc(pkt);

	k_mutex_lock(&fsci_context_data.req_mutex, K_FOREVER);

	fsci_context_data.req_started = true;

	uart_pipe_send((u8_t *)&pkt->structured.header,
		       sizeof(clientPacketHdr_t));
	uart_pipe_send(pkt->structured.payload,
		       pkt->structured.header.len + 1);

	k_sem_take(&fsci_context_data.req_sem, K_FOREVER);

	k_mutex_unlock(&fsci_context_data.req_mutex);

	return &fsci_context_data.fsci_pkt;
}

static resultType_t fsci_mac_reset(bool reset_pib)
{
	clientPacket_t req;
	clientPacket_t *resp;
	mlmeResetReq_t *payload;

	req.structured.header.opGroup = gFSCI_NwkMlmeOpcodeGroup_c;
	req.structured.header.opCode = mFsciNwkMlmeResetReq_c;
	req.structured.header.len = sizeof(mlmeResetReq_t);

	payload = (mlmeResetReq_t *)req.structured.payload;
	payload->setDefaultPIB = reset_pib;

	resp = fsci_send_payload(&req);

	return resp->headerAndStatus.status;
}

static resultType_t fsci_mac_start(void)
{
	clientPacket_t req;
	clientPacket_t *resp;
	mlmeStartReq_t *payload;

	req.structured.header.opGroup = gFSCI_NwkMlmeOpcodeGroup_c;
	req.structured.header.opCode = mFsciNwkMlmeStartReq_c;
	req.structured.header.len = sizeof(mlmeStartReq_t);

	payload = (mlmeStartReq_t *)req.structured.payload;
	memset(payload, 0, sizeof(mlmeStartReq_t));
	UNALIGNED_PUT(fsci_context_data.pan_id, &payload->panId);
	payload->logicalChannel = fsci_context_data.channel;
	payload->beaconOrder = 0x0F;
	payload->superframeOrder = 0x0F;
	payload->panCoordinator = 0x01;

	resp = fsci_send_payload(&req);

	return resp->headerAndStatus.status;
}

static resultType_t fsci_set_pib_attribute(pibId_t attr_id, u8_t *val, u8_t len)
{
	clientPacket_t req;
	clientPacket_t *resp;
	mlmeSetReq_t *payload;
	u8_t *attr_value;
	u8_t i;

	req.structured.header.opGroup = gFSCI_NwkMlmeOpcodeGroup_c;
	req.structured.header.opCode = mFsciNwkMlmeSetReq_c;
	req.structured.header.len = 2 + len;

	payload = (mlmeSetReq_t *)req.structured.payload;
	payload->pibAttribute = attr_id;
	payload->pibAttributeIndex = 0;

	attr_value = (u8_t *)&payload->pibAttributeValue;
	for (i = 0; i < len; i++) {
		attr_value[i] = val[i];
	}

	resp = fsci_send_payload(&req);

	return resp->headerAndStatus.status;
}

static resultType_t fsci_set_short_addr_helper(u16_t short_addr)
{
	return fsci_set_pib_attribute(gMPibShortAddress_c, (u8_t *)&short_addr,
				      IEEE802154_SHORT_ADDR_LENGTH);
}

static resultType_t fsci_set_channel_helper(u8_t channel)
{
	return fsci_set_pib_attribute(gMPibLogicalChannel_c, &channel,
				      sizeof(u8_t));
}

static resultType_t fsci_set_pan_id_helper(u16_t pan_id)
{
	return fsci_set_pib_attribute(gMPibPanId_c, (u8_t *)&pan_id,
				      IEEE802154_PAN_ID_LENGTH);
}

static resultType_t fsci_set_ext_addr(const u8_t *ext_addr)
{
	clientPacket_t req;
	clientPacket_t *resp;

	req.structured.header.opGroup = gFSCI_ReqOpcodeGroup_c;
	req.structured.header.opCode = mFsciMsgWriteExtendedAdrReq_c;
	req.structured.header.len = IEEE802154_EXT_ADDR_LENGTH;
	memcpy(req.structured.payload, ext_addr, IEEE802154_EXT_ADDR_LENGTH);

	resp = fsci_send_payload(&req);

	return resp->headerAndStatus.status;
}

static resultType_t fsci_set_power_level(s16_t power_level)
{
	clientPacket_t req;
	clientPacket_t *resp;
	aspSetPowerLevelReq_t *payload;

	req.structured.header.opGroup = gFSCI_AppAspOpcodeGroup_c;
	req.structured.header.opCode = aspMsgTypeSetPowerLevel_c;
	req.structured.header.len = sizeof(aspSetPowerLevelReq_t);

	payload = (aspSetPowerLevelReq_t *)req.structured.payload;
	payload->powerLevel = power_level;

	resp = fsci_send_payload(&req);

	return resp->headerAndStatus.status;
}

static resultType_t fsci_mac_data_tx(u8_t *payload, u8_t len)
{
	clientPacket_t req;
	clientPacket_t *resp;
	mcpsDataReq_t *data;
	struct ieee802154_address_field *addr_field;
	struct ieee802154_address *addr;
	struct ieee802154_fcf_seq *fs;
	static u8_t handle;
	u8_t *buf;

	req.structured.header.opGroup = gFSCI_NwkMcpsOpcodeGroup_c;
	req.structured.header.opCode = mFsciNwkMcpsDataReq_c;

	data = (mcpsDataReq_t *)req.structured.payload;
	memset(data, 0, sizeof(mcpsDataReq_t));

	data->msduHandle = handle++;

	fs = (struct ieee802154_fcf_seq *)payload;
	buf = payload + sizeof(struct ieee802154_fcf_seq);

	if (fs->fc.frame_type != IEEE802154_FRAME_TYPE_DATA) {
		SYS_LOG_ERR("tx: unsupported frame type %d", fs->fc.frame_type);
		return gInvalidParameter_c;
	}

	data->txOptions = gMacTxOptionsNone_c;

	if (fs->fc.frame_pending) {
		data->txOptions |= gMacTxOptionFramePending_c;
	}

	if (fs->fc.ar) {
		data->txOptions |= gMacTxOptionsAck_c;
	}

	addr_field = (struct ieee802154_address_field *)buf;
	addr = (struct ieee802154_address *)&addr_field->plain.addr;
	data->dstAddrMode = fs->fc.dst_addr_mode;
	data->dstPanId = addr_field->plain.pan_id;
	buf += IEEE802154_PAN_ID_LENGTH;

	if (data->dstAddrMode == gAddrModeShortAddress_c) {
		data->dstAddr = addr->short_addr;
		buf += IEEE802154_SHORT_ADDR_LENGTH;
	} else { /* gAddrModeExtendedAddress_c */
		memcpy(&data->dstAddr, addr->ext_addr,
		       IEEE802154_EXT_ADDR_LENGTH);
		buf += IEEE802154_EXT_ADDR_LENGTH;
	}

	addr_field = (struct ieee802154_address_field *)buf;
	data->srcAddrMode = fs->fc.src_addr_mode;

	if (!fs->fc.pan_id_comp) {
		data->srcPanId = addr_field->plain.pan_id;
		buf += IEEE802154_PAN_ID_LENGTH;
		addr = (struct ieee802154_address *)&addr_field->plain.addr;
	} else {
		addr = (struct ieee802154_address *)&addr_field->comp.addr;
	}

	if (data->srcAddrMode == gAddrModeShortAddress_c) {
		data->srcAddr = addr->short_addr;
		buf += IEEE802154_SHORT_ADDR_LENGTH;
	} else { /* gAddrModeExtendedAddress_c */
		memcpy(&data->srcAddr, addr->ext_addr,
		       IEEE802154_EXT_ADDR_LENGTH);
		buf += IEEE802154_EXT_ADDR_LENGTH;
	}

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (fs->fc.security_enabled) {
		SYS_LOG_ERR("tx: MAC security is not supported");
		return gInvalidParameter_c;
	}
#endif

	data->msduLength = len - (u8_t)(buf - payload);
	req.structured.header.len = data->msduLength + sizeof(mcpsDataReq_t) -
				    sizeof(u8_t *);
	memcpy(req.structured.payload + sizeof(mcpsDataReq_t) - sizeof(u8_t *),
	       buf, data->msduLength);

	resp = fsci_send_payload(&req);

	return resp->headerAndStatus.status;
}

static int fsci_cca(struct device *dev)
{
	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	ARG_UNUSED(dev);

	/* No CCA command, fsci_tx will return busy if needed */
	return 0;
}

static int fsci_set_channel(struct device *dev, u16_t channel)
{
	struct fsci_context *fsci = dev->driver_data;

	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	if (channel == fsci->channel) {
		return 0;
	}

	fsci->channel = channel;

	return fsci_set_channel_helper(channel) != gSuccess_c ? -EIO : 0;
}

static int fsci_set_pan_id(struct device *dev, u16_t pan_id)
{
	struct fsci_context *fsci = dev->driver_data;

	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	if (pan_id == fsci->pan_id) {
		return 0;
	}

	fsci->pan_id = pan_id;

	return fsci_set_pan_id_helper(pan_id) != gSuccess_c ? -EIO : 0;
}

static int fsci_set_short_addr(struct device *dev, u16_t short_addr)
{
	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	ARG_UNUSED(dev);

	return fsci_set_short_addr_helper(short_addr) != gSuccess_c ? -EIO : 0;
}

static int fsci_set_ieee_addr(struct device *dev, const u8_t *ieee_addr)
{
	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	ARG_UNUSED(dev);

	return fsci_set_ext_addr(ieee_addr) != gSuccess_c ? -EIO : 0;
}

static int fsci_set_txpower(struct device *dev, s16_t dbm)
{
	u8_t power_level;

	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	ARG_UNUSED(dev);

	power_level = tx_power_lt[dbm - FSCI_TX_POWER_MIN];

	return fsci_set_power_level(power_level) != gSuccess_c ? -EIO : 0;
}

static u8_t fsci_get_lqi(struct device *dev)
{
	struct fsci_context *fsci = dev->driver_data;

	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	return fsci->lqi;
}

static int fsci_tx(struct device *dev,
		   struct net_pkt *pkt,
		   struct net_buf *frag)
{
	u8_t payload_len = net_pkt_ll_reserve(pkt) + frag->len;
	u8_t *payload = frag->data - net_pkt_ll_reserve(pkt);

	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	ARG_UNUSED(dev);

	return fsci_mac_data_tx(payload, payload_len) != gSuccess_c ? -EIO : 0;
}

static int fsci_start(struct device *dev)
{
	resultType_t ret;

	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	ARG_UNUSED(dev);

	ret = fsci_mac_start();
	if (ret == gNoShortAddress_c) {
		/* MAC will not start if ShortAddress is not set */
		if (fsci_set_short_addr(dev, 0x0000) != 0) {
			return -EIO;
		}

		ret = fsci_mac_start();
	}

	return ret != gSuccess_c ? -EIO : 0;
}

static int fsci_stop(struct device *dev)
{
	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	ARG_UNUSED(dev);

	/* No command for stop, so using reset */
	return fsci_mac_reset(false) != gSuccess_c ? -EIO : 0;
}

static int fsci_init(struct device *dev)
{
	struct fsci_context *fsci = dev->driver_data;

	SYS_LOG_DBG("%s:%d", __func__, __LINE__);

	k_sem_init(&fsci->req_sem, 0, 1);
	k_mutex_init(&fsci->req_mutex);
	fsci->req_started = false;

	fsci->pan_id = 0;
	fsci->channel = gLogicalChannel26_c;
	fsci->rx_state = FSCI_RX_STATE_SYNC;
	uart_pipe_register(fsci->pipe_buf, 1, fsci_rx);

	return fsci_mac_reset(true) != gSuccess_c ? -EIO : 0;
}

static inline u8_t *get_mac(struct device *dev)
{
	struct fsci_context *fsci = dev->driver_data;
	u32_t *ptr = (u32_t *)(fsci->mac_addr);

	UNALIGNED_PUT(sys_rand32_get(), ptr);
	ptr = (u32_t *)(fsci->mac_addr + 4);
	UNALIGNED_PUT(sys_rand32_get(), ptr);

	fsci->mac_addr[0] = (fsci->mac_addr[0] & ~0x01) | 0x02;

	return fsci->mac_addr;
}

static void fsci_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct fsci_context *fsci = dev->driver_data;
	u8_t *mac = get_mac(dev);

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);
	fsci->iface = iface;
	ieee802154_init(iface);
}

static struct ieee802154_radio_api fsci_radio_api = {
	.iface_api.init		= fsci_iface_init,
	.iface_api.send		= ieee802154_radio_send,

	.cca			= fsci_cca,
	.set_channel		= fsci_set_channel,
	.set_pan_id		= fsci_set_pan_id,
	.set_short_addr		= fsci_set_short_addr,
	.set_ieee_addr		= fsci_set_ieee_addr,
	.set_txpower		= fsci_set_txpower,
	.tx			= fsci_tx,
	.start			= fsci_start,
	.stop			= fsci_stop,
	.get_lqi		= fsci_get_lqi,
};

NET_DEVICE_INIT(fsci_802_15_4, CONFIG_IEEE802154_FSCI_DRV_NAME,
		fsci_init, &fsci_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&fsci_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);
