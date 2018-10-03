/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_IEEE802154)
#define SYS_LOG_DOMAIN "net/ieee802154"
#define NET_LOG_ENABLED 1
#endif

#include <net/net_core.h>

#include <errno.h>

#include <net/net_if.h>
#include <net/ieee802154_radio.h>
#include <net/ieee802154_mgmt.h>
#include <net/ieee802154.h>

#include "ieee802154_frame.h"
#include "ieee802154_mgmt_priv.h"
#include "ieee802154_security.h"
#include "ieee802154_utils.h"

enum net_verdict ieee802154_handle_beacon(struct net_if *iface,
					  struct ieee802154_mpdu *mpdu,
					  u8_t lqi)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	NET_DBG("Beacon received");

	if (!ctx->scan_ctx) {
		return NET_DROP;
	}

	if (!mpdu->beacon->sf.association) {
		return NET_DROP;
	}

	k_sem_take(&ctx->res_lock, K_FOREVER);

	ctx->scan_ctx->pan_id = mpdu->mhr.src_addr->plain.pan_id;
	ctx->scan_ctx->lqi = lqi;

	if (mpdu->mhr.fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
		ctx->scan_ctx->len = IEEE802154_SHORT_ADDR_LENGTH;
		ctx->scan_ctx->short_addr =
			mpdu->mhr.src_addr->plain.addr.short_addr;
	} else {
		ctx->scan_ctx->len = IEEE802154_EXT_ADDR_LENGTH;
		sys_memcpy_swap(ctx->scan_ctx->addr,
				mpdu->mhr.src_addr->plain.addr.ext_addr,
				IEEE802154_EXT_ADDR_LENGTH);
	}

	net_mgmt_event_notify(NET_EVENT_IEEE802154_SCAN_RESULT, iface);

	k_sem_give(&ctx->res_lock);

	return NET_OK;
}

static int ieee802154_cancel_scan(u32_t mgmt_request, struct net_if *iface,
				  void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	ARG_UNUSED(data);
	ARG_UNUSED(len);

	NET_DBG("Cancelling scan request");

	ctx->scan_ctx = NULL;

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_CANCEL_SCAN,
				  ieee802154_cancel_scan);

static int ieee802154_scan(u32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_req_params *scan =
		(struct ieee802154_req_params *)data;
	struct net_pkt *pkt = NULL;
	u8_t channel;
	int ret;

	NET_DBG("%s scan requested",
		mgmt_request == NET_REQUEST_IEEE802154_ACTIVE_SCAN ?
		"Active" : "Passive");

	if (ctx->scan_ctx) {
		return -EALREADY;
	}

	if (mgmt_request == NET_REQUEST_IEEE802154_ACTIVE_SCAN) {
		struct ieee802154_frame_params params;

		params.dst.short_addr = IEEE802154_BROADCAST_ADDRESS;
		params.dst.pan_id = IEEE802154_BROADCAST_PAN_ID;

		pkt = ieee802154_create_mac_cmd_frame(
			ctx, IEEE802154_CFI_BEACON_REQUEST, &params);
		if (!pkt) {
			NET_DBG("Could not create Beacon Request");
			return -ENOBUFS;
		}
	}

	ctx->scan_ctx = scan;
	ret = 0;

	ieee802154_filter_pan_id(iface, IEEE802154_BROADCAST_PAN_ID);

	if (ieee802154_start(iface)) {
		NET_DBG("Could not start device");
		ret = -EIO;

		goto out;
	}

	/* ToDo: For now, we assume we are on 2.4Ghz
	 * (device will have to export capabilities) */
	for (channel = 11; channel <= 26; channel++) {
		if (IEEE802154_IS_CHAN_UNSCANNED(scan->channel_set, channel)) {
			continue;
		}

		scan->channel = channel;
		NET_DBG("Scanning channel %u", channel);
		ieee802154_set_channel(iface, channel);

		/* Active scan sends a beacon request */
		if (mgmt_request == NET_REQUEST_IEEE802154_ACTIVE_SCAN) {
			net_pkt_ref(pkt);
			net_pkt_frag_ref(pkt->frags);

			ret = ieee802154_radio_send(iface, pkt);
			if (ret) {
				NET_DBG("Could not send Beacon Request (%d)",
					ret);
				net_pkt_unref(pkt);

				break;
			}
		}

		/* Context aware sleep */
		k_sleep(scan->duration);

		if (!ctx->scan_ctx) {
			NET_DBG("Scan request cancelled");
			ret = -ECANCELED;

			break;
		}
	}

	/* Let's come back to context's settings */
	ieee802154_filter_pan_id(iface, ctx->pan_id);
	ieee802154_set_channel(iface, ctx->channel);
out:
	ctx->scan_ctx = NULL;

	if (pkt) {
		net_pkt_unref(pkt);
	}

	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_PASSIVE_SCAN,
				  ieee802154_scan);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_ACTIVE_SCAN,
				  ieee802154_scan);

