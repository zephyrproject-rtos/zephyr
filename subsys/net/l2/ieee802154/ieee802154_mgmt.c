/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_mgmt, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <zephyr/net/net_core.h>

#include <errno.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/ieee802154_mgmt.h>
#include <zephyr/net/ieee802154.h>

#include "ieee802154_frame.h"
#include "ieee802154_mgmt_priv.h"
#include "ieee802154_security.h"
#include "ieee802154_utils.h"
#include "ieee802154_radio_utils.h"

enum net_verdict ieee802154_handle_beacon(struct net_if *iface,
					  struct ieee802154_mpdu *mpdu,
					  uint8_t lqi)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	NET_DBG("Beacon received");

	if (!ctx->scan_ctx) {
		return NET_DROP;
	}

	if (!mpdu->beacon->sf.association) {
		return NET_DROP;
	}

	k_sem_take(&ctx->scan_ctx_lock, K_FOREVER);

	ctx->scan_ctx->pan_id = mpdu->mhr.src_addr->plain.pan_id;
	ctx->scan_ctx->lqi = lqi;

	if (mpdu->mhr.fs->fc.src_addr_mode == IEEE802154_ADDR_MODE_SHORT) {
		ctx->scan_ctx->len = IEEE802154_SHORT_ADDR_LENGTH;
		ctx->scan_ctx->short_addr =
			sys_le16_to_cpu(mpdu->mhr.src_addr->plain.addr.short_addr);
	} else {
		ctx->scan_ctx->len = IEEE802154_EXT_ADDR_LENGTH;
		sys_memcpy_swap(ctx->scan_ctx->addr,
				mpdu->mhr.src_addr->plain.addr.ext_addr,
				IEEE802154_EXT_ADDR_LENGTH);
	}

	net_mgmt_event_notify(NET_EVENT_IEEE802154_SCAN_RESULT, iface);

	k_sem_give(&ctx->scan_ctx_lock);

	return NET_OK;
}

static int ieee802154_cancel_scan(uint32_t mgmt_request, struct net_if *iface,
				  void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	ARG_UNUSED(data);
	ARG_UNUSED(len);

	NET_DBG("Cancelling scan request");

	k_sem_take(&ctx->scan_ctx_lock, K_FOREVER);
	ctx->scan_ctx = NULL;
	k_sem_give(&ctx->scan_ctx_lock);

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_CANCEL_SCAN,
				  ieee802154_cancel_scan);

static int ieee802154_scan(uint32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_req_params *scan =
		(struct ieee802154_req_params *)data;
	struct net_pkt *pkt = NULL;
	uint8_t channel;
	int ret;

	NET_DBG("%s scan requested",
		mgmt_request == NET_REQUEST_IEEE802154_ACTIVE_SCAN ?
		"Active" : "Passive");

	if (scan == NULL) {
		return -EINVAL;
	}

	k_sem_take(&ctx->scan_ctx_lock, K_FOREVER);

	if (ctx->scan_ctx) {
		ret = -EALREADY;
		goto out;
	}

	if (mgmt_request == NET_REQUEST_IEEE802154_ACTIVE_SCAN) {
		struct ieee802154_frame_params params;

		params.dst.short_addr = IEEE802154_BROADCAST_ADDRESS;
		params.dst.pan_id = IEEE802154_BROADCAST_PAN_ID;

		pkt = ieee802154_create_mac_cmd_frame(
			iface, IEEE802154_CFI_BEACON_REQUEST, &params);
		if (!pkt) {
			NET_DBG("Could not create Beacon Request");
			ret = -ENOBUFS;
			goto out;
		}

		ieee802154_mac_cmd_finalize(pkt, IEEE802154_CFI_BEACON_REQUEST);
	}

	ctx->scan_ctx = scan;
	k_sem_give(&ctx->scan_ctx_lock);

	ret = 0;

	ieee802154_filter_pan_id(iface, IEEE802154_BROADCAST_PAN_ID);

	if (ieee802154_start(iface)) {
		NET_DBG("Could not start device");
		ret = -EIO;

		goto out;
	}

	/* TODO: For now, we assume we are on 2.4Ghz
	 * (device will have to export current channel page)
	 */
	for (channel = 11U; channel <= 26U; channel++) {
		if (IEEE802154_IS_CHAN_UNSCANNED(scan->channel_set, channel)) {
			continue;
		}

		scan->channel = channel;
		NET_DBG("Scanning channel %u", channel);
		ieee802154_set_channel(iface, channel);

		/* Active scan sends a beacon request */
		if (mgmt_request == NET_REQUEST_IEEE802154_ACTIVE_SCAN) {
			net_pkt_ref(pkt);
			net_pkt_frag_ref(pkt->buffer);

			ret = ieee802154_radio_send(iface, pkt, pkt->buffer);
			if (ret) {
				NET_DBG("Could not send Beacon Request (%d)",
					ret);
				net_pkt_unref(pkt);
				k_sem_take(&ctx->scan_ctx_lock, K_FOREVER);
				goto out;
			}
		}

		/* Context aware sleep */
		k_sleep(K_MSEC(scan->duration));

		k_sem_take(&ctx->scan_ctx_lock, K_FOREVER);

		if (!ctx->scan_ctx) {
			NET_DBG("Scan request cancelled");
			ret = -ECANCELED;
			goto out;
		}

		k_sem_give(&ctx->scan_ctx_lock);
	}

	/* Let's come back to context's settings */
	ieee802154_filter_pan_id(iface, ctx->pan_id);
	ieee802154_set_channel(iface, ctx->channel);

out:
	ctx->scan_ctx = NULL;
	k_sem_give(&ctx->scan_ctx_lock);

	if (pkt) {
		net_pkt_unref(pkt);
	}

	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_PASSIVE_SCAN,
				  ieee802154_scan);
NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_ACTIVE_SCAN,
				  ieee802154_scan);

/* Requires the context lock to be held. */
static inline void update_net_if_link_addr(struct net_if *iface, struct ieee802154_context *ctx)
{
	bool was_if_up;

	was_if_up = net_if_flag_test_and_clear(iface, NET_IF_RUNNING);
	net_if_set_link_addr(iface, ctx->linkaddr.addr, ctx->linkaddr.len, ctx->linkaddr.type);

	if (was_if_up) {
		net_if_flag_set(iface, NET_IF_RUNNING);
	}
}

/* Requires the context lock to be held. */
static inline void set_linkaddr_to_ext_addr(struct net_if *iface, struct ieee802154_context *ctx)
{
	ctx->linkaddr.len = IEEE802154_EXT_ADDR_LENGTH;
	sys_memcpy_swap(ctx->linkaddr.addr, ctx->ext_addr, IEEE802154_EXT_ADDR_LENGTH);

	update_net_if_link_addr(iface, ctx);
}

/* Requires the context lock to be held. */
static inline void set_association(struct net_if *iface, struct ieee802154_context *ctx,
				   uint16_t short_addr)
{
	__ASSERT_NO_MSG(short_addr != IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED);

	uint16_t short_addr_be;

	ctx->linkaddr.len = IEEE802154_SHORT_ADDR_LENGTH;
	ctx->short_addr = short_addr;
	short_addr_be = htons(short_addr);
	memcpy(ctx->linkaddr.addr, &short_addr_be, IEEE802154_SHORT_ADDR_LENGTH);

	update_net_if_link_addr(iface, ctx);
	ieee802154_filter_short_addr(iface, ctx->short_addr);
}

/* Requires the context lock to be held. */
static inline void remove_association(struct net_if *iface, struct ieee802154_context *ctx)
{
	ctx->short_addr = IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED;
	memset(ctx->coord_ext_addr, 0, IEEE802154_EXT_ADDR_LENGTH);
	ctx->coord_short_addr = 0U;
	set_linkaddr_to_ext_addr(iface, ctx);
	ieee802154_filter_short_addr(iface, IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED);
}

/* Requires the context lock to be held. */
static inline bool is_associated(struct ieee802154_context *ctx)
{
	return ctx->short_addr != IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED;
}

