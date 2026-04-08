/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Contains various structures and bitmask defined in the 802.11 specification. Since the 802.11 is
 * huge, only a small subset is reproduced here.
 */
#ifndef SIWX91X_IEEEE80211_H
#define SIWX91X_IEEEE80211_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/net/wifi.h>

/* Masks and flags for Frame Control field */
#define IEEE80211_FCTL_VERS                0x0003
#define IEEE80211_FCTL_FTYPE               0x000c
#define IEEE80211_FCTL_STYPE               0x00f0
#define IEEE80211_FCTL_TODS                0x0100
#define IEEE80211_FCTL_FROMDS              0x0200
#define IEEE80211_FCTL_MOREFRAGS           0x0400
#define IEEE80211_FCTL_RETRY               0x0800
#define IEEE80211_FCTL_PM                  0x1000
#define IEEE80211_FCTL_MOREDATA            0x2000
#define IEEE80211_FCTL_PROTECTED           0x4000
#define IEEE80211_FCTL_ORDER               0x8000

/* Frame types (see IEEE80211_FCTL_FTYPE) */
enum {
	IEEE80211_FTYPE_MGMT  = 0,
	IEEE80211_FTYPE_CTL   = 1,
	IEEE80211_FTYPE_DATA  = 2,
	IEEE80211_FTYPE_EXT   = 3,
};

/* Subtype for management frame (see IEEE80211_FCTL_STYPE) */
enum {
	IEEE80211_STYPE_ASSOC_REQ          = 0x0,
	IEEE80211_STYPE_ASSOC_RESP         = 0x1,
	IEEE80211_STYPE_REASSOC_REQ        = 0x2,
	IEEE80211_STYPE_REASSOC_RESP       = 0x3,
	IEEE80211_STYPE_PROBE_REQ          = 0x4,
	IEEE80211_STYPE_PROBE_RESP         = 0x5,
	IEEE80211_STYPE_BEACON             = 0x8,
	IEEE80211_STYPE_ATIM               = 0x9,
	IEEE80211_STYPE_DISASSOC           = 0xA,
	IEEE80211_STYPE_AUTH               = 0xB,
	IEEE80211_STYPE_DEAUTH             = 0xC,
	IEEE80211_STYPE_ACTION             = 0xD,
};

/* Subtype for control frame (see IEEE80211_FCTL_STYPE) */
enum {
	IEEE80211_STYPE_TRIGGER            = 0x2,
	IEEE80211_STYPE_CTL_EXT            = 0x6,
	IEEE80211_STYPE_BACK_REQ           = 0x8,
	IEEE80211_STYPE_BACK               = 0x9,
	IEEE80211_STYPE_PSPOLL             = 0xA,
	IEEE80211_STYPE_RTS                = 0xB,
	IEEE80211_STYPE_CTS                = 0xC,
	IEEE80211_STYPE_ACK                = 0xD,
	IEEE80211_STYPE_CFEND              = 0xE,
	IEEE80211_STYPE_CFENDACK           = 0xF,
};

/* Subtype for data frame (see IEEE80211_FCTL_STYPE) */
enum {
	IEEE80211_STYPE_DATA               = 0x0,
	IEEE80211_STYPE_DATA_CFACK         = 0x1,
	IEEE80211_STYPE_DATA_CFPOLL        = 0x2,
	IEEE80211_STYPE_DATA_CFACKPOLL     = 0x3,
	IEEE80211_STYPE_NULLFUNC           = 0x4,
	IEEE80211_STYPE_CFACK              = 0x5,
	IEEE80211_STYPE_CFPOLL             = 0x6,
	IEEE80211_STYPE_CFACKPOLL          = 0x7,
	IEEE80211_STYPE_QOS_DATA           = 0x8,
	IEEE80211_STYPE_QOS_DATA_CFACK     = 0x9,
	IEEE80211_STYPE_QOS_DATA_CFPOLL    = 0xA,
	IEEE80211_STYPE_QOS_DATA_CFACKPOLL = 0xB,
	IEEE80211_STYPE_QOS_NULLFUNC       = 0xC,
	IEEE80211_STYPE_QOS_CFACK          = 0xD,
	IEEE80211_STYPE_QOS_CFPOLL         = 0xE,
	IEEE80211_STYPE_QOS_CFACKPOLL      = 0xF,
};

/* Capability Information field */
enum {
	WLAN_CAPABILITY_ESS                = BIT(0),
	WLAN_CAPABILITY_IBSS               = BIT(1),
	WLAN_CAPABILITY_CF_POLLABLE        = BIT(2),
	WLAN_CAPABILITY_CF_POLL_REQUEST    = BIT(3),
	WLAN_CAPABILITY_PRIVACY            = BIT(4),
	WLAN_CAPABILITY_SHORT_PREAMBLE     = BIT(5),
	WLAN_CAPABILITY_PBCC               = BIT(6),
	WLAN_CAPABILITY_CHANNEL_AGILITY    = BIT(7),
	WLAN_CAPABILITY_SPECTRUM_MGMT      = BIT(8),
	WLAN_CAPABILITY_QOS                = BIT(9),
	WLAN_CAPABILITY_SHORT_SLOT_TIME    = BIT(10),
	WLAN_CAPABILITY_APSD               = BIT(11),
	WLAN_CAPABILITY_RADIO_MEASURE      = BIT(12),
	WLAN_CAPABILITY_DSSS_OFDM          = BIT(13),
	WLAN_CAPABILITY_DEL_BACK           = BIT(14),
	WLAN_CAPABILITY_IMM_BACK           = BIT(15),
};

/* Only a subset of interresting IEs */
enum ieee80211_eid {
	WLAN_EID_SSID = 0,
	WLAN_EID_RSN = 48,
};

/* Information Element */
struct ieee80211_element {
	uint8_t id;
	uint8_t datalen;
	uint8_t data[];
} __packed;

/* Can also used for probe response */
struct ieee80211_beacon {
	uint16_t frame_control;
	uint16_t duration_id;
	uint8_t  da[WIFI_MAC_ADDR_LEN];
	uint8_t  sa[WIFI_MAC_ADDR_LEN];
	uint8_t  bssid[WIFI_MAC_ADDR_LEN];
	uint16_t seq_ctrl;
	uint64_t timestamp;
	uint16_t beacon_int;
	uint16_t capab_info;
	uint8_t  ies[];
} __packed __aligned(2);

static inline bool ieee80211_is_beacon(uint16_t fc)
{
	if (FIELD_GET(IEEE80211_FCTL_FTYPE, fc) != IEEE80211_FTYPE_MGMT) {
		return false;
	}
	if (FIELD_GET(IEEE80211_FCTL_STYPE, fc) != IEEE80211_STYPE_BEACON) {
		return false;
	}
	return true;
}

static inline bool ieee80211_is_probe_resp(uint16_t fc)
{
	if (FIELD_GET(IEEE80211_FCTL_FTYPE, fc) != IEEE80211_FTYPE_MGMT) {
		return false;
	}
	if (FIELD_GET(IEEE80211_FCTL_STYPE, fc) != IEEE80211_STYPE_PROBE_RESP) {
		return false;
	}
	return true;
}

#endif
