/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * IEEE 802.15.4 HDLC RCP interface. This is meant for network connectivity
 * between a host and a RCP radio device.
 */

/* -------------------------------------------------------------------------- */
/*                                  Includes                                  */
/* -------------------------------------------------------------------------- */
#include <fwk_platform_hdlc.h>
#include <fwk_platform_ot.h>
#include <openthread/platform/radio.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/hdlc_rcp_if/hdlc_rcp_if.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/openthread.h>

/* -------------------------------------------------------------------------- */
/*                                  Definitions                               */
/* -------------------------------------------------------------------------- */

#define DT_DRV_COMPAT nxp_hdlc_rcp_if

#define LOG_MODULE_NAME hdlc_rcp_if_nxp
#define LOG_LEVEL       CONFIG_HDLC_RCP_IF_DRIVER_LOG_LEVEL
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static struct ot_hdlc_rcp_context {
	struct net_if *iface;
	struct openthread_context *ot_context;
} ot_hdlc_rcp_ctx;

/* -------------------------------------------------------------------------- */
/*                             Private prototypes                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                             Private functions                              */
/* -------------------------------------------------------------------------- */

static void hdlc_iface_init(struct net_if *iface)
{
	struct ot_hdlc_rcp_context *ctx = net_if_get_device(iface)->data;
	otExtAddress eui64;

	ctx->iface = iface;

	ieee802154_init(iface);

	ctx->ot_context = net_if_l2_data(iface);

	otPlatRadioGetIeeeEui64(ctx->ot_context->instance, eui64.m8);
	net_if_set_link_addr(iface, eui64.m8, OT_EXT_ADDRESS_SIZE, NET_LINK_IEEE802154);
}

static int hdlc_register_rx_cb(hdlc_rx_callback_t hdlc_rx_callback, void *param)
{
	int ret = 0;

	ret = PLATFORM_InitHdlcInterface(hdlc_rx_callback, param);
	if (ret < 0) {
		LOG_ERR("HDLC RX callback registration failed");
	}

	return ret;
}

static int hdlc_send(const uint8_t *frame, uint16_t length)
{
	int ret = 0;

	ret = PLATFORM_SendHdlcMessage((uint8_t *)frame, length);
	if (ret < 0) {
		LOG_ERR("HDLC send frame failed");
	}

	return ret;
}

static int hdlc_deinit(void)
{
	int ret = 0;

	ret = PLATFORM_TerminateHdlcInterface();
	if (ret < 0) {
		LOG_ERR("Failed to shutdown OpenThread controller");
	}

	return ret;
}

static const struct hdlc_api nxp_hdlc_api = {
	.iface_api.init = hdlc_iface_init,
	.register_rx_cb = hdlc_register_rx_cb,
	.send = hdlc_send,
	.deinit = hdlc_deinit,
};

#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)

#define MTU 1280

NET_DEVICE_DT_INST_DEFINE(0, NULL,                             /* Initialization Function */
			  NULL,                                /* No PM API support */
			  &ot_hdlc_rcp_ctx,                    /* HDLC RCP context data */
			  NULL,                                /* Configuration info */
			  CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, /* Initial priority */
			  &nxp_hdlc_api,                       /* API interface functions */
			  OPENTHREAD_L2,                       /* Openthread L2 */
			  NET_L2_GET_CTX_TYPE(OPENTHREAD_L2),  /* Openthread L2 context type */
			  MTU);                                /* MTU size */
