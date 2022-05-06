/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief gPTP message helpers.
 *
 * This is not to be included by the application.
 */

#ifndef __GPTP_MESSAGES_H
#define __GPTP_MESSAGES_H

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/gptp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Helpers to access gPTP messages. */
#define GPTP_HDR(pkt) gptp_get_hdr(pkt)
#define GPTP_ANNOUNCE(pkt) ((struct gptp_announce *)gptp_data(pkt))
#define GPTP_SIGNALING(pkt) ((struct gptp_signaling *)gptp_data(pkt))
#define GPTP_SYNC(pkt) ((struct gptp_sync *)gptp_data(pkt))
#define GPTP_FOLLOW_UP(pkt) ((struct gptp_follow_up *)gptp_data(pkt))
#define GPTP_DELAY_REQ(pkt) \
	((struct gptp_delay_req *)gptp_data(pkt))
#define GPTP_PDELAY_REQ(pkt) \
	((struct gptp_pdelay_req *)gptp_data(pkt))
#define GPTP_PDELAY_RESP(pkt) \
	((struct gptp_pdelay_resp *)gptp_data(pkt))
#define GPTP_PDELAY_RESP_FOLLOWUP(pkt) \
	((struct gptp_pdelay_resp_follow_up *)gptp_data(pkt))

/* Field values. */
#define GPTP_TRANSPORT_802_1_AS 0x1
#define GPTP_VERSION 0x2

/* Message Lengths. */
#define GPTP_PACKET_LEN(pkt) net_pkt_get_len(pkt)
#define GPTP_VALID_LEN(pkt, len) \
	(len > (NET_ETH_MINIMAL_FRAME_SIZE - GPTP_L2_HDR_LEN(pkt)))
#define GPTP_L2_HDR_LEN(pkt) \
	((long)GPTP_HDR(pkt) - (long)NET_ETH_HDR(pkt))

#define GPTP_SYNC_LEN \
	(sizeof(struct gptp_hdr) + sizeof(struct gptp_sync))
#define GPTP_FOLLOW_UP_LEN \
	(sizeof(struct gptp_hdr) + sizeof(struct gptp_follow_up))
#define GPTP_PDELAY_REQ_LEN \
	(sizeof(struct gptp_hdr) + sizeof(struct gptp_pdelay_req))
#define GPTP_PDELAY_RESP_LEN \
	(sizeof(struct gptp_hdr) + sizeof(struct gptp_pdelay_resp))
#define GPTP_PDELAY_RESP_FUP_LEN \
	(sizeof(struct gptp_hdr) + sizeof(struct gptp_pdelay_resp_follow_up))
#define GPTP_SIGNALING_LEN \
	(sizeof(struct gptp_hdr) + sizeof(struct gptp_signaling))

/* For the Announce message, the TLV is variable length. The len field
 * indicates the length of the TLV not accounting for tlvType and lengthField
 * which are 4 bytes.
 */
#define GPTP_ANNOUNCE_LEN(pkt) \
	(sizeof(struct gptp_hdr) + sizeof(struct gptp_announce) \
	 + ntohs(GPTP_ANNOUNCE(pkt)->tlv.len) \
	 - sizeof(struct gptp_path_trace_tlv) + 4)

#define GPTP_CHECK_LEN(pkt, len) \
	((GPTP_PACKET_LEN(pkt) != len) && (GPTP_VALID_LEN(pkt, len)))
#define GPTP_ANNOUNCE_CHECK_LEN(pkt) \
	((GPTP_PACKET_LEN(pkt) != GPTP_ANNOUNCE_LEN(pkt)) && \
	 (GPTP_VALID_LEN(pkt, GPTP_ANNOUNCE_LEN(pkt))))

/* Header Flags. Byte 0. */
#define GPTP_FLAG_ALT_MASTER        BIT(0)
#define GPTP_FLAG_TWO_STEP          BIT(1)
#define GPTP_FLAG_UNICAST           BIT(2)
#define GPTP_FLAG_PROFILE_SPECIFIC1 BIT(5)
#define GPTP_FLAG_PROFILE_SPECIFIC2 BIT(6)