enum net_verdict ieee802154_handle_mac_command(struct net_if *iface,
					       struct ieee802154_mpdu *mpdu)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	if (mpdu->command->cfi == IEEE802154_CFI_ASSOCIATION_RESPONSE) {
		if (mpdu->command->assoc_res.status !=
		    IEEE802154_ASF_SUCCESSFUL) {
			return NET_DROP;
		}

		ctx->associated = true;
		k_sem_give(&ctx->req_lock);

		return NET_OK;
	}

	if (mpdu->command->cfi == IEEE802154_CFI_DISASSOCIATION_NOTIFICATION) {
		if (mpdu->command->disassoc_note.reason !=
		    IEEE802154_DRF_COORDINATOR_WISH) {
			return NET_DROP;
		}

		if (ctx->associated) {
			/* ToDo: check src address vs coord ones and reject
			 * if they don't match
			 */
			ctx->associated = false;

			return NET_OK;
		}
	}

	NET_DBG("Drop MAC command, unsupported CFI: 0x%x",
		mpdu->command->cfi);

	return NET_DROP;
}

static int ieee802154_associate(u32_t mgmt_request, struct net_if *iface,
				void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_req_params *req =
		(struct ieee802154_req_params *)data;
	struct ieee802154_frame_params params;
	struct ieee802154_command *cmd;
	struct net_pkt *pkt;
	int ret = 0;

	k_sem_take(&ctx->req_lock, K_FOREVER);

	params.dst.len = req->len;
	if (params.dst.len == IEEE802154_SHORT_ADDR_LENGTH) {
		params.dst.short_addr = req->short_addr;
	} else {
		params.dst.ext_addr = req->addr;
	}

	params.dst.pan_id = req->pan_id;
	params.pan_id = req->pan_id;

	/* Set channel first */
	if (ieee802154_set_channel(iface, req->channel)) {
		ret = -EIO;
		goto out;
	}

	pkt = ieee802154_create_mac_cmd_frame(
		ctx, IEEE802154_CFI_ASSOCIATION_REQUEST, &params);
	if (!pkt) {
		ret = -ENOBUFS;
		goto out;
	}

	cmd = ieee802154_get_mac_command(pkt);
	cmd->assoc_req.ci.dev_type = 0; /* RFD */
	cmd->assoc_req.ci.power_src = 0; /* ToDo: set right power source */
	cmd->assoc_req.ci.rx_on = 1; /* ToDo: that will depends on PM */
	cmd->assoc_req.ci.sec_capability = 0; /* ToDo: security support */
	cmd->assoc_req.ci.alloc_addr = 0; /* ToDo: handle short addr */

	ctx->associated = false;

	if (net_if_send_data(iface, pkt)) {
		net_pkt_unref(pkt);
		ret = -EIO;
		goto out;
	}

	/* ToDo: current timeout is arbitrary */
	k_sem_take(&ctx->req_lock, K_SECONDS(1));

	if (ctx->associated) {
		ctx->channel = req->channel;
		ctx->pan_id = req->pan_id;

		ctx->coord_addr_len = req->len;
		if (ctx->coord_addr_len == IEEE802154_SHORT_ADDR_LENGTH) {
			ctx->coord.short_addr = req->short_addr;
		} else {
			memcpy(ctx->coord.ext_addr,
			       req->addr, IEEE802154_EXT_ADDR_LENGTH);
		}
	} else {
		ret = -EACCES;
	}

out:
	k_sem_give(&ctx->req_lock);

	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_ASSOCIATE,
				  ieee802154_associate);

