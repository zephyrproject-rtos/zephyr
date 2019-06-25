/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_NET_PPP_H_
#define ZEPHYR_INCLUDE_NET_PPP_H_

#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>
#include <net/net_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Point-to-point (PPP) L2/driver support functions
 * @defgroup ppp PPP L2/driver Support Functions
 * @ingroup networking
 * @{
 */

/** PPP maximum receive unit (MRU) */
#define PPP_MRU 1500

/** PPP maximum transfer unit (MTU) */
#define PPP_MTU PPP_MRU

/** Max length of terminate description string */
#define PPP_MAX_TERMINATE_REASON_LEN 32

/** Length of network interface identifier */
#define PPP_INTERFACE_IDENTIFIER_LEN 8

/** PPP L2 API */
struct ppp_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Start the device */
	int (*start)(struct device *dev);

	/** Stop the device */
	int (*stop)(struct device *dev);

	/** Send a network packet */
	int (*send)(struct device *dev, struct net_pkt *pkt);

#if defined(CONFIG_NET_STATISTICS_PPP)
	/** Collect optional PPP specific statistics. This pointer
	 * should be set by driver if statistics needs to be collected
	 * for that driver.
	 */
	struct net_stats_ppp *(*get_stats)(struct device *dev);
#endif
};

/**
 * PPP protocol types.
 * See https://www.iana.org/assignments/ppp-numbers/ppp-numbers.xhtml
 * for details.
 */
enum ppp_protocol_type {
	PPP_IP     = 0x0021, /**< RFC 1332 */
	PPP_IPV6   = 0x0057, /**< RFC 5072 */
	PPP_IPCP   = 0x8021, /**< RFC 1332 */
	PPP_ECP    = 0x8053, /**< RFC 1968 */
	PPP_IPV6CP = 0x8057, /**< RFC 5072 */
	PPP_CCP    = 0x80FD, /**< RFC 1962 */
	PPP_LCP    = 0xc021, /**< RFC 1661 */
};

/**
 * PPP phases
 */
enum ppp_phase {
	/** Physical-layer not ready */
	PPP_DEAD,
	/** Link is being established */
	PPP_ESTABLISH,
	/** Link authentication with peer */
	PPP_AUTH,
	/** Network connection establishment */
	PPP_NETWORK,
	/** Network running */
	PPP_RUNNING,
	/** Link termination */
	PPP_TERMINATE,
};

/**
 * PPP states, RFC 1661 ch. 4.2
 */
enum ppp_state {
	PPP_INITIAL,
	PPP_STARTING,
	PPP_CLOSED,
	PPP_STOPPED,
	PPP_CLOSING,
	PPP_STOPPING,
	PPP_REQUEST_SENT,
	PPP_ACK_RECEIVED,
	PPP_ACK_SENT,
	PPP_OPENED
};

/**
 * PPP protocol operations from RFC 1661
 */
enum ppp_packet_type {
	PPP_CONFIGURE_REQ  = 1,
	PPP_CONFIGURE_ACK  = 2,
	PPP_CONFIGURE_NACK = 3,
	PPP_CONFIGURE_REJ  = 4,
	PPP_TERMINATE_REQ  = 5,
	PPP_TERMINATE_ACK  = 6,
	PPP_CODE_REJ       = 7,
	PPP_PROTOCOL_REJ   = 8,
	PPP_ECHO_REQ       = 9,
	PPP_ECHO_REPLY     = 10,
	PPP_DISCARD_REQ    = 11
};

/**
 * LCP option types from RFC 1661 ch. 6
 */
enum lcp_option_type {
	LCP_OPTION_RESERVED = 0,

	/** Maximum-Receive-Unit */
	LCP_OPTION_MRU = 1,

	/** Async-Control-Character-Map */
	LCP_OPTION_ASYNC_CTRL_CHAR_MAP = 2,

	/** Authentication-Protocol */
	LCP_OPTION_AUTH_PROTO = 3,

	/** Quality-Protocol */
	LCP_OPTION_QUALITY_PROTO = 4,

	/** Magic-Number */
	LCP_OPTION_MAGIC_NUMBER = 5,

	/** Protocol-Field-Compression */
	LCP_OPTION_PROTO_COMPRESS = 7,

	/** Address-and-Control-Field-Compression */
	LCP_OPTION_ADDR_CTRL_COMPRESS = 8
} __packed;