enum net_verdict ieee802154_handle_mac_command(struct net_if *iface,
					       struct ieee802154_mpdu *mpdu)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	if (mpdu->command->cfi == IEEE802154_CFI_ASSOCIATION_RESPONSE) {
		if (mpdu->command->assoc_res.status !=
		    IEEE802154_ASF_SUCCESSFUL) {
			return NET_DROP;
		}

		/* validation of the association response, see section 7.3.3.1 */

		if (mpdu->mhr.fs->fc.src_addr_mode !=
			    IEEE802154_ADDR_MODE_EXTENDED ||
		    mpdu->mhr.fs->fc.dst_addr_mode !=
			    IEEE802154_ADDR_MODE_EXTENDED ||
		    mpdu->mhr.fs->fc.ar != 1 ||
		    mpdu->mhr.fs->fc.pan_id_comp != 1) {
			return NET_DROP;
		}

		k_sem_take(&ctx->ctx_lock, K_FOREVER);

		set_association(iface, ctx, sys_le16_to_cpu(mpdu->command->assoc_res.short_addr));

		memcpy(ctx->coord_ext_addr,
		       mpdu->mhr.src_addr->comp.addr.ext_addr,
		       IEEE802154_EXT_ADDR_LENGTH);

		k_sem_give(&ctx->ctx_lock);

		k_sem_give(&ctx->scan_ctx_lock);

		return NET_OK;
	}

	if (mpdu->command->cfi == IEEE802154_CFI_DISASSOCIATION_NOTIFICATION) {
		enum net_verdict ret = NET_DROP;

		if (mpdu->command->disassoc_note.reason !=
		    IEEE802154_DRF_COORDINATOR_WISH) {
			return NET_DROP;
		}

		k_sem_take(&ctx->ctx_lock, K_FOREVER);

		if (!is_associated(ctx)) {
			goto out;
		}

		/* validation of the disassociation request, see section 7.3.3.1 */

		if (mpdu->mhr.fs->fc.src_addr_mode !=
			    IEEE802154_ADDR_MODE_EXTENDED ||
		    mpdu->mhr.fs->fc.pan_id_comp != 1) {
			goto out;
		}

		if (memcmp(ctx->coord_ext_addr,
			   mpdu->mhr.src_addr->comp.addr.ext_addr,
			   IEEE802154_EXT_ADDR_LENGTH)) {
			goto out;
		}

		remove_association(iface, ctx);
		ret = NET_OK;

out:
		k_sem_give(&ctx->ctx_lock);
		return ret;
	}

	NET_DBG("Drop MAC command, unsupported CFI: 0x%x",
		mpdu->command->cfi);

	return NET_DROP;
}

static int ieee802154_associate(uint32_t mgmt_request, struct net_if *iface,
				void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_req_params *req =
		(struct ieee802154_req_params *)data;
	struct ieee802154_frame_params params;
	struct ieee802154_command *cmd;
	struct net_pkt *pkt;
	int ret = 0;

	params.dst.len = req->len;
	if (params.dst.len == IEEE802154_SHORT_ADDR_LENGTH) {
		params.dst.short_addr = req->short_addr;
	} else {
		params.dst.ext_addr = req->addr;
	}

	params.dst.pan_id = req->pan_id;
	params.pan_id = req->pan_id;

	pkt = ieee802154_create_mac_cmd_frame(
		iface, IEEE802154_CFI_ASSOCIATION_REQUEST, &params);
	if (!pkt) {
		ret = -ENOBUFS;
		goto out;
	}

	cmd = ieee802154_get_mac_command(pkt);
	cmd->assoc_req.ci.reserved_1 = 0U; /* Reserved */
	cmd->assoc_req.ci.dev_type = 0U; /* RFD */
	cmd->assoc_req.ci.power_src = 0U; /* TODO: set right power source */
	cmd->assoc_req.ci.rx_on = 1U; /* TODO: that will depends on PM */
	cmd->assoc_req.ci.reserved_2 = 0U; /* Reserved */
	cmd->assoc_req.ci.sec_capability = 0U; /* TODO: security support */
	cmd->assoc_req.ci.alloc_addr = 0U; /* TODO: handle short addr */

	k_sem_take(&ctx->ctx_lock, K_FOREVER);
	remove_association(iface, ctx);
	k_sem_give(&ctx->ctx_lock);

	ieee802154_mac_cmd_finalize(pkt, IEEE802154_CFI_ASSOCIATION_REQUEST);

	ieee802154_filter_pan_id(iface, req->pan_id);

	if (net_if_send_data(iface, pkt)) {
		net_pkt_unref(pkt);
		ret = -EIO;
		goto out;
	}

	/* acquire the lock so that the next k_sem_take() blocks */
	k_sem_take(&ctx->scan_ctx_lock, K_FOREVER);

	/* TODO: current timeout is arbitrary, see IEEE 802.15.4-2015, 8.4.2, macResponseWaitTime */
	k_sem_take(&ctx->scan_ctx_lock, K_SECONDS(1));

	/* release the lock */
	k_sem_give(&ctx->scan_ctx_lock);

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	if (is_associated(ctx)) {
		bool validated = false;

		if (req->len == IEEE802154_SHORT_ADDR_LENGTH &&
		    ctx->coord_short_addr == req->short_addr) {
			validated = true;
		} else {
			if (req->len == IEEE802154_EXT_ADDR_LENGTH) {
				uint8_t ext_addr_le[IEEE802154_EXT_ADDR_LENGTH];

				sys_memcpy_swap(ext_addr_le, req->addr,
						IEEE802154_EXT_ADDR_LENGTH);
				if (!memcmp(ctx->coord_ext_addr, ext_addr_le,
					    IEEE802154_EXT_ADDR_LENGTH)) {
					validated = true;
				}
			}
		}

		if (!validated) {
			remove_association(iface, ctx);
			ret = -EFAULT;
			goto out;
		}

		ctx->channel = req->channel;
		ctx->pan_id = req->pan_id;
		goto out;

	} else {
		ret = -EACCES;
	}

out:
	if (ret < 0) {
		ieee802154_filter_pan_id(iface, 0);
	}

	k_sem_give(&ctx->ctx_lock);
	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_ASSOCIATE,
				  ieee802154_associate);

