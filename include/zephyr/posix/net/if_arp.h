/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_NET_IF_ARP_H_
#define ZEPHYR_INCLUDE_POSIX_NET_IF_ARP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* See https://www.iana.org/assignments/arp-parameters/arp-parameters.xhtml
 * for the ARP hardware address type values.
 */
/* ARP protocol HARDWARE identifiers. */
#define ARPHRD_NETROM     0            /* From KA9Q: NET/ROM pseudo. */
#define ARPHRD_ETHER      1            /* Ethernet 10/100Mbps.  */
#define ARPHRD_EETHER     2            /* Experimental Ethernet.  */
#define ARPHRD_AX25       3            /* AX.25 Level 2.  */
#define ARPHRD_PRONET     4            /* PROnet token ring.  */
#define ARPHRD_CHAOS      5            /* Chaosnet.  */
#define ARPHRD_IEEE802    6            /* IEEE 802.2 Ethernet/TR/TB.  */
#define ARPHRD_ARCNET     7            /* ARCnet.  */
#define ARPHRD_APPLETLK   8            /* APPLEtalk.  */
#define ARPHRD_DLCI       15           /* Frame Relay DLCI.  */
#define ARPHRD_ATM        19           /* ATM.  */
#define ARPHRD_METRICOM   23           /* Metricom STRIP (new IANA id).  */
#define ARPHRD_IEEE1394   24           /* IEEE 1394 IPv4 - RFC 2734.  */
#define ARPHRD_EUI64      27           /* EUI-64.  */
#define ARPHRD_INFINIBAND 32           /* InfiniBand.  */

/* Dummy types for non ARP hardware */
#define ARPHRD_SLIP       256
#define ARPHRD_CSLIP      257
#define ARPHRD_SLIP6      258
#define ARPHRD_CSLIP6     259
#define ARPHRD_RSRVD      260          /* Notional KISS type.  */
#define ARPHRD_ADAPT      264
#define ARPHRD_ROSE       270
#define ARPHRD_X25        271          /* CCITT X.25.  */
#define ARPHRD_HWX25      272          /* Boards with X.25 in firmware.  */
#define ARPHRD_CAN        280          /* Controller Area Network.  */
#define ARPHRD_MCTP       290
#define ARPHRD_PPP        512
#define ARPHRD_CISCO      513          /* Cisco HDLC.  */
#define ARPHRD_HDLC       ARPHRD_CISCO
#define ARPHRD_LAPB       516          /* LAPB.  */
#define ARPHRD_DDCMP      517          /* Digital's DDCMP.  */
#define ARPHRD_RAWHDLC    518          /* Raw HDLC.  */
#define ARPHRD_RAWIP      519          /* Raw IP.  */
#define ARPHRD_TUNNEL     768          /* IPIP tunnel.  */
#define ARPHRD_TUNNEL6    769          /* IPIP6 tunnel.  */
#define ARPHRD_FRAD       770          /* Frame Relay Access Device.  */
#define ARPHRD_SKIP       771          /* SKIP vif.  */
#define ARPHRD_LOOPBACK   772          /* Loopback device.  */
#define ARPHRD_LOCALTLK   773          /* Localtalk device.  */
#define ARPHRD_FDDI       774          /* Fiber Distributed Data Interface. */
#define ARPHRD_BIF        775          /* AP1000 BIF.  */
#define ARPHRD_SIT        776          /* sit0 device - IPv6-in-IPv4.  */
#define ARPHRD_IPDDP      777          /* IP-in-DDP tunnel.  */
#define ARPHRD_IPGRE      778          /* GRE over IP.  */
#define ARPHRD_PIMREG     779          /* PIMSM register interface.  */
#define ARPHRD_HIPPI      780          /* High Performance Parallel I'face. */
#define ARPHRD_ASH        781          /* (Nexus Electronics) Ash.  */
#define ARPHRD_ECONET     782          /* Acorn Econet.  */
#define ARPHRD_IRDA       783          /* Linux-IrDA.  */
#define ARPHRD_FCPP       784          /* Point to point fibrechanel.  */
#define ARPHRD_FCAL       785          /* Fibrechanel arbitrated loop.  */
#define ARPHRD_FCPL       786          /* Fibrechanel public loop.  */
#define ARPHRD_FCFABRIC   787          /* Fibrechanel fabric.  */
#define ARPHRD_IEEE802_TR 800          /* Magic type ident for TR.  */
#define ARPHRD_IEEE80211  801          /* IEEE 802.11.  */
#define ARPHRD_IEEE80211_PRISM    802  /* IEEE 802.11 + Prism2 header.  */
#define ARPHRD_IEEE80211_RADIOTAP 803  /* IEEE 802.11 + radiotap header.  */
#define ARPHRD_IEEE802154         804  /* IEEE 802.15.4 header.  */
#define ARPHRD_IEEE802154_PHY     805  /* IEEE 802.15.4 PHY header.  */

#define ARPHRD_VOID       0xFFFF       /* Void type, nothing is known.  */
#define ARPHRD_NONE       0xFFFE       /* Zero header length.  */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_NET_IF_ARP_H_ */
