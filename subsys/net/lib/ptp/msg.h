/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file msg.h
 * @brief PTP messages definition.
 *
 * References are to version 2019 of IEEE 1588, ("PTP")
 */

#ifndef ZEPHYR_INCLUDE_PTP_MSG_H_
#define ZEPHYR_INCLUDE_PTP_MSG_H_

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ptp_time.h>

#include "ddt.h"

#ifdef __cplusplus
extern "C" {
#endif

/* values of the bits of the flagField array for PTP message */
#define PTP_MSG_ALT_TIME_TRANSMITTER_FLAG BIT(0)
#define PTP_MSG_TWO_STEP_FLAG		  BIT(1)
#define PTP_MSG_UNICAST_FLAG		  BIT(2)

/**
 * @brief PTP message type.
 */
enum ptp_msg_type {
	/* PTP event message types */
	PTP_MSG_SYNC = 0,
	PTP_MSG_DELAY_REQ,
	PTP_MSG_PDELAY_REQ,
	PTP_MSG_PDELAY_RESP,
	/* General PTP message types */
	PTP_MSG_FOLLOW_UP = 8,
	PTP_MSG_DELAY_RESP,
	PTP_MSG_PDELAY_RESP_FOLLOW_UP,
	PTP_MSG_ANNOUNCE,
	PTP_MSG_SIGNALING,
	PTP_MSG_MANAGEMENT,
};

/**
 * @brief Common PTP message header.
 */
struct ptp_header {
	/** PTP message type and most significant 4 bytes of SdoId. */
	uint8_t		   type_major_sdo_id;
	/** PTP version. */
	uint8_t		   version;
	/** Number of bytes in the message. */
	uint16_t	   msg_length;
	/** ID number of an instance in a domain. */
	uint8_t		   domain_number;
	/** Minor SdoId. */
	uint8_t		   minor_sdo_id;
	/** Array of message flags. */
	uint8_t		   flags[2];
	/** Value of correction in nanoseconds multiplied by 2^16. */
	int64_t		   correction;
	/** @cond INTERNAL_HIDDEN */
	/** Padding. */
	uint32_t	   reserved;
	/** @endcond */
	/** PTP Port ID of the sender. */
	struct ptp_port_id src_port_id;
	/** unique ID number of the message. */
	uint16_t	   sequence_id;
	/** @cond INTERNAL_HIDDEN */
	/** Obsolete, used for padding. */
	uint8_t		   control;
	/** @endcond */
	/** Logarithm base of 2 of message interval. */
	int8_t		   log_msg_interval;
} __packed;

/**
 * @brief PTP Announce message header.
 */
struct ptp_announce_msg {
	/** PTP message header. */
	struct ptp_header      hdr;
	/** Estimate of the PTP Instance Time of an originating Instance
	 * when message was transmitted.
	 */
	struct ptp_timestamp   origin_timestamp;
	/** Value of @ref ptp_time_prop_ds.current_utc_offset . */
	uint16_t	       current_utc_offset;
	/** @cond INTERNAL_HIDDEN */
	/** Padding. */
	uint8_t		       reserved;
	/** @endcond */
	/** Value of @ref ptp_parent_ds.gm_priority1 of transmitter. */
	uint8_t		       gm_priority1;
	/** Value of @ref ptp_parent_ds.gm_clk_quality of transmitter. */
	struct ptp_clk_quality gm_clk_quality;
	/** Value of @ref ptp_parent_ds.gm_priority2 of transmitter. */
	uint8_t		       gm_priority2;
	/** Value of @ref ptp_parent_ds.gm_id of transmitter. */
	ptp_clk_id	       gm_id;
	/** Value of @ref ptp_current_ds.steps_rm of transmitter. */
	uint16_t	       steps_rm;
	/** Time source of a clock of the sender. */
	uint8_t		       time_src;
	/** Flexible array of zero or more TLV entities. */
	uint8_t		       suffix[];
} __packed;

/**
 * @brief PTP Sync message header.
 */
struct ptp_sync_msg {
	/** PTP message header. */
	struct ptp_header    hdr;
	/** PTP Instance Time of an originating PTP Instance when message was transmitted. */
	struct ptp_timestamp origin_timestamp;
	/** Flexible array of zero or more TLV entities. */
	uint8_t		     suffix[];
} __packed;

/**
 * @brief PTP Delay_Req message header.
 */
struct ptp_delay_req_msg {
	/** PTP message header. */
	struct ptp_header    hdr;
	/** PTP Instance Time of an originating PTP Instance when message was transmitted. */
	struct ptp_timestamp origin_timestamp;
	/** Flexible array of zero or more TLV entities. */
	uint8_t		     suffix[];
} __packed;

/**
 * @brief PTP Follow_Up message header.
 */
struct ptp_follow_up_msg {
	/** PTP message header. */
	struct ptp_header    hdr;
	/** Precise timestamp of Sync message corresponding to that message. */
	struct ptp_timestamp precise_origin_timestamp;
	/** Flexible array of zero or more TLV entities. */
	uint8_t		     suffix[];
} __packed;

/**
 * @brief PTP Delay_Resp message header.
 */
struct ptp_delay_resp_msg {
	/** PTP message header. */
	struct ptp_header    hdr;
	/** Ingress timestamp of Delay_Req message. */
	struct ptp_timestamp receive_timestamp;
	/** Port's ID of the sender of Delay_Req message. */
	struct ptp_port_id   req_port_id;
	/** Flexible array of zero or more TLV entities. */
	uint8_t		     suffix[];
} __packed;

/**
 * @brief PTP Pdelay_Req message header.
 */
struct ptp_pdelay_req_msg {
	/** PTP message header. */
	struct ptp_header    hdr;
	/** PTP Instance Time of an originating PTP Instance when message was transmitted. */
	struct ptp_timestamp origin_timestamp;
	/** @cond INTERNAL_HIDDEN */
	/** Padding. */
	struct ptp_port_id   reserved; /* make it the same length as ptp_pdelay_resp */
	/** @endcond */
	/** Flexible array of zero or more TLV entities. */
	uint8_t		     suffix[];
} __packed;

/**
 * @brief PTP Pdelay_Resp message header.
 */
struct ptp_pdelay_resp_msg {
	/** PTP message header. */
	struct ptp_header    hdr;
	/** Ingress timestamp of Pdelay_Req message. */
	struct ptp_timestamp req_receipt_timestamp;
	/** Port's ID of the sender of Pdelay_Req message. */
	struct ptp_port_id   req_port_id;
	/** Flexible array of zero or more TLV entities. */
	uint8_t		     suffix[];
} __packed;

/**
 * @brief PTP Pdelay_Resp_Follow_Up message header.
 */
struct ptp_pdelay_resp_follow_up_msg {
	/** PTP message header. */
	struct ptp_header    hdr;
	/** Precise timestamp of Pdelay_Resp message corresponding to that message. */
	struct ptp_timestamp resp_origin_timestamp;
	/** Port's ID of the sender of Pdelay_Req message. */
	struct ptp_port_id   req_port_id;
	/** Flexible array of zero or more TLV entities. */
	uint8_t		     suffix[];
} __packed;

/**
 * @brief PTP Signaling message header.
 */
struct ptp_signaling_msg {
	/** PTP message header. */
	struct ptp_header  hdr;
	/** Port's ID to which this message is addressed. */
	struct ptp_port_id target_port_id;
	/** Flexible array of zero or more TLV entities. */
	uint8_t		   suffix[];
} __packed;

/**
 * @brief PTP Management message header.
 */
struct ptp_management_msg {
	/** PTP message header. */
	struct ptp_header  hdr;
	/** Port's ID to which this message is addressed. */
	struct ptp_port_id target_port_id;
	/** For response it should be computed from starting_boundary_hops and boundary_hops
	 * of the requesting message.
	 */
	uint8_t		   starting_boundary_hops;
	/** Number of successive retransmissions of the message
	 * by Boundary Clocks receiving message.
	 */
	uint8_t		   boundary_hops;
	/** Action to be taken on receipt of the message */
	uint8_t		   action:5;
	/** @cond INTERNAL_HIDDEN */
	/** Padding. */
	uint8_t		   reserved;
	/** @endcond */
	/** Flexible array of zero or more TLV entities. */
	uint8_t		   suffix[];
} __packed;

/**
 * @brief Generic PTP message structure.
 */
struct ptp_msg {
	union {
		/** General PTP message header. */
		struct ptp_header		     header;
		/** Announce message. */
		struct ptp_announce_msg		     announce;
		/** Sync message. */
		struct ptp_sync_msg		     sync;
		/** Delay_Req message. */
		struct ptp_delay_req_msg	     delay_req;
		/** Follow_Up message. */
		struct ptp_follow_up_msg	     follow_up;
		/** Delay_Resp message. */
		struct ptp_delay_resp_msg	     delay_resp;
		/** Pdelay_Req message. */
		struct ptp_pdelay_req_msg	     pdelay_req;
		/** Pdelay_Resp message. */
		struct ptp_pdelay_resp_msg	     pdelay_resp;
		/** Pdelay_Resp_Follow_Up message. */
		struct ptp_pdelay_resp_follow_up_msg pdelay_resp_follow_up;
		/** Signaling message. */
		struct ptp_signaling_msg	     signaling;
		/** Management message. */
		struct ptp_management_msg	     management;
		/** @cond INTERNAL_HIDEN */
		/** MTU. */
		uint8_t				     mtu[NET_ETH_MTU];
		/** @endcond */
	} __packed;
	struct {
		/**
		 * Timestamp extracted from the message in a host binary format.
		 * Depending on the message type the value comes from different
		 * field of the message.
		 */
		struct net_ptp_time protocol;
		 /** Ingress timestamp on the host side. */
		struct net_ptp_time host;

	} timestamp;
	/** Reference counter. */
	atomic_t ref;
	/** List object. */
	sys_snode_t node;
	/** Single-linked list of TLVs attached to the message. */
	sys_slist_t tlvs;
	/** Protocol address of the sender/receiver of the message. */
	struct sockaddr addr;
};

/**
 * @brief Function allocating space for a new PTP message.
 *
 * @return Pointer to the new PTP Message.
 */
struct ptp_msg *ptp_msg_alloc(void);

/**
 * @brief Function removing reference to the PTP message.
 *
 * @note If the message is not referenced anywhere, the memory space is cleared.
 *
 * @param[in] msg Pointer to the PTP message.
 */
void ptp_msg_unref(struct ptp_msg *msg);

/**
 * @brief Function incrementing reference count for the PTP message.
 *
 * @param[in] msg Pointer to the PTP message.
 */
void ptp_msg_ref(struct ptp_msg *msg);

/**
 * @brief Function extracting message type from it.
 *
 * @param[in] msg Pointer to the message.
 *
 * @return Type of the message.
 */
enum ptp_msg_type ptp_msg_type(const struct ptp_msg *msg);

/**
 * @brief Function extracting PTP message from network packet.
 *
 * @param[in] pkt Pointer to the network packet.
 *
 * @note Returned message has all data in the network byte order.
 *
 * @return Pointer to a PTP message.
 */
struct ptp_msg *ptp_msg_from_pkt(struct net_pkt *pkt);

/**
 * @brief Function preparing message right before transmission.
 *
 * @param[in] msg Pointer to the prepared PTP message.
 */
void ptp_msg_pre_send(struct ptp_msg *msg);

/**
 * @brief Function preparing message for further processing after reception.
 *
 * @param[in] port Pointer to the PTP Port instance.
 * @param[in] msg  Pointer to the received PTP message.
 * @param[in] cnt  Length of the message in bytes.
 *
 * @return 0 on success, negative otherwise.
 */
int ptp_msg_post_recv(struct ptp_port *port, struct ptp_msg *msg, int cnt);

/**
 * @brief Function adding TLV of specified length to the message.
 *
 * @param[in] msg    Pointer to the message that will have TLV added.
 * @param[in] length Length of the TLV.
 *
 * @return Pointer to the TLV or NULL if there was no space to append TLV to the message.
 */
struct ptp_tlv *ptp_msg_add_tlv(struct ptp_msg *msg, int length);

/**
 * @brief Function compering content of two PTP Announce messages.
 *
 * @param[in] m1 Pointer to the Announce message to be compared.
 * @param[in] m2 Pointer to the Announce message to be compared.
 *
 * @return Negative if m1 < m2, 0 if equal, else positive
 */
int ptp_msg_announce_cmp(const struct ptp_announce_msg *m1, const struct ptp_announce_msg *m2);

/**
 * @brief Function checking if given message comes from current PTP Port's
 * TimeTransmitter PTP instance.
 *
 * @param[in] msg  Pointer to the message.
 *
 * @return True if the message is received from the current PTP Port's TimeTransmitter,
 * false otherwise.
 */
bool ptp_msg_current_parent(const struct ptp_msg *msg);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_MSG_H_ */
