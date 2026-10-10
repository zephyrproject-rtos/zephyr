/*
 * Copyright (c) 2026 Fabien Proriol
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief KNX packet buffer — the struct knx_pkt carried through L2..L7.
 */

#ifndef ZEPHYR_INCLUDE_KNX_PKT_H_
#define ZEPHYR_INCLUDE_KNX_PKT_H_

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/knx/knx_core.h>

#if CONFIG_LOG
#include <zephyr/logging/log.h>
#endif

/**
 * @defgroup knx_pkt KNX Packet Buffer
 * @ingroup knx_core
 * @{
 */
/** @brief Largest wire frame this buffer can hold (extended frame, LG = 254, plus FCS). */
#define MAX_KNX_TELEGRAM_SIZE 269

/**
 * @brief Maximum APDU length this device supports, in octets.
 *
 * This is the value published in PID_MAX_APDU_LENGTH (PID 56) and the ceiling
 * every layer must respect.  It is the APCI + data octet count, i.e. the frame's
 * LG field — not the LSDU (which additionally carries the TPCI octet).
 *
 * Why 55, derived rather than chosen:
 *
 *   extended wire frame = CTRL CTRLE SA(2) DA(2) LG TPCI APCI+data(LG) FCS
 *                       = 9 + LG octets
 *   knx_l1_send_frame() is handed  wire_len = 9 + LG
 *   and works on           raw_len = wire_len - 1 = 8 + LG
 *
 * The NCN5130 addresses frame octets with a 6-bit index, so octets beyond the
 * first 64 need U_L_DataOffset.req — a path this driver deliberately refuses
 * because the datasheet does not resolve how index 0 of block >= 1 is encoded
 * (see the long comment in knx_l1_send_frame()).  Keeping raw_len <= 63 keeps
 * every frame inside block 0:
 *
 *   raw_len <= 63  =>  LG <= 55  =>  wire frame <= 64 octets
 *
 * So 55 is exactly the largest APDU that never touches the untested offset
 * path.  Volume 6 §3.2 requires >= 15 for a TP1 profile and makes extended
 * frames optional, so 55 is comfortably conformant.
 *
 * Raising this means implementing U_L_DataOffset.req first — and the
 * BUILD_ASSERT in ncn5130-uart.c will refuse the change until then.
 *
 * @note The ETS product database declares its own MaxAPDULength. Keep the two
 *       in step, or a management client will fragment against the wrong bound.
 */
#define KNX_MAX_APDU_OCTETS 55

/** Largest LSDU (TPCI + APDU) that can be built from KNX_MAX_APDU_OCTETS. */
#define KNX_MAX_LSDU_OCTETS (KNX_MAX_APDU_OCTETS + 1)

/**
 * @brief Unified transmission status replacing L_Status, N_Status, T_Status, A_Status
 */
typedef enum knx_status {
	KNX_STATUS_OK = 0,     /**< Transmission succeeded */
	KNX_STATUS_NOT_OK = 1, /**< Transmission failed */
} knx_status_t;

/*
 * Ownership rule (Phase 1.5.4):
 *
 * Every public function that takes a struct knx_pkt * CONSUMES it — the
 * callee either frees it (knx_pkt_unref) or passes it to another consumer.
 * A caller that wants to keep the pkt after the call must knx_pkt_ref()
 * first; the callee's unref then just decrements the count.
 *
 * Functions declared as "borrows" in comments are an anti-pattern; use
 * knx_pkt_ref/unref pairs instead.
 */

/**
 * @brief Network service variant — set by L2, used by L3/L4 for .con dispatch
 */
typedef enum knx_pkt_type {
	KNX_PKT_DATA_INDIVIDUAL,       /**< Unicast to an individual address */
	KNX_PKT_DATA_GROUP,            /**< Multicast to a group address */
	KNX_PKT_DATA_BROADCAST,        /**< Domain broadcast */
	KNX_PKT_DATA_SYSTEM_BROADCAST, /**< System broadcast (management) */
} knx_pkt_type_t;

/**
 * @brief KNX packet — carries all layer metadata and payload through L2..L7
 *
 * Allocated from knx_pkt_slab. Ownership passes downward on .req and upward
 * on .ind. The holder is responsible for freeing via knx_pkt_free() when done.
 *
 * Fields set by layer:
 *  L2 (RX): src, dst, addr_type, priority, frame_format, hop_count, ack_request, buf, len
 *  L2 (TX): type (derived from addr_type/dst)
 *  L3:      tsap (group address → TSAP lookup)
 *  L4:      asap (TSAP → ASAP lookup)
 *  L2 con:  status
 */