/* Header Flags. Byte 1. */
#define GPTP_FLAG_LEAP61            BIT(0)
#define GPTP_FLAG_LEAP59            BIT(1)
#define GPTP_FLAG_CUR_UTC_OFF_VALID BIT(2)
#define GPTP_FLAG_PTP_TIMESCALE     BIT(3)
#define GPTP_FLAG_TIME_TRACEABLE    BIT(4)
#define GPTP_FLAG_FREQ_TRACEABLE    BIT(5)

/* Signaling Interval Flags. */
#define GPTP_FLAG_COMPUTE_NEIGHBOR_RATE_RATIO 0x1
#define GPTP_FLAG_COMPUTE_NEIGHBOR_PROP_DELAY 0x2

/* Signaling Interval Values. */
#define GPTP_ITV_KEEP               -128
#define GPTP_ITV_SET_TO_INIT        126
#define GPTP_ITV_STOP               127

/* Control. Only set for header compatibility with v1. */
#define GPTP_SYNC_CONTROL_VALUE     0x0
#define GPTP_FUP_CONTROL_VALUE      0x2
#define GPTP_OTHER_CONTROL_VALUE    0x5

/* Other default values. */
#define GPTP_RESP_LOG_MSG_ITV           0x7F
#define GPTP_ANNOUNCE_MSG_PATH_SEQ_TYPE htons(0x8)

/* Organization Id used for TLV. */
#define GPTP_FUP_TLV_ORG_ID_BYTE_0  0x00
#define GPTP_FUP_TLV_ORG_ID_BYTE_1  0x80
#define GPTP_FUP_TLV_ORG_ID_BYTE_2  0xC2
#define GPTP_FUP_TLV_ORG_SUB_TYPE   0x01

/**
 * @brief gPTP Clock Quality
 *
 * Defines the quality of a clock.
 * This is used by the Best Master Clock Algorithm.
 */
struct gptp_clock_quality {
	uint8_t clock_class;
	uint8_t clock_accuracy;
	uint16_t offset_scaled_log_var;
} __packed;

/**
 * @brief gPTP Root System Identity
 *
 * Defines the Grand Master of a clock.
 * This is used by the Best Master Clock Algorithm.
 */
struct gptp_root_system_identity {
	/** Grand Master priority1 component. */
	uint8_t grand_master_prio1;

	/** Grand Master clock quality. */
	struct gptp_clock_quality clk_quality;

	/** Grand Master priority2 component. */
	uint8_t grand_master_prio2;

	/** Grand Master clock identity. */
	uint8_t grand_master_id[GPTP_CLOCK_ID_LEN];
} __packed;

/* Definition of all message types as defined by IEEE802.1AS. */

struct gptp_path_trace_tlv {
	/** TLV type: 0x8. */
	uint16_t type;

	/** Length. Number of TLVs * 8 bytes. */
	uint16_t len;

	/** ClockIdentity array of the successive time-aware systems. */
	uint8_t  path_sequence[1][8];
} __packed;

struct gptp_announce {
	/** Reserved fields. */
	uint8_t reserved1[10];

	/** Current UTC offset. */
	int16_t cur_utc_offset;

	/** Reserved field. */
	uint8_t reserved2;

	/* gmPriorityVector priority 1 of the peer sending the message. */
	struct gptp_root_system_identity root_system_id;

	/** masterStepsRemoved of the peer sending the message. */
	uint16_t steps_removed;

	/** timeSource of the peer sending the message. */
	uint8_t time_source;

	/* Path Trace TLV. This field has a variable length. */
	struct gptp_path_trace_tlv tlv;
} __packed;

struct gptp_sync {
	/** Reserved field. This field is used for PTPv2, unused in gPTP. */
	uint8_t reserved[10];
} __packed;

struct gptp_follow_up_tlv_hdr {
	/** TLV type: 0x3. */
	uint16_t type;

	/** Length: 28. */
	uint16_t len;
} __packed;

struct gptp_follow_up_tlv {
	/** Organization Id: 00-80-C2. */
	uint8_t org_id[3];

	/** Organization Sub Type: 1. */
	uint8_t org_sub_type[3];

	/** Rate ratio relative to the grand master of the peer. */
	int32_t cumulative_scaled_rate_offset;