/**
 * IPCP option types from RFC 1332
 */
enum ipcp_option_type {
	IPCP_OPTION_RESERVED = 0,

	/** IP Addresses */
	IPCP_OPTION_IP_ADDRESSES = 1,

	/** IP Compression Protocol */
	IPCP_OPTION_IP_COMP_PROTO = 2,

	/** IP Address */
	IPCP_OPTION_IP_ADDRESS = 3,
} __packed;

/**
 * IPV6CP option types from RFC 5072
 */
enum ipv6cp_option_type {
	IPV6CP_OPTION_RESERVED = 0,

	/** Interface identifier */
	IPV6CP_OPTION_INTERFACE_IDENTIFIER = 1,
} __packed;

/**
 * @typedef net_ppp_lcp_echo_reply_cb_t
 * @brief A callback function that can be called if a Echo-Reply needs to
 *        be received.
 * @param user_data User settable data that is passed to the callback
 *        function.
 * @param user_data_len Length of the user data.
 */
typedef void (*net_ppp_lcp_echo_reply_cb_t)(void *user_data,
					    size_t user_data_len);

/**
 * Generic PPP Finite State Machine
 */
struct ppp_fsm {
	/** Timeout timer */
	struct k_delayed_work timer;

	/* We need to send a packet from separate thread so that we do not
	 * receive reply before we are ready to receive it. The issue was seen
	 * with QEMU where the link to peer is so fast that we received the
	 * reply before the net_send_data() returned.
	 */
	struct {
		/** Packet sending timer. */
		struct k_delayed_work work;

		/** Packet to send */
		struct net_pkt *pkt;
	} sender;

	struct {
		/** Acknowledge Configuration Information */
		int (*config_info_ack)(struct ppp_fsm *fsm,
				       struct net_pkt *pkt,
				       u16_t length);

		/** Add Configuration Information */
		struct net_buf *(*config_info_add)(struct ppp_fsm *fsm);

		/** Length of Configuration Information */
		int  (*config_info_len)(struct ppp_fsm *fsm);

		/** Negative Acknowledge Configuration Information */
		int (*config_info_nack)(struct ppp_fsm *fsm,
					struct net_pkt *pkt,
					u16_t length,
					bool rejected);

		/** Request peer's Configuration Information */
		int (*config_info_req)(struct ppp_fsm *fsm,
				       struct net_pkt *pkt,
				       u16_t length,
				       struct net_buf **buf);

		/** Reject Configuration Information */
		int (*config_info_rej)(struct ppp_fsm *fsm,
				       struct net_pkt *pkt,
				       u16_t length);

		/** Reset Configuration Information */
		void (*config_info_reset)(struct ppp_fsm *fsm);

		/** FSM goes to OPENED state */
		void (*up)(struct ppp_fsm *fsm);

		/** FSM leaves OPENED state */
		void (*down)(struct ppp_fsm *fsm);

		/** Starting this protocol */
		void (*starting)(struct ppp_fsm *fsm);

		/** Quitting this protocol */
		void (*finished)(struct ppp_fsm *fsm);

		/** We received Protocol-Reject */
		void (*proto_reject)(struct ppp_fsm *fsm);

		/** Retransmit */
		void (*retransmit)(struct ppp_fsm *fsm);

		/** Any code that is not understood by PPP is passed to
		 * this FSM for further processing.
		 */
		enum net_verdict (*proto_extension)(struct ppp_fsm *fsm,
						    enum ppp_packet_type code,
						    u8_t id,
						    struct net_pkt *pkt);
	} cb;

	/** Option bits */
	u32_t flags;

	/** Number of re-transmissions left */;
	u32_t retransmits;

	/** Number of NACK loops since last ACK */
	u32_t nack_loops;

	/** Number of NACKs received */
	u32_t recv_nack_loops;

	/** Reason for closing protocol */
	char terminate_reason[PPP_MAX_TERMINATE_REASON_LEN];

	/** PPP protocol number for this FSM */
	u16_t protocol;

	/** Current state of PPP link */
	enum ppp_state state;

	/** Protocol/layer name of this FSM (for debugging) */
	const char *name;

	/** Current id */
	u8_t id;

	/** Current request id */
	u8_t req_id;

	/** Have received valid Ack, Nack or Reject to a Request */
	u8_t ack_received : 1;
};