struct knx_pkt {
	/** Embedded work item for dispatching RX frames to the app work-queue
	 *  (Phase 1.5.2).  Must be first so &pkt == CONTAINER_OF(work, ...).
	 */
	struct k_work work;

	/** Filled by Ph_Data__ind: the raw wire frame as received. */
	uint8_t buf[MAX_KNX_TELEGRAM_SIZE];
	/** Length of the valid prefix of buf; up to 263 for extended frames — see
	 *  KNX spec 3/2/2.
	 */
	uint16_t buf_len;

	/* Filled by l_process_rx_frame */
	knx_frame_format_t frame_format; /**< Standard or extended wire frame */
	knx_addr_type_t addr_type;       /**< Destination address type: individual or group */
	knx_addr_t src;                  /**< Source individual address */
	knx_addr_t dst;                  /**< Destination address (individual or group) */
	knx_priority_t priority;         /**< L2 priority (System/Urgent/Normal/Low) */
	uint8_t hop_count;               /**< Routing hop count */
	uint8_t lsdu_len;                /**< LSDU length in bytes (TPCI + APDU) */
	uint8_t ack_request;             /**< L2 ACK requested for this frame */
	/**
	 * Repetition service parameter (KNX spec 3/2/2 §2.2.2 / §2.4.2).
	 * 1 = this frame is a repetition of an earlier one.
	 *
	 * Note the wire polarity is INVERTED: CTRL bit 5 r = 0 means repeated.
	 * This field is already normalised, so it reads the obvious way.
	 */
	uint8_t repeated;
	knx_status_t status; /**< L2 .con outcome for a transmitted frame */
	uint8_t *lsdu;       /**< Always points at the TPCI octet, on TX and RX alike */

	knx_pkt_type_t type; /**< Network service variant, set by L2, used by L3/L4 for .con dispatch */
	uint16_t tsap;       /**< L3: group address → TSAP lookup */
	uint16_t asap;       /**< L4: TSAP → ASAP lookup */

	/**
	 * Set by L4's l7_dispatch_apdu() (layer4_transport.c) on the pkt it
	 * builds for an L7 __ind handler: true if the request arrived via
	 * T_Data_Individual (point-to-point connectionless — no open
	 * Transport Layer connection), false if via T_Data_Connected.
	 *
	 * L7's send_response() helper (layer7_application.c) reads this to
	 * pick T_Data_Individual__req (replying to pkt->src) vs
	 * T_Data_Connected__req (replying to the connected peer). Not set by
	 * L2/L3, so it must not be read outside that path; knx_pkt_alloc()
	 * does not zero the slab slot.
	 */
	bool connectionless;

	/**
	 * managed by knx_pkt_ref/unref.  atomic_t, not a plain integer:
	 * ref/unref is a read-modify-write touched from the L2 RX thread,
	 * knx_app_wq and the L2 TX thread — a plain "pkt->refcount++"/"--"
	 * is not safe across those contexts.
	 */
	atomic_t refcount;
};

/** @brief Backing pool for struct knx_pkt allocations (CONFIG_KNX_PKT_POOL_SIZE slots). */
extern struct k_mem_slab knx_pkt_slab;

/**
 * @brief Allocate a KNX packet from the slab pool (refcount = 1)
 */
static inline struct knx_pkt *knx_pkt_alloc(k_timeout_t timeout)
{
	struct knx_pkt *pkt = NULL;

	if (k_mem_slab_alloc(&knx_pkt_slab, (void **)&pkt, timeout) == 0) {
		atomic_set(&pkt->refcount, 1);
	}
	return pkt;
}

/**
 * @brief Increment reference count — caller keeps a copy of the pointer.
 */
static inline void knx_pkt_ref(struct knx_pkt *pkt)
{
	atomic_inc(&pkt->refcount);
}

/**
 * @brief Decrement reference count; return to pool when it reaches zero.
 *
 * Every consumer of a pkt pointer must call knx_pkt_unref() exactly once.
 */
static inline void knx_pkt_unref(struct knx_pkt *pkt)
{
	/* atomic_dec() returns the value BEFORE decrementing. */
	if (atomic_dec(&pkt->refcount) == 1) {
		k_mem_slab_free(&knx_pkt_slab, pkt);
	}
}

/**
 * @brief Compatibility alias — prefer knx_pkt_unref() in new code.
 */
static inline void knx_pkt_free(struct knx_pkt *pkt)
{
	knx_pkt_unref(pkt);
}

/** @} */

#endif /* ZEPHYR_INCLUDE_KNX_PKT_H_ */