static int ieee802154_disassociate(u32_t mgmt_request, struct net_if *iface,
				   void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_frame_params params;
	struct ieee802154_command *cmd;
	struct net_pkt *pkt;

	if (!ctx->associated) {
		return -EALREADY;
	}

	params.dst.pan_id = ctx->pan_id;
	params.dst.len = ctx->coord_addr_len;
	if (params.dst.len == IEEE802154_SHORT_ADDR_LENGTH) {
		params.dst.short_addr = ctx->coord.short_addr;
	} else {
		params.dst.ext_addr = ctx->coord.ext_addr;
	}

	params.pan_id = ctx->pan_id;

	pkt = ieee802154_create_mac_cmd_frame(
		ctx, IEEE802154_CFI_DISASSOCIATION_NOTIFICATION, &params);
	if (!pkt) {
		return -ENOBUFS;
	}

	cmd = ieee802154_get_mac_command(pkt);
	cmd->disassoc_note.reason = IEEE802154_DRF_DEVICE_WISH;

	if (net_if_send_data(iface, pkt)) {
		net_pkt_unref(pkt);
		return -EIO;
	}

	ctx->associated = false;

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_DISASSOCIATE,
				  ieee802154_disassociate);

static int ieee802154_set_ack(u32_t mgmt_request, struct net_if *iface,
			      void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	ARG_UNUSED(data);
	ARG_UNUSED(len);

	if (mgmt_request == NET_REQUEST_IEEE802154_SET_ACK) {
		ctx->ack_requested = true;
	} else if (mgmt_request == NET_REQUEST_IEEE802154_UNSET_ACK) {
		ctx->ack_requested = false;
	}

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_ACK,
				  ieee802154_set_ack);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_UNSET_ACK,
				  ieee802154_set_ack);

static int ieee802154_set_parameters(u32_t mgmt_request,
				     struct net_if *iface,
				     void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	u16_t value;
	int ret = 0;

	if (ctx->associated) {
		return -EBUSY;
	}

	if (mgmt_request != NET_REQUEST_IEEE802154_SET_EXT_ADDR &&
	    (len != sizeof(u16_t) || !data)) {
		return -EINVAL;
	}

	value = *((u16_t *) data);

	if (mgmt_request == NET_REQUEST_IEEE802154_SET_CHANNEL) {
		if (ctx->channel != value) {
			if (!ieee802154_verify_channel(iface, value)) {
				return -EINVAL;
			}

			ret = ieee802154_set_channel(iface, value);
			if (!ret) {
				ctx->channel = value;
			}
		}
	} else if (mgmt_request == NET_REQUEST_IEEE802154_SET_PAN_ID) {
		if (ctx->pan_id != value) {
			ctx->pan_id = value;
			ieee802154_filter_pan_id(iface, ctx->pan_id);
		}
	} else if (mgmt_request == NET_REQUEST_IEEE802154_SET_EXT_ADDR) {
		if (len != IEEE802154_EXT_ADDR_LENGTH) {
			return -EINVAL;
		}

		if (memcmp(ctx->ext_addr, data, IEEE802154_EXT_ADDR_LENGTH)) {
			memcpy(ctx->ext_addr, data, IEEE802154_EXT_ADDR_LENGTH);
			ieee802154_filter_ieee_addr(iface, ctx->ext_addr);
		}
	} else if (mgmt_request == NET_REQUEST_IEEE802154_SET_SHORT_ADDR) {
		if (ctx->short_addr != value) {
			ctx->short_addr = value;
			ieee802154_filter_short_addr(iface, ctx->short_addr);
		}
	} else if (mgmt_request == NET_REQUEST_IEEE802154_SET_TX_POWER) {
		if (ctx->tx_power != (s16_t)value) {
			ret = ieee802154_set_tx_power(iface, (s16_t)value);
			if (!ret) {
				ctx->tx_power = (s16_t)value;
			}
		}
	}

	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_CHANNEL,
				  ieee802154_set_parameters);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_PAN_ID,
				  ieee802154_set_parameters);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_EXT_ADDR,
				  ieee802154_set_parameters);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_SHORT_ADDR,
				  ieee802154_set_parameters);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_TX_POWER,
				  ieee802154_set_parameters);

