/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2022 Florian Grandel.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Packet data common to all IEEE 802.15.4 L2 layers
 *
 * All references to the spec refer to IEEE 802.15.4-2020.
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

/* See section 6.16.2.8 - Received Signal Strength Indicator (RSSI) */
#define IEEE802154_MAC_RSSI_MIN       0U   /* corresponds to -174 dBm */
#define IEEE802154_MAC_RSSI_MAX       254U /* corresponds to 80 dBm */
#define IEEE802154_MAC_RSSI_UNDEFINED 255U /* used by us to indicate an undefined RSSI value */

#define IEEE802154_MAC_RSSI_DBM_MIN       -174      /* in dBm */
#define IEEE802154_MAC_RSSI_DBM_MAX       80        /* in dBm */
#define IEEE802154_MAC_RSSI_DBM_UNDEFINED INT16_MIN /* represents an undefined RSSI value */

struct net_pkt_cb_ieee802154 {
#if defined(CONFIG_NET_L2_OPENTHREAD)
	uint32_t ack_fc;   /* Frame counter set in the ACK */
	uint8_t ack_keyid; /* Key index set in the ACK */
#endif
	union {
		/* RX packets */
		struct {
			uint8_t lqi;  /* Link Quality Indicator */
			/* See section 6.16.2.8 - Received Signal Strength Indicator (RSSI)
			 * "RSSI is represented as one octet of integer [...]; therefore,
			 * the minimum and maximum values are 0 (–174 dBm) and 254 (80 dBm),
			 * respectively. 255 is reserved." (MAC PIB attribute macRssi, see
			 * section 8.4.3.10, table 8-108)
			 *
			 * TX packets will show zero for this value. Drivers may set the
			 * field to the reserved value 255 (0xff) to indicate that an RSSI
			 * value is not available for this packet.
			 */
			uint8_t rssi;
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
#if defined(CONFIG_NET_L2_OPENTHREAD)
	uint8_t fv2015: 1;   /* Frame version is IEEE 802.15.4 (0b10),
			      * see section 7.2.2.10, table 7-3.
			      */
	uint8_t ack_seb : 1; /* Security Enabled Bit was set in the ACK */
#endif
};

struct net_pkt;
static inline void *net_pkt_cb(struct net_pkt *pkt);

static inline struct net_pkt_cb_ieee802154 *net_pkt_cb_ieee802154(struct net_pkt *pkt)
{
	return (struct net_pkt_cb_ieee802154 *)net_pkt_cb(pkt);
};

static inline uint8_t net_pkt_ieee802154_lqi(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->lqi;
}

static inline void net_pkt_set_ieee802154_lqi(struct net_pkt *pkt, uint8_t lqi)
{
	net_pkt_cb_ieee802154(pkt)->lqi = lqi;
}

/**
 * @brief Get the unsigned RSSI value as defined in section 6.16.2.8,
 *        Received Signal Strength Indicator (RSSI)
 *
 * @param pkt Pointer to the packet.
 *
 * @returns RSSI represented as unsigned byte value, ranging from
 *          0 (–174 dBm) to 254 (80 dBm).
 *          The special value 255 (IEEE802154_MAC_RSSI_UNDEFINED)
 *          indicates that an RSSI value is not available for this
 *          packet. Will return zero for packets on the TX path.
 */
static inline uint8_t net_pkt_ieee802154_rssi(struct net_pkt *pkt)
{
	return net_pkt_cb_ieee802154(pkt)->rssi;
}

/**
 * @brief Set the unsigned RSSI value as defined in section 6.16.2.8,
 *        Received Signal Strength Indicator (RSSI).
 *
 * @param pkt Pointer to the packet that was received with the given
 *            RSSI.
 * @param rssi RSSI represented as unsigned byte value, ranging from
 *             0 (–174 dBm) to 254 (80 dBm).
 *             The special value 255 (IEEE802154_MAC_RSSI_UNDEFINED)
 *             indicates that an RSSI value is not available for this
 *             packet.
 */
static inline void net_pkt_set_ieee802154_rssi(struct net_pkt *pkt, uint8_t rssi)
{
	net_pkt_cb_ieee802154(pkt)->rssi = rssi;
}

/**
 * @brief Get a signed RSSI value measured in dBm.
 *
 * @param pkt Pointer to the packet.
 *
 * @returns RSSI represented in dBm. Returns the special value
 *          IEEE802154_MAC_RSSI_DBM_UNDEFINED if an RSSI value
 *          is not available for this packet. Packets on the TX
 *          path will always show -174 dBm (which corresponds to
 *          an internal value of unsigned zero).
 */
static inline int16_t net_pkt_ieee802154_rssi_dbm(struct net_pkt *pkt)
{
	int16_t rssi = net_pkt_cb_ieee802154(pkt)->rssi;
	return rssi == IEEE802154_MAC_RSSI_UNDEFINED ? IEEE802154_MAC_RSSI_DBM_UNDEFINED
						     : rssi + IEEE802154_MAC_RSSI_DBM_MIN;
}

/**
 * @brief Set the RSSI value as a signed integer measured in dBm.
 *
 * @param pkt Pointer to the packet that was received with the given
 *            RSSI.
 * @param rssi represented in dBm. Set to the special value
 *             IEEE802154_MAC_RSSI_DBM_UNDEFINED if an RSSI value is
 *             not available for this packet. Values above 80 dBm will
 *             be mapped to 80 dBm, values below -174 dBm will be mapped
 *             to -174 dBm.
 */
static inline void net_pkt_set_ieee802154_rssi_dbm(struct net_pkt *pkt, int16_t rssi)
{
	if (likely(rssi >= IEEE802154_MAC_RSSI_DBM_MIN && rssi <= IEEE802154_MAC_RSSI_DBM_MAX)) {
		int16_t unsigned_rssi = rssi - IEEE802154_MAC_RSSI_DBM_MIN;

		net_pkt_cb_ieee802154(pkt)->rssi = unsigned_rssi;
		return;
	} else if (rssi == IEEE802154_MAC_RSSI_DBM_UNDEFINED) {
		net_pkt_cb_ieee802154(pkt)->rssi = IEEE802154_MAC_RSSI_UNDEFINED;
		return;
	} else if (rssi < IEEE802154_MAC_RSSI_DBM_MIN) {
		net_pkt_cb_ieee802154(pkt)->rssi = IEEE802154_MAC_RSSI_MIN;
		return;
	} else if (rssi > IEEE802154_MAC_RSSI_DBM_MAX) {
		net_pkt_cb_ieee802154(pkt)->rssi = IEEE802154_MAC_RSSI_MAX;
		return;
	}

	CODE_UNREACHABLE;
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

#if defined(CONFIG_NET_L2_OPENTHREAD)
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
#endif /* CONFIG_NET_L2_OPENTHREAD */

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_PKT_H_ */