	/** Time Base Indicator of the current Grand Master. */
	uint16_t gm_time_base_indicator;

	/** Difference of the time between the current GM and the previous. */
	struct gptp_scaled_ns last_gm_phase_change;

	/** Diff of the frequency between the current GM and the previous. */
	int32_t scaled_last_gm_freq_change;
} __packed;

struct gptp_follow_up {
	/** Higher 16 bits of the seconds at which the sync was sent. */
	uint16_t prec_orig_ts_secs_high;

	/** Lower 32 bits of the seconds at which the sync was sent. */
	uint32_t prec_orig_ts_secs_low;

	/** Nanoseconds at which the sync was sent. */
	uint32_t prec_orig_ts_nsecs;

	/** Follow up TLV. */
	struct gptp_follow_up_tlv_hdr tlv_hdr;
	struct gptp_follow_up_tlv tlv;
} __packed;

struct gptp_pdelay_req {
	/** Reserved fields. */
	uint8_t reserved1[10];

	/** Reserved fields. */
	uint8_t reserved2[10];
} __packed;

struct gptp_pdelay_resp {
	/** Higher 16 bits of the seconds at which the request was received. */
	uint16_t req_receipt_ts_secs_high;

	/** Lower 32 bits of the seconds at which the request was received. */
	uint32_t req_receipt_ts_secs_low;

	/** Nanoseconds at which the pdelay request was received. */
	uint32_t req_receipt_ts_nsecs;

	/** Source Port Id of the Path Delay Request. */
	struct gptp_port_identity requesting_port_id;
} __packed;

struct gptp_pdelay_resp_follow_up {
	/** Higher 16 bits of the seconds at which the response was sent. */
	uint16_t resp_orig_ts_secs_high;

	/** Lower 32 bits of the seconds at which the response was sent. */
	uint32_t resp_orig_ts_secs_low;

	/** Nanoseconds at which the response was received. */
	uint32_t resp_orig_ts_nsecs;

	/** Source Port Id of the Path Delay Request. */
	struct gptp_port_identity requesting_port_id;
} __packed;

struct gptp_message_itv_req_tlv {
	/** TLV type: 0x3. */
	uint16_t type;

	/** Length field: 12. */
	uint16_t len;

	/** Organization Id: 00-80-C2. */
	uint8_t org_id[3];

	/** Organization sub type: 0x2. */
	uint8_t org_sub_type[3];

	/** Log to base 2 of the mean time interval between pdelay requests. */
	int8_t link_delay_itv;

	/** Log to base 2 of the mean time interval between syncs. */
	int8_t time_sync_itv;

	/** Log to base 2 of the mean time interval between announces. */
	int8_t announce_itv;

	/** Flags (computeNeighborRateRatio and computeNeighborPropDelay). */
	union {
		struct {
		    uint8_t compute_neighbor_rate_ratio : 1;
		    uint8_t compute_neighbor_prop_delay : 1;
		};
		uint8_t flags;
	};
	/** Reserved fields. */
	uint8_t reserved[2];
} __packed;

struct gptp_signaling {
	/** Target Port Identity , always 0xFF. */
	struct gptp_port_identity target_port_id;

	/** Message Interval TLV. */
	struct gptp_message_itv_req_tlv tlv;
} __packed;

/**
 * @brief Compute gPTP message location.
 *
 * @param pkt Network Buffer containing a gPTP message.
 *
 * @return Pointer to the start of the gPTP message inside the packet.
 */
static inline uint8_t *gptp_data(struct net_pkt *pkt)
{
	return (uint8_t *)GPTP_HDR(pkt) + sizeof(struct gptp_hdr);
}

/* Functions to prepare messages. */

/**
 * @brief Prepare Sync message.
 *
 * @param port gPTP port number.
 *
 * @return Pointer to the prepared Network Buffer.
 */
struct net_pkt *gptp_prepare_sync(int port);

/**
 * @brief Prepare Follow Up message.
 *
 * @param port gPTP port number.
 *
 * @return Pointer to the prepared Network Buffer.
 */
struct net_pkt *gptp_prepare_follow_up(int port, struct net_pkt *sync);

/**
 * @brief Prepare Path Delay Request message.
 *
 * @param port gPTP port number.
 *
 * @return Pointer to the prepared Network Buffer.
 */
struct net_pkt *gptp_prepare_pdelay_req(int port);

/**
 * @brief Prepare Path Delay Response message.
 *
 * @param port gPTP port number.
 * @param req Path Delay Request to reply to.
 *
 * @return Pointer to the prepared Network Buffer.
 */
struct net_pkt *gptp_prepare_pdelay_resp(int port,
		struct net_pkt *req);

/**
 * @brief Prepare Announce message.
 *
 * @param port gPTP port number.
 *
 * @return Pointer to the prepared Network Buffer.
 */
struct net_pkt *gptp_prepare_announce(int port);

/**
 * @brief Prepare Path Delay Response message.
 *
 * @param port gPTP port number.
 * @param resp Related Path Delay Follow Up.
 *
 * @return Pointer to the prepared Network Buffer.
 */
struct net_pkt *gptp_prepare_pdelay_follow_up(int port,
		struct net_pkt *resp);

/* Functions to handle received messages. */

/**
 * @brief Handle Sync message.
 *
 * @param port gPTP port number.
 * @param pkt Network Buffer.
 */
void gptp_handle_sync(int port, struct net_pkt *pkt);

/**
 * @brief Handle Follow Up message.
 *
 * @param port gPTP port number.
 * @param pkt Network Buffer to parse.
 *
 * @return 0 if success, Error Code otherwise.
 */
int gptp_handle_follow_up(int port, struct net_pkt *pkt);

/**
 * @brief Handle Path Delay Request message.
 *
 * @param port gPTP port number.
 * @param pkt Network Buffer.
 */
void gptp_handle_pdelay_req(int port, struct net_pkt *pkt);

/**
 * @brief Handle Path Delay Response message.
 *
 * @param port gPTP port number.
 * @param pkt Network Buffer to parse.
 *
 * @return 0 if success, Error Code otherwise.
 */
int gptp_handle_pdelay_resp(int port, struct net_pkt *pkt);

/**
 * @brief Handle Path Delay Follow Up message.
 *
 * @param port gPTP port number.
 * @param pkt Network Buffer to parse.
 *
 * @return 0 if success, Error Code otherwise.
 */
int gptp_handle_pdelay_follow_up(int port, struct net_pkt *pkt);

/**
 * @brief Handle Signaling message.
 *
 * @param port gPTP port number.
 * @param pkt Network Buffer
 */
void gptp_handle_signaling(int port, struct net_pkt *pkt);

/* Functions to send messages. */

/**
 * @brief Send a Sync message.
 *
 * @param port gPTP port number.
 * @param pkt Sync message.
 */
void gptp_send_sync(int port, struct net_pkt *pkt);

/**
 * @brief Send a Follow Up message.
 *
 * @param port gPTP port number.
 * @param pkt Follow Up message.
 */
void gptp_send_follow_up(int port, struct net_pkt *pkt);

/**
 * @brief Send an Announce message.
 *
 * @param port gPTP port number.
 * @param pkt Announce message.
 */
void gptp_send_announce(int port, struct net_pkt *pkt);

/**
 * @brief Send a Path Delay Request on the given port.
 *
 * @param port gPTP port number.
 */
void gptp_send_pdelay_req(int port);

/**
 * @brief Send a Path Delay Response for the given Path Delay Request.
 *
 * @param port gPTP port number.
 * @param pkt Network Buffer containing the prepared Path Delay Response.
 * @param treq Time at which the Path Delay Request was received.
 */
void gptp_send_pdelay_resp(int port, struct net_pkt *pkt,
			   struct net_ptp_time *treq);

/**
 * @brief Send a Path Delay Response for the given Path Delay Request.
 *
 * @param port gPTP port number.
 * @param pkt Network Buffer containing the prepared Path Delay Follow Up.
 * @param tresp Time at which the Path Delay Response was sent.
 */
void gptp_send_pdelay_follow_up(int port, struct net_pkt *pkt,
				struct net_ptp_time *tresp);

#ifdef __cplusplus
}
#endif

#endif /* __GPTP_MESSAGES_H */