static int ieee802154_get_parameters(u32_t mgmt_request,
				     struct net_if *iface,
				     void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	u16_t *value;

	if (mgmt_request != NET_REQUEST_IEEE802154_GET_EXT_ADDR &&
	    (len != sizeof(u16_t) || !data)) {
		return -EINVAL;
	}

	value = (u16_t *)data;

	if (mgmt_request == NET_REQUEST_IEEE802154_GET_CHANNEL) {
		*value = ctx->channel;
	} else if (mgmt_request == NET_REQUEST_IEEE802154_GET_PAN_ID) {
		*value = ctx->pan_id;
	} else if (mgmt_request == NET_REQUEST_IEEE802154_GET_EXT_ADDR) {
		if (len != IEEE802154_EXT_ADDR_LENGTH) {
			return -EINVAL;
		}

		memcpy(data, ctx->ext_addr, IEEE802154_EXT_ADDR_LENGTH);
	} else if (mgmt_request == NET_REQUEST_IEEE802154_GET_SHORT_ADDR) {
		*value = ctx->short_addr;
	} else if (mgmt_request == NET_REQUEST_IEEE802154_GET_TX_POWER) {
		s16_t *s_value = (s16_t *)data;

		*s_value = ctx->tx_power;
	}

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_GET_CHANNEL,
				  ieee802154_get_parameters);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_GET_PAN_ID,
				  ieee802154_get_parameters);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_GET_EXT_ADDR,
				  ieee802154_get_parameters);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_GET_SHORT_ADDR,
				  ieee802154_get_parameters);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_GET_TX_POWER,
				  ieee802154_get_parameters);

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY

static int ieee802154_set_security_settings(u32_t mgmt_request,
					    struct net_if *iface,
					    void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_security_params *params;

	if (ctx->associated) {
		return -EBUSY;
	}

	if (len != sizeof(struct ieee802154_security_params) ||
	    !data) {
		return -EINVAL;
	}

	params = (struct ieee802154_security_params *)data;

	if (ieee802154_security_setup_session(&ctx->sec_ctx, params->level,
					      params->key_mode, params->key,
					      params->key_len)) {
		NET_ERR("Could not set the security parameters");
		return -EINVAL;
	}

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_SECURITY_SETTINGS,
				  ieee802154_set_security_settings);

static int ieee802154_get_security_settings(u32_t mgmt_request,
					    struct net_if *iface,
					    void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_security_params *params;

	if (len != sizeof(struct ieee802154_security_params) ||
	    !data) {
		return -EINVAL;
	}

	params = (struct ieee802154_security_params *)data;

	memcpy(params->key, ctx->sec_ctx.key, ctx->sec_ctx.key_len);
	params->key_len = ctx->sec_ctx.key_len;
	params->key_mode = ctx->sec_ctx.key_mode;
	params->level = ctx->sec_ctx.level;

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_GET_SECURITY_SETTINGS,
				  ieee802154_get_security_settings);

#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */
