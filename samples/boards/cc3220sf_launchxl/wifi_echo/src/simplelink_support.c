/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <stdint.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <CC3220SF_LAUNCHXL.h>
#include "simplelink_support.h"

#define SYS_LOG_DOMAIN "SimpleLink WiFi"
#define SYS_LOG_LEVEL 4
#include <logging/sys_log.h>

#define SET_STATUS_BIT(status, bit) {status |= (1 << (bit)); }
#define CLR_STATUS_BIT(status, bit) {status &= ~(1 << (bit)); }
#define GET_STATUS_BIT(status, bit) (0 != (status & (1 << (bit))))

#define SSID_LEN_MAX	 (32)
#define BSSID_LEN_MAX	 (6)
#define SL_STOP_TIMEOUT	 (200)

#undef ASSERT_ON_ERROR
#define ASSERT_ON_ERROR(ret, e) __ASSERT(ret >= 0, e)
#define DEVICE_ERROR	 ("See \"DEVICE ERRORS CODES\" in SimpleLink errors.h")
#define WLAN_ERROR	 ("See \"WLAN ERRORS CODES\" in SimpleLink errors.h")
#define NETAPP_ERROR	 ("See \"NETAPP ERRORS CODES\" in SimpleLink errors.h")

#define CHANNEL_MASK_ALL	    (0x1FFF)
#define RSSI_TH_MAX		    (-95)

/* KConfig options for other security types could be added in future. */
#define SECURITY_TYPE	    SL_WLAN_SEC_TYPE_WPA_WPA2

enum status_bits {
	/* Network Processor is powered up */
	STATUS_BIT_NWP_INIT = 0,
	/* The device is connected to the AP */
	STATUS_BIT_CONNECTION,
	/* The device has leased IP to any connected client */
	STATUS_BIT_IP_LEASED,
	/* The device has acquired an IP */
	STATUS_BIT_IP_ACQUIRED,
	/* The device has acquired an IPv6 address */
	STATUS_BIT_IPV6_ACQUIRED,
};

struct connect_state {
	struct k_sem connect_sem;
	struct k_sem ip4acquire_sem;
	struct k_sem ip6acquire_sem;
	u32_t gateway_ip;
	u8_t ssid[SSID_LEN_MAX + 1];
	u8_t bssid[BSSID_LEN_MAX];
	u32_t ip_addr;
	u32_t sta_ip;
	u32_t ipv6_addr[4];
};

struct nwp_status {
	/* Status Variables */
	u32_t status;	/* The state of the NWP */
	u32_t role;	/* The device's role (STA, P2P or AP) */

	/* STA/AP mode state */
	struct connect_state conn;
};

/* Network Coprocessor state, including role and connection state: */
struct nwp_status nwp;

/* Configure the device to a default state, resetting previous parameters .*/
static s32_t configure_simplelink(void)
{
	u8_t config_opt;
	u8_t power;
	s32_t retval = -1;
	s32_t mode = -1;
	u32_t if_bitmap = 0;
	SlWlanScanParamCommand_t scan_default = { 0 };
	SlWlanRxFilterOperationCommandBuff_t rx_filterid_mask = { { 0 } };

	/* Turn on NWP */
	mode = sl_Start(0, 0, 0);
	ASSERT_ON_ERROR(mode, DEVICE_ERROR);

	if (mode != ROLE_STA) {
		/* Set NWP role as STA */
		mode = sl_WlanSetMode(ROLE_STA);
		ASSERT_ON_ERROR(mode, WLAN_ERROR);

		/* For changes to take affect, we restart the NWP */
		retval = sl_Stop(SL_STOP_TIMEOUT);
		ASSERT_ON_ERROR(retval, DEVICE_ERROR);

		mode = sl_Start(0, 0, 0);
		ASSERT_ON_ERROR(mode, DEVICE_ERROR);
	}

	if (mode != ROLE_STA) {
		SYS_LOG_ERR("Failed to configure NWP to default state");
		return -1;
	}

	/* Set policy to auto only */
	retval = sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION,
				  SL_WLAN_CONNECTION_POLICY(1, 0, 0, 0),
				  NULL, 0);
	ASSERT_ON_ERROR(retval, WLAN_ERROR);

	/* Disable Auto Provisioning */
	retval = sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_STOP, 0xFF, 0,
				     NULL, 0x0);
	ASSERT_ON_ERROR(retval, WLAN_ERROR);

	/* Delete existing profiles */
	retval = sl_WlanProfileDel(0xFF);
	ASSERT_ON_ERROR(retval, WLAN_ERROR);

	/* enable DHCP client */
	retval = sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE,
			      SL_NETCFG_ADDR_DHCP, 0, 0);
	ASSERT_ON_ERROR(retval, NETAPP_ERROR);

	/* Disable ipv6 */
	if_bitmap = !(SL_NETCFG_IF_IPV6_STA_LOCAL |
		      SL_NETCFG_IF_IPV6_STA_GLOBAL);
	retval = sl_NetCfgSet(SL_NETCFG_IF, SL_NETCFG_IF_STATE,
			      sizeof(if_bitmap),
			      (const unsigned char *)&if_bitmap);
	ASSERT_ON_ERROR(retval, NETAPP_ERROR);

	/* Configure scan parameters to default */
	scan_default.ChannelsMask = CHANNEL_MASK_ALL;
	scan_default.RssiThershold = RSSI_TH_MAX;

	retval = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
			    SL_WLAN_GENERAL_PARAM_OPT_SCAN_PARAMS,
			    sizeof(scan_default), (u8_t *)&scan_default);
	ASSERT_ON_ERROR(retval, WLAN_ERROR);

	/* Disable scans */
	config_opt = SL_WLAN_SCAN_POLICY(0, 0);
	retval = sl_WlanPolicySet(SL_WLAN_POLICY_SCAN, config_opt, NULL, 0);
	ASSERT_ON_ERROR(retval, WLAN_ERROR);

	/* Set TX power lvl to max */
	power = 0;
	retval = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID,
			    SL_WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1,
			    (u8_t *)&power);
	ASSERT_ON_ERROR(retval, WLAN_ERROR);

	/* Set NWP Power policy to 'normal' */
	retval = sl_WlanPolicySet(SL_WLAN_POLICY_PM, SL_WLAN_NORMAL_POLICY,
				  NULL, 0);
	ASSERT_ON_ERROR(retval, WLAN_ERROR);

	/* Unregister mDNS services */
	retval = sl_NetAppMDNSUnRegisterService(0, 0, 0);
	ASSERT_ON_ERROR(retval, NETAPP_ERROR);

	/* Remove all 64 RX filters (8*8) */
	memset(rx_filterid_mask.FilterBitmap, 0xFF, 8);

	retval = sl_WlanSet(SL_WLAN_RX_FILTERS_ID, SL_WLAN_RX_FILTER_REMOVE,
			    sizeof(SlWlanRxFilterOperationCommandBuff_t),
			    (u8_t *)&rx_filterid_mask);
	ASSERT_ON_ERROR(retval, WLAN_ERROR);

	/* Set NWP role as STA */
	retval = sl_WlanSetMode(ROLE_STA);
	ASSERT_ON_ERROR(retval, WLAN_ERROR);

	/* For changes to take affect, we restart the NWP */
	retval = sl_Stop(0xFF);
	ASSERT_ON_ERROR(retval, DEVICE_ERROR);

	mode = sl_Start(0, 0, 0);
	ASSERT_ON_ERROR(mode, DEVICE_ERROR);

	if (mode != ROLE_STA) {
		SYS_LOG_ERR("Failed to configure device to it's default state");
		retval = -1;
	} else {
		nwp.role = ROLE_STA;
		SET_STATUS_BIT(nwp.status, STATUS_BIT_NWP_INIT);
		retval = 0;
	}

	return retval;
}

/**
 *  @brief SimpleLinkWlanEventHandler
 *
 *  This handler gets called whenever a WLAN event is reported
 *  by the host driver / NWP.
 *
 *  @note See the CC3120/CC3220 NWP programmer's guide (SWRU455)
 *	  sections 4.3.4, 4.4.5 and 4.5.5.
 */
void SimpleLinkWlanEventHandler(SlWlanEvent_t *wlan_event)
{
	SlWlanEventDisconnect_t	 *event_data = NULL;

	if (!wlan_event) {
		return;
	}

	switch (wlan_event->Id) {
	case SL_WLAN_EVENT_CONNECT:
		SET_STATUS_BIT(nwp.status, STATUS_BIT_CONNECTION);

		/* Store new connection SSID and BSSID: */
		memcpy(nwp.conn.ssid, wlan_event->Data.Connect.SsidName,
		       wlan_event->Data.Connect.SsidLen);
		memcpy(nwp.conn.bssid, wlan_event->Data.Connect.Bssid,
		       BSSID_LEN_MAX);

		SYS_LOG_INF("\n[WLAN EVENT] STA Connected to the AP: %s, "
		       "BSSID: %x:%x:%x:%x:%x:%x",
		       nwp.conn.ssid, nwp.conn.bssid[0],
		       nwp.conn.bssid[1], nwp.conn.bssid[2],
		       nwp.conn.bssid[3], nwp.conn.bssid[4],
		       nwp.conn.bssid[5]);

		k_sem_give(&nwp.conn.connect_sem);
		break;

	case SL_WLAN_EVENT_DISCONNECT:
		CLR_STATUS_BIT(nwp.status, STATUS_BIT_CONNECTION);
		CLR_STATUS_BIT(nwp.status, STATUS_BIT_IP_ACQUIRED);
		CLR_STATUS_BIT(nwp.status, STATUS_BIT_IPV6_ACQUIRED);

		event_data = &wlan_event->Data.Disconnect;

		/* If the user has initiated 'Disconnect' request,
		 * 'reason_code' is SL_WLAN_DISCONNECT_USER_INITIATED
		 */
		if (SL_WLAN_DISCONNECT_USER_INITIATED ==
		    event_data->ReasonCode) {
			SYS_LOG_INF("\n[WLAN EVENT] "
				    "Device disconnected from the AP: %s,\n\r"
				    "BSSID: %x:%x:%x:%x:%x:%x on application's"
				    " request",
				    nwp.conn.ssid, nwp.conn.bssid[0],
				    nwp.conn.bssid[1], nwp.conn.bssid[2],
				    nwp.conn.bssid[3], nwp.conn.bssid[4],
				    nwp.conn.bssid[5]);
		} else {
			SYS_LOG_INF("\n[WLAN ERROR] "
				    "Device disconnected from the AP: %s,\n\r"
				    "BSSID: %x:%x:%x:%x:%x:%x on an ERROR!",
				    nwp.conn.ssid, nwp.conn.bssid[0],
				    nwp.conn.bssid[1], nwp.conn.bssid[2],
				    nwp.conn.bssid[3], nwp.conn.bssid[4],
				    nwp.conn.bssid[5]);
		}
		memset(&(nwp.conn.ssid), 0x0, sizeof(nwp.conn.ssid));
		memset(&(nwp.conn.bssid), 0x0, sizeof(nwp.conn.bssid));
		break;

	case SL_WLAN_EVENT_STA_ADDED:
		memcpy(&(nwp.conn.bssid), wlan_event->Data.STAAdded.Mac,
		       SL_WLAN_BSSID_LENGTH);
		SYS_LOG_INF("\n[WLAN EVENT] STA was added to AP: "
			    "BSSID: %x:%x:%x:%x:%x:%x",
			    nwp.conn.bssid[0], nwp.conn.bssid[1],
			    nwp.conn.bssid[2], nwp.conn.bssid[3],
			    nwp.conn.bssid[4], nwp.conn.bssid[5]);
		break;
	case SL_WLAN_EVENT_STA_REMOVED:
		memcpy(&(nwp.conn.bssid), wlan_event->Data.STAAdded.Mac,
		       SL_WLAN_BSSID_LENGTH);
		SYS_LOG_INF("\n[WLAN EVENT] STA was removed from AP: "
			    "BSSID: %x:%x:%x:%x:%x:%x",
			    nwp.conn.bssid[0], nwp.conn.bssid[1],
			    nwp.conn.bssid[2], nwp.conn.bssid[3],
			    nwp.conn.bssid[4], nwp.conn.bssid[5]);

		memset(&(nwp.conn.bssid), 0x0, sizeof(nwp.conn.bssid));
		break;
	default:
		SYS_LOG_INF("\n[WLAN EVENT] Unexpected event [0x%lx]",
			    wlan_event->Id);
		break;
	}
}