static int ieee802154_disassociate(uint32_t mgmt_request, struct net_if *iface,
				   void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_frame_params params;
	struct ieee802154_command *cmd;
	struct net_pkt *pkt;
	int ret = 0;

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	if (!is_associated(ctx)) {
		ret = -EALREADY;
		goto out;
	}

	/* See section 7.3.3 */

	params.dst.pan_id = ctx->pan_id;

	if (ctx->coord_short_addr != 0 &&
	    ctx->coord_short_addr != IEEE802154_NO_SHORT_ADDRESS_ASSIGNED) {
		params.dst.len = IEEE802154_SHORT_ADDR_LENGTH;
		params.dst.short_addr = ctx->coord_short_addr;
	} else {
		params.dst.len = IEEE802154_EXT_ADDR_LENGTH;
		sys_memcpy_swap(params.dst.ext_addr, ctx->coord_ext_addr,
				IEEE802154_EXT_ADDR_LENGTH);
	}

	params.pan_id = ctx->pan_id;

	pkt = ieee802154_create_mac_cmd_frame(
		iface, IEEE802154_CFI_DISASSOCIATION_NOTIFICATION, &params);
	if (!pkt) {
		ret = -ENOBUFS;
		goto out;
	}

	cmd = ieee802154_get_mac_command(pkt);
	cmd->disassoc_note.reason = IEEE802154_DRF_DEVICE_WISH;

	ieee802154_mac_cmd_finalize(
		pkt, IEEE802154_CFI_DISASSOCIATION_NOTIFICATION);

	if (net_if_send_data(iface, pkt)) {
		net_pkt_unref(pkt);
		ret = -EIO;
		goto out;
	}

	remove_association(iface, ctx);

out:
	k_sem_give(&ctx->ctx_lock);
	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_DISASSOCIATE,
				  ieee802154_disassociate);

static int ieee802154_set_ack(uint32_t mgmt_request, struct net_if *iface,
			      void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	ARG_UNUSED(data);
	ARG_UNUSED(len);

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	if (mgmt_request == NET_REQUEST_IEEE802154_SET_ACK) {
		ctx->ack_requested = true;
	} else if (mgmt_request == NET_REQUEST_IEEE802154_UNSET_ACK) {
		ctx->ack_requested = false;
	}

	k_sem_give(&ctx->ctx_lock);

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_ACK,
				  ieee802154_set_ack);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_UNSET_ACK,
				  ieee802154_set_ack);