/** PPP configuration options */
struct ppp_option_pkt {
	/** Option value */
	struct net_pkt_cursor value;

	/** Option type */
	union {
		enum lcp_option_type lcp;
		enum ipcp_option_type ipcp;
		enum ipv6cp_option_type ipv6cp;
	} type;

	/** Option length */
	u8_t len;
};

struct lcp_options {
	/** Magic number */
	u32_t magic;

	/** Async char map */
	u32_t async_map;

	/** Maximum Receive Unit value */
	u16_t mru;

	/* Flags what to negotiate */

	/** Negotiate MRU */
	u16_t negotiate_mru : 1;

	/** Negotiate */
	u16_t negotiate_async_map :1;

	/** Negotiate HDLC protocol field compression*/
	u16_t negotiate_proto_compression :1;

	/** Negotiate HDLC address/control field compression */
	u16_t negotiate_addr_compression :1;

	/** Negotiate magic number */
	u16_t negotiate_magic :1;
};

struct ipcp_options {
	/** IPv4 address */
	struct in_addr address;
};

struct ipv6cp_options {
	/** Interface identifier */
	u8_t iid[PPP_INTERFACE_IDENTIFIER_LEN];
};

/** PPP L2 context specific to certain network interface */
struct ppp_context {
	/** PPP startup worker. */
	struct k_delayed_work startup;

	struct {
		/** Carrier ON/OFF handler worker. This is used to create
		 * network interface UP/DOWN event when PPP L2 driver
		 * notices carrier ON/OFF situation. We must not create another
		 * network management event from inside management handler thus
		 * we use worker thread to trigger the UP/DOWN event.
		 */
		struct k_work work;

		/** Is the carrier enabled already */
		bool enabled;
	} carrier_mgmt;

	struct {
		/** Finite state machine for LCP */
		struct ppp_fsm fsm;

		/** Options that we want to request */
		struct lcp_options my_options;

		/** Options that peer want to request */
		struct lcp_options peer_options;

		/** Options that we accepted */
		struct lcp_options my_accepted;

		/** Options that peer accepted */
		struct lcp_options peer_accepted;

		/** Magic-Number value */
		u32_t magic;
	} lcp;

#if defined(CONFIG_NET_IPV4)
	struct {
		/** Finite state machine for IPCP */
		struct ppp_fsm fsm;

		/** Options that we want to request */
		struct ipcp_options my_options;

		/** Options that peer want to request */
		struct ipcp_options peer_options;

		/** Options that we accepted */
		struct ipcp_options my_accepted;

		/** Options that peer accepted */
		struct ipcp_options peer_accepted;
	} ipcp;
#endif

#if defined(CONFIG_NET_IPV6)
	struct {
		/** Finite state machine for IPV6CP */
		struct ppp_fsm fsm;

		/** Options that we want to request */
		struct ipv6cp_options my_options;

		/** Options that peer want to request */
		struct ipv6cp_options peer_options;

		/** Options that we accepted */
		struct ipv6cp_options my_accepted;

		/** Options that peer accepted */
		struct ipv6cp_options peer_accepted;
	} ipv6cp;
#endif

#if defined(CONFIG_NET_SHELL)
	struct {
		struct {
			/** Callback to be called when Echo-Reply is received.
			 */
			net_ppp_lcp_echo_reply_cb_t cb;

			/** User specific data for the callback */
			void *user_data;

			/** User data length */
			size_t user_data_len;
		} echo_reply;

		/** Used when waiting Echo-Reply */
		struct k_sem wait_echo_reply;

		/** Echo-Req data value */
		u32_t echo_req_data;

		/** Echo-Reply data value */
		u32_t echo_reply_data;
	} shell;
#endif

	/** Network interface related to this PPP connection */
	struct net_if *iface;

	/** Current phase of PPP link */
	enum ppp_phase phase;

	/** This tells what features the PPP supports. */
	enum net_l2_flags ppp_l2_flags;

	/** This tells how many network protocols are open */
	int network_protos_open;

	/** This tells how many network protocols are up */
	int network_protos_up;

	/** Is this context already initialized */
	u16_t is_init : 1;

	/** Is PPP ready to receive packets */
	u16_t is_ready_to_serve : 1;

	/** Is PPP L2 enabled or not */
	u16_t is_enabled : 1;