/**
 *  @brief SimpleLinkNetAppEventHandler
 *
 *  This handler gets called whenever a Netapp event is reported
 *  by the host driver / NWP.
 *
 *  @note See the CC3120/CC3220 NWP programmer's guide (SWRU455)
 *	  section 5.7.
 */
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *netapp_event)
{
	u32_t i = 0;

	SlIpV4AcquiredAsync_t *event_data = NULL;

	if (!netapp_event) {
		return;
	}

	switch (netapp_event->Id) {
	case SL_DEVICE_EVENT_DROPPED_NETAPP_IPACQUIRED:
		SET_STATUS_BIT(nwp.status, STATUS_BIT_IP_ACQUIRED);

		/* Ip Acquired Event Data */
		event_data = &netapp_event->Data.IpAcquiredV4;
		nwp.conn.ip_addr = event_data->Ip;

		/* Gateway IP address */
		nwp.conn.gateway_ip = event_data->Gateway;

		SYS_LOG_INF("\n[NETAPP EVENT] IP set to: IPv4=%d.%d.%d.%d, "
		       "Gateway=%d.%d.%d.%d",
		       SL_IPV4_BYTE(nwp.conn.ip_addr, 3),
		       SL_IPV4_BYTE(nwp.conn.ip_addr, 2),
		       SL_IPV4_BYTE(nwp.conn.ip_addr, 1),
		       SL_IPV4_BYTE(nwp.conn.ip_addr, 0),

		       SL_IPV4_BYTE(nwp.conn.gateway_ip, 3),
		       SL_IPV4_BYTE(nwp.conn.gateway_ip, 2),
		       SL_IPV4_BYTE(nwp.conn.gateway_ip, 1),
		       SL_IPV4_BYTE(nwp.conn.gateway_ip, 0));

		k_sem_give(&(nwp.conn.ip4acquire_sem));
		break;

	case SL_DEVICE_EVENT_DROPPED_NETAPP_IPACQUIRED_V6:
		SET_STATUS_BIT(nwp.status, STATUS_BIT_IPV6_ACQUIRED);

		for (i = 0; i < 4; i++) {
			nwp.conn.ipv6_addr[i] =
			  netapp_event->Data.IpAcquiredV6.Ip[i];
		}

		SYS_LOG_INF("[NETAPP EVENT] IP Acquired: IPv6=");

		for (i = 0; i < 3; i++) {
			SYS_LOG_INF("%04x:%04x:",
				    ((nwp.conn.ipv6_addr[i] >> 16) & 0xffff),
				    nwp.conn.ipv6_addr[i] & 0xffff);
		}

		SYS_LOG_INF("%04x:%04x",
			    ((nwp.conn.ipv6_addr[3] >> 16) & 0xffff),
			    nwp.conn.ipv6_addr[3] & 0xffff);

		k_sem_give(&nwp.conn.ip6acquire_sem);
		break;

	case SL_DEVICE_EVENT_DROPPED_NETAPP_IP_LEASED:
		SET_STATUS_BIT(nwp.status, STATUS_BIT_IP_LEASED);
		SET_STATUS_BIT(nwp.status, STATUS_BIT_IP_ACQUIRED);

		nwp.conn.sta_ip = netapp_event->Data.IpLeased.IpAddress;
		SYS_LOG_INF("\n[NETAPP EVENT] IP Leased to Client: "
			    "IP=%d.%d.%d.%d",
			    SL_IPV4_BYTE(nwp.conn.sta_ip, 3),
			    SL_IPV4_BYTE(nwp.conn.sta_ip, 2),
			    SL_IPV4_BYTE(nwp.conn.sta_ip, 1),
			    SL_IPV4_BYTE(nwp.conn.sta_ip, 0));

		k_sem_give(&(nwp.conn.ip4acquire_sem));
		break;

	case SL_DEVICE_EVENT_DROPPED_NETAPP_IP_RELEASED:
		SYS_LOG_INF("\n[NETAPP EVENT] IP is released.");
		break;

	default:
		SYS_LOG_INF("\n[NETAPP EVENT] Unexpected event [0x%lx]",
			    netapp_event->Id);
		break;
	}
}

/**
 *  @brief SimpleLinkGeneralEventHandler
 *
 *  This handler gets called whenever a general error is reported
 *  by the NWP / Host driver. Since these errors are not fatal,
 *  the application can handle them.
 *
 *  @note See the CC3120/CC3220 NWP programmer's guide (SWRU455)
 *	  section 17.9.
 */
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *dev_event)
{
	if (!dev_event) {
		return;
	}

	SYS_LOG_INF("\n[GENERAL EVENT] - ID=[%d] Sender=[%d]",
		    dev_event->Data.Error.Code,
		    dev_event->Data.Error.Source);
}

