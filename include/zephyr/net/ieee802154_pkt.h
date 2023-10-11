/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2022 Florian Grandel.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Packet data common to all IEEE 802.15.4 L2 layers
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_PKT_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_PKT_H_

#include <string.h>

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @cond ignore */

#ifndef NET_PKT_HAS_CONTROL_BLOCK
#define NET_PKT_HAS_CONTROL_BLOCK
#endif

struct net_pkt_cb_ieee802154 {
#if defined(CONFIG_IEEE802154_2015)
	uint32_t ack_fc;   /* Frame counter set in the ACK */
	uint8_t ack_keyid; /* Key index set in the ACK */
#endif
	union {
		/* RX packets */
		struct {
			uint8_t lqi;  /* Link Quality Indicator */
			uint8_t rssi; /* Received Signal Strength Indication */
		};
#if defined(CONFIG_IEEE802154_SELECTIVE_TXPOWER)
		/* TX packets */
		struct {
			int8_t txpwr; /* TX power in dBm. */
		};
#endif /* CONFIG_IEEE802154_SELECTIVE_TXPOWER */
	};

	/* Flags */
	uint8_t arb : 1;	   /* ACK Request Bit is set in the frame */
	uint8_t ack_fpb : 1;	   /* Frame Pending Bit was set in the ACK */
	uint8_t frame_secured : 1; /* Frame is authenticated and
				    * encrypted according to its
				    * Auxiliary Security Header
				    */
	uint8_t mac_hdr_rdy : 1;   /* Indicates if frame's MAC header
				    * is ready to be transmitted or if
				    * it requires further modifications,
				    * e.g. Frame Counter injection.
				    */
#if defined(CONFIG_IEEE802154_2015)
	uint8_t fv2015 : 1;  /* Frame version is IEEE 802.15.4-2015 or later */
	uint8_t ack_seb : 1; /* Security Enabled Bit was set in the ACK */
#endif
};

struct net_pkt;
static inline void *net_pkt_cb(struct net_pkt *pkt);

static inline struct net_pkt_cb_ieee802154 *net_pkt_cb_ieee802154(struct net_pkt *pkt)
{
	return net_pkt_cb(pkt);
};

static inline uint8_t net_pkt_ieee802154_lqi(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->lqi;
}

static inline void net_pkt_set_ieee802154_lqi(struct net_pkt *pkt, uint8_t lqi)
{
	net_pkt_cb_ieee802154(pkt)->lqi = lqi;
}

static inline uint8_t net_pkt_ieee802154_rssi(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->rssi;
}

static inline void net_pkt_set_ieee802154_rssi(struct net_pkt *pkt, uint8_t rssi)
{
	net_pkt_cb_ieee802154(pkt)->rssi = rssi;
}

#if defined(CONFIG_IEEE802154_SELECTIVE_TXPOWER)
static inline int8_t net_pkt_ieee802154_txpwr(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->txpwr;
}

static inline void net_pkt_set_ieee802154_txpwr(struct net_pkt *pkt, int8_t txpwr)
{
	net_pkt_cb_ieee802154(pkt)->txpwr = txpwr;
}
#endif /* CONFIG_IEEE802154_SELECTIVE_TXPOWER */

static inline bool net_pkt_ieee802154_arb(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->arb;
}

static inline void net_pkt_set_ieee802154_arb(struct net_pkt *pkt, bool arb)
{
	net_pkt_cb_ieee802154(pkt)->arb = arb;
}

static inline bool net_pkt_ieee802154_ack_fpb(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->ack_fpb;
}

static inline void net_pkt_set_ieee802154_ack_fpb(struct net_pkt *pkt, bool fpb)
{
	net_pkt_cb_ieee802154(pkt)->ack_fpb = fpb;
}

static inline bool net_pkt_ieee802154_frame_secured(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->frame_secured;
}

static inline void net_pkt_set_ieee802154_frame_secured(struct net_pkt *pkt, bool secured)
{
	net_pkt_cb_ieee802154(pkt)->frame_secured = secured;
}

static inline bool net_pkt_ieee802154_mac_hdr_rdy(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->mac_hdr_rdy;
}

static inline void net_pkt_set_ieee802154_mac_hdr_rdy(struct net_pkt *pkt, bool rdy)
{
	net_pkt_cb_ieee802154(pkt)->mac_hdr_rdy = rdy;
}

#if defined(CONFIG_IEEE802154_2015)
static inline uint32_t net_pkt_ieee802154_ack_fc(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->ack_fc;
}

static inline void net_pkt_set_ieee802154_ack_fc(struct net_pkt *pkt, uint32_t fc)
{
	net_pkt_cb_ieee802154(pkt)->ack_fc = fc;
}

static inline uint8_t net_pkt_ieee802154_ack_keyid(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->ack_keyid;
}

static inline void net_pkt_set_ieee802154_ack_keyid(struct net_pkt *pkt, uint8_t keyid)
{
	net_pkt_cb_ieee802154(pkt)->ack_keyid = keyid;
}

static inline bool net_pkt_ieee802154_fv2015(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->fv2015;
}

static inline void net_pkt_set_ieee802154_fv2015(struct net_pkt *pkt, bool fv2015)
{
	net_pkt_cb_ieee802154(pkt)->fv2015 = fv2015;
}

static inline bool net_pkt_ieee802154_ack_seb(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->ack_seb;
}

static inline void net_pkt_set_ieee802154_ack_seb(struct net_pkt *pkt, bool seb)
{
	net_pkt_cb_ieee802154(pkt)->ack_seb = seb;
}
#endif /* CONFIG_IEEE802154_2015 */

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_PKT_H_ */
