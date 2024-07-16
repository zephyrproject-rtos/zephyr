/*
 * Copyright (c) 2024, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * IEEE 802.15.4 HDLC RCP interface. This is meant for network connectivity between
 * a host and a RCP radio device.
 */

/* -------------------------------------------------------------------------- */
/*                                  Includes                                  */
/* -------------------------------------------------------------------------- */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stddef.h>
#include <zephyr/init.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/hdlc_rcp_if/hdlc_rcp_if.h>
#include <openthread/platform/radio.h>
#include "fwk_platform_hdlc.h"
#include "fwk_platform_ot.h"

/* -------------------------------------------------------------------------- */
/*                                  Definitions                               */
/* -------------------------------------------------------------------------- */

#define DT_DRV_COMPAT nxp_hdlc_rcp_if

#define LOG_MODULE_NAME hdlc_rcp_if_nxp
#define LOG_LEVEL CONFIG_NET_OPENTHREAD_HOST_LOG_LEVEL

/* -------------------------------------------------------------------------- */
/*                             Private prototypes                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                             Private functions                              */
/* -------------------------------------------------------------------------- */

static void hdlc_iface_init(struct net_if *iface)
{
	ieee802154_init(iface);
}

static int hdlc_register_rx_cb(hdlc_rx_callback_t hdlc_rx_callback, void *param)
{
	int ret = 0;
	int status = 0;

	do {
		status = PLATFORM_InitHdlcInterface(hdlc_rx_callback, param);
		if (status < 0)	{
			LOG_ERR("HDLC RX callback registration failed");
			ret = EIO;
			break;
		}

	} while (false);

	return ret;
}

static int hdlc_send(const uint8_t *aFrame, uint16_t aLength)
{
	int ret = 0;
	int status = 0;

	status = PLATFORM_SendHdlcMessage((uint8_t *)aFrame, aLength);
	if (status < 0) {
		LOG_ERR("HDLC send frame failed");
		ret = EIO;
	}

	return ret;
}

static int hdlc_deinit(void)
{
	int ret = 0;
	int status = 0;

	status = PLATFORM_TerminateHdlcInterface();
	if (status < 0) {
		LOG_ERR("Failed to shutdown OpenThread controller");
		ret = EIO;
	}

	return ret;
}

static struct hdlc_api hdlc_api = {
	.iface_api.init   = hdlc_iface_init,
	.register_rx_cb   = hdlc_register_rx_cb,
	.send             = hdlc_send,
	.deinit           = hdlc_deinit
};

#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)
#define hdlc_MTU 1280

NET_DEVICE_DT_INST_DEFINE(
	0,
	NULL,                                  /* Initialization Function */
	NULL,                                  /* No PM API support */
	NULL,                                  /* No context data */
	NULL,                                  /* Configuration info */
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,   /* Initial priority */
	&hdlc_api,                             /* API interface functions */
	OPENTHREAD_L2,                         /* Openthread L2 */
	NET_L2_GET_CTX_TYPE(OPENTHREAD_L2),    /* Openthread L2 context type */
	hdlc_MTU);                             /* MTU size */