	/** PPP startup pending */
	u16_t is_startup_pending : 1;

	/** PPP enable pending */
	u16_t is_enable_done : 1;

	/** Network status (up / down) */
	u16_t is_network_up : 1;

	/** IPCP status (up / down) */
	u16_t is_ipcp_up : 1;

	/** IPCP open status (open / closed) */
	u16_t is_ipcp_open : 1;

	/** IPV6CP status (up / down) */
	u16_t is_ipv6cp_up : 1;

	/** IPV6CP open status (open / closed) */
	u16_t is_ipv6cp_open : 1;
};

/**
 * @brief Inform PPP L2 driver that carrier is detected.
 * This happens when cable is connected etc.
 *
 * @param iface Network interface
 */
void net_ppp_carrier_on(struct net_if *iface);

/**
 * @brief Inform PPP L2 driver that carrier was lost.
 * This happens when cable is disconnected etc.
 *
 * @param iface Network interface
 */
void net_ppp_carrier_off(struct net_if *iface);

/**
 * @brief Initialize PPP L2 stack for a given interface
 *
 * @param iface A valid pointer to a network interface
 */
void net_ppp_init(struct net_if *iface);

/* Management API for PPP */

/** @cond INTERNAL_HIDDEN */

#define PPP_L2_CTX_TYPE	struct ppp_context

#define _NET_PPP_LAYER	NET_MGMT_LAYER_L2
#define _NET_PPP_CODE	0x209
#define _NET_PPP_BASE	(NET_MGMT_IFACE_BIT |				\
			 NET_MGMT_LAYER(_NET_PPP_LAYER) |		\
			 NET_MGMT_LAYER_CODE(_NET_PPP_CODE))
#define _NET_PPP_EVENT	(_NET_PPP_BASE | NET_MGMT_EVENT_BIT)

enum net_event_ppp_cmd {
	NET_EVENT_PPP_CMD_CARRIER_ON = 1,
	NET_EVENT_PPP_CMD_CARRIER_OFF,
};

#define NET_EVENT_PPP_CARRIER_ON					\
	(_NET_PPP_EVENT | NET_EVENT_PPP_CMD_CARRIER_ON)

#define NET_EVENT_PPP_CARRIER_OFF					\
	(_NET_PPP_EVENT | NET_EVENT_PPP_CMD_CARRIER_OFF)

struct net_if;

/** @endcond */

/**
 * @brief Raise CARRIER_ON event when PPP is connected.
 *
 * @param iface PPP network interface.
 */
#if defined(CONFIG_NET_L2_PPP_MGMT)
void ppp_mgmt_raise_carrier_on_event(struct net_if *iface);
#else
static inline void ppp_mgmt_raise_carrier_on_event(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

/**
 * @brief Raise CARRIER_OFF event when PPP is disconnected.
 *
 * @param iface PPP network interface.
 */
#if defined(CONFIG_NET_L2_PPP_MGMT)
void ppp_mgmt_raise_carrier_off_event(struct net_if *iface);
#else
static inline void ppp_mgmt_raise_carrier_off_event(struct net_if *iface)
{
	ARG_UNUSED(iface);
}
#endif

/**
 * @brief Send PPP Echo-Request to peer. We expect to receive Echo-Reply back.
 *
 * @param idx PPP network interface index
 * @param timeout Amount of time to wait Echo-Reply.
 *
 * @return 0 if Echo-Reply was received, < 0 if there is a timeout or network
 * index is not a valid PPP network index.
 */
#if defined(CONFIG_NET_L2_PPP)
int net_ppp_ping(int idx, s32_t timeout);
#else
static inline int net_ppp_ping(int idx, s32_t timeout)
{
	ARG_UNUSED(idx);
	ARG_UNUSED(timeout);

	return -ENOTSUP;
}
#endif

/**
 * @brief Get PPP context information. This is only used by net-shell to
 * print information about PPP.
 *
 * @param idx PPP network interface index
 *
 * @return PPP context or NULL if idx is invalid.
 */
#if defined(CONFIG_NET_L2_PPP) && defined(CONFIG_NET_SHELL)
struct ppp_context *net_ppp_context_get(int idx);
#else
static inline struct ppp_context *net_ppp_context_get(int idx)
{
	ARG_UNUSED(idx);

	return NULL;
}
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_PPP_H_ */