static int ieee802154_set_parameters(uint32_t mgmt_request,
				     struct net_if *iface,
				     void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	uint16_t value;
	int ret = 0;

	if (mgmt_request != NET_REQUEST_IEEE802154_SET_EXT_ADDR &&
	    (len != sizeof(uint16_t) || !data)) {
		return -EINVAL;
	}

	value = *((uint16_t *) data);

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	if (is_associated(ctx) && !(mgmt_request == NET_REQUEST_IEEE802154_SET_SHORT_ADDR &&
				    value == IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED)) {
		ret = -EBUSY;
		goto out;
	}

	if (mgmt_request == NET_REQUEST_IEEE802154_SET_CHANNEL) {
		if (ctx->channel != value) {
			if (!ieee802154_verify_channel(iface, value)) {
				ret = -EINVAL;
				goto out;
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
			ret = -EINVAL;
			goto out;
		}

		uint8_t ext_addr_le[IEEE802154_EXT_ADDR_LENGTH];

		sys_memcpy_swap(ext_addr_le, data, IEEE802154_EXT_ADDR_LENGTH);

		if (memcmp(ctx->ext_addr, ext_addr_le, IEEE802154_EXT_ADDR_LENGTH)) {
			memcpy(ctx->ext_addr, ext_addr_le, IEEE802154_EXT_ADDR_LENGTH);

			if (net_if_get_link_addr(iface)->len == IEEE802154_EXT_ADDR_LENGTH) {
				set_linkaddr_to_ext_addr(iface, ctx);
			}

			ieee802154_filter_ieee_addr(iface, ctx->ext_addr);
		}
	} else if (mgmt_request == NET_REQUEST_IEEE802154_SET_SHORT_ADDR) {
		if (ctx->short_addr != value) {
			if (value == IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED) {
				remove_association(iface, ctx);
			} else {
				set_association(iface, ctx, value);
			}
		}
	} else if (mgmt_request == NET_REQUEST_IEEE802154_SET_TX_POWER) {
		if (ctx->tx_power != (int16_t)value) {
			ret = ieee802154_set_tx_power(iface, (int16_t)value);
			if (!ret) {
				ctx->tx_power = (int16_t)value;
			}
		}
	}

out:
	k_sem_give(&ctx->ctx_lock);
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

static int ieee802154_get_parameters(uint32_t mgmt_request,
				     struct net_if *iface,
				     void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	uint16_t *value;
	int ret = 0;

	if (mgmt_request != NET_REQUEST_IEEE802154_GET_EXT_ADDR &&
	    (len != sizeof(uint16_t) || !data)) {
		return -EINVAL;
	}

	value = (uint16_t *)data;

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	if (mgmt_request == NET_REQUEST_IEEE802154_GET_CHANNEL) {
		*value = ctx->channel;
	} else if (mgmt_request == NET_REQUEST_IEEE802154_GET_PAN_ID) {
		*value = ctx->pan_id;
	} else if (mgmt_request == NET_REQUEST_IEEE802154_GET_EXT_ADDR) {
		if (len != IEEE802154_EXT_ADDR_LENGTH) {
			ret = -EINVAL;
			goto out;
		}

		sys_memcpy_swap(data, ctx->ext_addr, IEEE802154_EXT_ADDR_LENGTH);
	} else if (mgmt_request == NET_REQUEST_IEEE802154_GET_SHORT_ADDR) {
		*value = ctx->short_addr;
	} else if (mgmt_request == NET_REQUEST_IEEE802154_GET_TX_POWER) {
		int16_t *s_value = (int16_t *)data;

		*s_value = ctx->tx_power;
	}

out:
	k_sem_give(&ctx->ctx_lock);
	return ret;
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

static int ieee802154_set_security_settings(uint32_t mgmt_request,
					    struct net_if *iface,
					    void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_security_params *params;
	int ret = 0;

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	if (is_associated(ctx)) {
		ret = -EBUSY;
		goto out;
	}

	if (len != sizeof(struct ieee802154_security_params) || !data) {
		ret = -EINVAL;
		goto out;
	}

	params = (struct ieee802154_security_params *)data;

	if (ieee802154_security_setup_session(&ctx->sec_ctx, params->level,
					      params->key_mode, params->key,
					      params->key_len)) {
		NET_ERR("Could not set the security parameters");
		ret = -EINVAL;
	}

out:
	k_sem_give(&ctx->ctx_lock);
	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_SET_SECURITY_SETTINGS,
				  ieee802154_set_security_settings);

static int ieee802154_get_security_settings(uint32_t mgmt_request,
					    struct net_if *iface,
					    void *data, size_t len)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);
	struct ieee802154_security_params *params;

	if (len != sizeof(struct ieee802154_security_params) || !data) {
		return -EINVAL;
	}

	params = (struct ieee802154_security_params *)data;

	k_sem_take(&ctx->ctx_lock, K_FOREVER);

	memcpy(params->key, ctx->sec_ctx.key, ctx->sec_ctx.key_len);
	params->key_len = ctx->sec_ctx.key_len;
	params->key_mode = ctx->sec_ctx.key_mode;
	params->level = ctx->sec_ctx.level;

	k_sem_give(&ctx->ctx_lock);

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_IEEE802154_GET_SECURITY_SETTINGS,
				  ieee802154_get_security_settings);

#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */
