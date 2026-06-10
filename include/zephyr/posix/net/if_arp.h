/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linux-compatible ARP hardware type definitions.
 * @ingroup posix_extensions
 *
 * @compat_api{This header provides non-POSIX compatibility interfaces.}
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

/**
 * @brief Linux ARP hardware type ARPHRD_NETROM.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_NETROM}
 */
#define ARPHRD_NETROM     0            /* From KA9Q: NET/ROM pseudo. */

/**
 * @brief Linux ARP hardware type ARPHRD_ETHER.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_ETHER}
 */
#define ARPHRD_ETHER      1            /* Ethernet 10/100Mbps.  */

/**
 * @brief Linux ARP hardware type ARPHRD_EETHER.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_EETHER}
 */
#define ARPHRD_EETHER     2            /* Experimental Ethernet.  */

/**
 * @brief Linux ARP hardware type ARPHRD_AX25.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_AX25}
 */
#define ARPHRD_AX25       3            /* AX.25 Level 2.  */

/**
 * @brief Linux ARP hardware type ARPHRD_PRONET.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_PRONET}
 */
#define ARPHRD_PRONET     4            /* PROnet token ring.  */

/**
 * @brief Linux ARP hardware type ARPHRD_CHAOS.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_CHAOS}
 */
#define ARPHRD_CHAOS      5            /* Chaosnet.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IEEE802.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IEEE802}
 */
#define ARPHRD_IEEE802    6            /* IEEE 802.2 Ethernet/TR/TB.  */

/**
 * @brief Linux ARP hardware type ARPHRD_ARCNET.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_ARCNET}
 */
#define ARPHRD_ARCNET     7            /* ARCnet.  */

/**
 * @brief Linux ARP hardware type ARPHRD_APPLETLK.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_APPLETLK}
 */
#define ARPHRD_APPLETLK   8            /* APPLEtalk.  */

/**
 * @brief Linux ARP hardware type ARPHRD_DLCI.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_DLCI}
 */
#define ARPHRD_DLCI       15           /* Frame Relay DLCI.  */

/**
 * @brief Linux ARP hardware type ARPHRD_ATM.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_ATM}
 */
#define ARPHRD_ATM        19           /* ATM.  */

/**
 * @brief Linux ARP hardware type ARPHRD_METRICOM.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_METRICOM}
 */
#define ARPHRD_METRICOM   23           /* Metricom STRIP (new IANA id).  */

/**
 * @brief Linux ARP hardware type ARPHRD_IEEE1394.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IEEE1394}
 */
#define ARPHRD_IEEE1394   24           /* IEEE 1394 IPv4 - RFC 2734.  */

/**
 * @brief Linux ARP hardware type ARPHRD_EUI64.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_EUI64}
 */
#define ARPHRD_EUI64      27           /* EUI-64.  */

/**
 * @brief Linux ARP hardware type ARPHRD_INFINIBAND.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_INFINIBAND}
 */
#define ARPHRD_INFINIBAND 32           /* InfiniBand.  */

/* Dummy types for non ARP hardware */

/**
 * @brief Linux ARP hardware type ARPHRD_SLIP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_SLIP}
 */
#define ARPHRD_SLIP       256

/**
 * @brief Linux ARP hardware type ARPHRD_CSLIP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_CSLIP}
 */
#define ARPHRD_CSLIP      257

/**
 * @brief Linux ARP hardware type ARPHRD_SLIP6.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_SLIP6}
 */
#define ARPHRD_SLIP6      258

/**
 * @brief Linux ARP hardware type ARPHRD_CSLIP6.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_CSLIP6}
 */
#define ARPHRD_CSLIP6     259

/**
 * @brief Linux ARP hardware type ARPHRD_RSRVD.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_RSRVD}
 */
#define ARPHRD_RSRVD      260          /* Notional KISS type.  */

/**
 * @brief Linux ARP hardware type ARPHRD_ADAPT.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_ADAPT}
 */
#define ARPHRD_ADAPT      264

/**
 * @brief Linux ARP hardware type ARPHRD_ROSE.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_ROSE}
 */
#define ARPHRD_ROSE       270

/**
 * @brief Linux ARP hardware type ARPHRD_X25.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_X25}
 */
#define ARPHRD_X25        271          /* CCITT X.25.  */

/**
 * @brief Linux ARP hardware type ARPHRD_HWX25.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_HWX25}
 */
#define ARPHRD_HWX25      272          /* Boards with X.25 in firmware.  */

/**
 * @brief Linux ARP hardware type ARPHRD_CAN.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_CAN}
 */
#define ARPHRD_CAN        280          /* Controller Area Network.  */

/**
 * @brief Linux ARP hardware type ARPHRD_MCTP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_MCTP}
 */
#define ARPHRD_MCTP       290

/**
 * @brief Linux ARP hardware type ARPHRD_PPP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_PPP}
 */
#define ARPHRD_PPP        512

/**
 * @brief Linux ARP hardware type ARPHRD_CISCO.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_CISCO}
 */
#define ARPHRD_CISCO      513          /* Cisco HDLC.  */

/**
 * @brief Linux ARP hardware type ARPHRD_HDLC.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_HDLC}
 */
#define ARPHRD_HDLC       ARPHRD_CISCO

/**
 * @brief Linux ARP hardware type ARPHRD_LAPB.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_LAPB}
 */
#define ARPHRD_LAPB       516          /* LAPB.  */

/**
 * @brief Linux ARP hardware type ARPHRD_DDCMP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_DDCMP}
 */
#define ARPHRD_DDCMP      517          /* Digital's DDCMP.  */

/**
 * @brief Linux ARP hardware type ARPHRD_RAWHDLC.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_RAWHDLC}
 */
#define ARPHRD_RAWHDLC    518          /* Raw HDLC.  */

/**
 * @brief Linux ARP hardware type ARPHRD_RAWIP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_RAWIP}
 */
#define ARPHRD_RAWIP      519          /* Raw IP.  */

/**
 * @brief Linux ARP hardware type ARPHRD_TUNNEL.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_TUNNEL}
 */
#define ARPHRD_TUNNEL     768          /* IPIP tunnel.  */

/**
 * @brief Linux ARP hardware type ARPHRD_TUNNEL6.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_TUNNEL6}
 */
#define ARPHRD_TUNNEL6    769          /* IPIP6 tunnel.  */

/**
 * @brief Linux ARP hardware type ARPHRD_FRAD.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_FRAD}
 */
#define ARPHRD_FRAD       770          /* Frame Relay Access Device.  */

/**
 * @brief Linux ARP hardware type ARPHRD_SKIP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_SKIP}
 */
#define ARPHRD_SKIP       771          /* SKIP vif.  */

/**
 * @brief Linux ARP hardware type ARPHRD_LOOPBACK.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_LOOPBACK}
 */
#define ARPHRD_LOOPBACK   772          /* Loopback device.  */

/**
 * @brief Linux ARP hardware type ARPHRD_LOCALTLK.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_LOCALTLK}
 */
#define ARPHRD_LOCALTLK   773          /* Localtalk device.  */

/**
 * @brief Linux ARP hardware type ARPHRD_FDDI.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_FDDI}
 */
#define ARPHRD_FDDI       774          /* Fiber Distributed Data Interface. */

/**
 * @brief Linux ARP hardware type ARPHRD_BIF.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_BIF}
 */
#define ARPHRD_BIF        775          /* AP1000 BIF.  */

/**
 * @brief Linux ARP hardware type ARPHRD_SIT.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_SIT}
 */
#define ARPHRD_SIT        776          /* sit0 device - IPv6-in-IPv4.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IPDDP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IPDDP}
 */
#define ARPHRD_IPDDP      777          /* IP-in-DDP tunnel.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IPGRE.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IPGRE}
 */
#define ARPHRD_IPGRE      778          /* GRE over IP.  */

/**
 * @brief Linux ARP hardware type ARPHRD_PIMREG.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_PIMREG}
 */
#define ARPHRD_PIMREG     779          /* PIMSM register interface.  */

/**
 * @brief Linux ARP hardware type ARPHRD_HIPPI.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_HIPPI}
 */
#define ARPHRD_HIPPI      780          /* High Performance Parallel I'face. */

/**
 * @brief Linux ARP hardware type ARPHRD_ASH.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_ASH}
 */
#define ARPHRD_ASH        781          /* (Nexus Electronics) Ash.  */

/**
 * @brief Linux ARP hardware type ARPHRD_ECONET.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_ECONET}
 */
#define ARPHRD_ECONET     782          /* Acorn Econet.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IRDA.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IRDA}
 */
#define ARPHRD_IRDA       783          /* Linux-IrDA.  */

/**
 * @brief Linux ARP hardware type ARPHRD_FCPP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_FCPP}
 */
#define ARPHRD_FCPP       784          /* Point to point fibrechanel.  */

/**
 * @brief Linux ARP hardware type ARPHRD_FCAL.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_FCAL}
 */
#define ARPHRD_FCAL       785          /* Fibrechanel arbitrated loop.  */

/**
 * @brief Linux ARP hardware type ARPHRD_FCPL.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_FCPL}
 */
#define ARPHRD_FCPL       786          /* Fibrechanel public loop.  */

/**
 * @brief Linux ARP hardware type ARPHRD_FCFABRIC.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_FCFABRIC}
 */
#define ARPHRD_FCFABRIC   787          /* Fibrechanel fabric.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IEEE802_TR.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IEEE802_TR}
 */
#define ARPHRD_IEEE802_TR 800          /* Magic type ident for TR.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IEEE80211.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IEEE80211}
 */
#define ARPHRD_IEEE80211  801          /* IEEE 802.11.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IEEE80211_PRISM.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IEEE80211_PRISM}
 */
#define ARPHRD_IEEE80211_PRISM    802  /* IEEE 802.11 + Prism2 header.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IEEE80211_RADIOTAP.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IEEE80211_RADIOTAP}
 */
#define ARPHRD_IEEE80211_RADIOTAP 803  /* IEEE 802.11 + radiotap header.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IEEE802154.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IEEE802154}
 */
#define ARPHRD_IEEE802154         804  /* IEEE 802.15.4 header.  */

/**
 * @brief Linux ARP hardware type ARPHRD_IEEE802154_PHY.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_IEEE802154_PHY}
 */
#define ARPHRD_IEEE802154_PHY     805  /* IEEE 802.15.4 PHY header.  */

/**
 * @brief Linux ARP hardware type ARPHRD_VOID.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_VOID}
 */
#define ARPHRD_VOID       0xFFFF       /* Void type, nothing is known.  */

/**
 * @brief Linux ARP hardware type ARPHRD_NONE.
 * @ingroup posix_extensions
 *
 * @compat_api{ARPHRD_NONE}
 */
#define ARPHRD_NONE       0xFFFE       /* Zero header length.  */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_NET_IF_ARP_H_ */