/**
 *  @brief SimpleLinkFatalErrorEventHandler
 *
 *  This handler gets called whenever a driver error occurs requiring
 *  restart of the device in order to recover.
 */
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *fatal_err_event)
{

	switch (fatal_err_event->Id) {
	case SL_DEVICE_EVENT_FATAL_DEVICE_ABORT:
		SYS_LOG_ERR("\n[ERROR] - FATAL ERROR: "
			    "Abort NWP event detected: "
			    "AbortType=%ld, AbortData=0x%lx",
			    fatal_err_event->Data.DeviceAssert.Code,
			    fatal_err_event->Data.DeviceAssert.Value);
		break;

	case SL_DEVICE_EVENT_FATAL_DRIVER_ABORT:
		SYS_LOG_ERR("\n[ERROR] - FATAL ERROR: Driver Abort detected.");
		break;

	case SL_DEVICE_EVENT_FATAL_NO_CMD_ACK:
		SYS_LOG_ERR("\n[ERROR] - FATAL ERROR: No Cmd Ack detected "
			    "[cmd opcode = 0x%lx]",
			    fatal_err_event->Data.NoCmdAck.Code);
		break;

	case SL_DEVICE_EVENT_FATAL_SYNC_LOSS:
		SYS_LOG_ERR("\n[ERROR] - FATAL ERROR: Sync loss detected");
		break;

	case SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT:
		SYS_LOG_ERR("\n[ERROR] - FATAL ERROR: "
			    "Async event timeout detected "
			    "[event opcode =0x%lx]",
			    fatal_err_event->Data.CmdTimeout.Code);
		break;

	default:
		SYS_LOG_ERR("\n[ERROR] - FATAL ERROR: "
			    "Unspecified error detected");
		break;
	}
}

/* Unused, but must be defined to link.	 */
void SimpleLinkSockEventHandler(SlSockEvent_t *psock)
{
	ARG_UNUSED(psock);
}

/* Unused, but must be defined to link.	 */
void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *http_event,
				      SlNetAppHttpServerResponse_t *http_resp)
{
	ARG_UNUSED(http_event);
	ARG_UNUSED(http_resp);
}


/* Unused, but must be defined to link.	 */
void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *netapp_request,
					 SlNetAppResponse_t *netapp_response)
{
	ARG_UNUSED(netapp_request);
	ARG_UNUSED(netapp_response);
}

/* Unused, but must be defined to link.	 */
void SimpleLinkNetAppRequestMemFreeEventHandler(u8_t *buffer)
{
	ARG_UNUSED(buffer);
}

static long wlan_connect(void)
{
	SlWlanSecParams_t secParams = { 0 };
	long lretval = 0;

	secParams.Key = (signed char *)SECURITY_KEY;
	secParams.KeyLen = strlen(SECURITY_KEY);
	secParams.Type = SECURITY_TYPE;

	lretval = sl_WlanConnect((signed char *)SSID_NAME,
				 (sizeof(SSID_NAME) - 1), 0, &secParams, 0);

	if (!lretval) {
		/* Wait for WLAN connect: */
		k_sem_take(&nwp.conn.connect_sem, K_FOREVER);
		/* Wait for IP4 address: */
		k_sem_take(&nwp.conn.ip4acquire_sem, K_FOREVER);
	}

	return lretval;
}

static void simplelink_support_init(void)
{
	long retval = 0;

	nwp.status = 0;
	nwp.role = ROLE_RESERVED;

	memset(&nwp.conn, 0x0, sizeof(nwp.conn));

	k_sem_init(&nwp.conn.connect_sem,    0, 1);
	k_sem_init(&nwp.conn.ip4acquire_sem, 0, 1);
	k_sem_init(&nwp.conn.ip6acquire_sem, 0, 1);

	retval = configure_simplelink();
	__ASSERT(retval >= 0, "Unable to configure SimpleLink");

	retval = wlan_connect();
	__ASSERT(retval >= 0, "Unable to connect");
}

void simplelink_init(void)
{
	/* Init the board: */
	CC3220SF_LAUNCHXL_init();

	/* Configure SimpleLink NWP and await wlan connection: */
	simplelink_support_init();
}

