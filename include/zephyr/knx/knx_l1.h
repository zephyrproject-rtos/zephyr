/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief KNX Physical Layer (L1) driver API.
 *
 * This header is the contract between a KNX L1 driver (e.g. NCN5130, or the
 * software loopback) and the KNX L2 subsystem. subsys/knx never calls into a
 * driver except through this API, and a driver never calls into subsys/knx
 * except through the callbacks subsys/knx registers with
 * knx_phy_set_callbacks() — there is no other channel between the two, and in
 * particular no function here is resolved by name at link time: every call
 * goes through dev->api or the registered struct knx_phy_callbacks.
 *
 * Contents:
 *   - enum knx_ph_data_ind_class / enum knx_ph_status
 *   - struct knx_phy_callbacks — upcalls (driver -> subsys/knx)
 *   - struct knx_phy_driver_api — downcalls (subsys/knx -> driver)
 *   - knx_ph_reset_req / knx_u_set_address_req / knx_l1_send_frame / knx_phy_set_callbacks
 *     — the inline wrappers subsys/knx actually calls
 */

#ifndef ZEPHYR_INCLUDE_KNX_L1_H_
#define ZEPHYR_INCLUDE_KNX_L1_H_

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

/* ============================================================
 * KNX Physical Layer enums (KNX spec 3/3/1)
 * ============================================================
 */

/** @brief Byte class carried by a knx_ph_data_ind (L1 -> L2 RX). */
enum knx_ph_data_ind_class {
	KNX_PH_IND_START_OF_FRAME,   /**< First byte of a frame */
	KNX_PH_IND_INNER_FRAME_CHAR, /**< A non-first, non-last byte of a frame */
	KNX_PH_IND_END_OF_FRAME,     /**< Silence-based end-of-frame detection fired */
	KNX_PH_IND_ACK_CHAR,         /**< Bus ACK/NAK/BUSY octet received */
	KNX_PH_IND_POLL_DATA_CHAR,   /**< Polling-mode data octet (unused, no poll master) */
	KNX_PH_IND_PARITY_ERROR,     /**< 9th-bit even parity mismatch on this octet */
	KNX_PH_IND_FRAMING_ERROR,    /**< UART framing error on this octet */
	KNX_PH_IND_BIT_ERROR         /**< TX echo did not match what was sent (bus collision) */
};

/** @brief Physical-layer result code returned by knx_ph_reset_con and friends. */
enum knx_ph_status {
	KNX_PH_STATUS_OK = 0,                 /**< Success */
	KNX_PH_STATUS_TRANSCEIVER_FAULT = 1,  /**< NCN5130 reported a transceiver fault */
	KNX_PH_STATUS_COLLISION_DETECTED = 2, /**< Bus collision detected during TX */
	KNX_PH_STATUS_BUS_NOT_FREE = 3,       /**< Bus was not free when TX was attempted */
	KNX_PH_STATUS_ERROR = 4,              /**< Generic/other error */
};

/* ============================================================
 * Upcalls: driver -> subsys/knx
 * ============================================================
 *
 * subsys/knx registers exactly one set of these per device via
 * knx_phy_set_callbacks(), called once at stack init. A driver invokes them
 * from whatever context it owns (ISR, work queue, ...); subsys/knx's
 * implementations (layer1_physical.c) do not assume a particular context.
 */

struct knx_phy_callbacks {
	/** L1 -> L2 receive indication (from driver ISR / EOF timer). */
	void (*data_ind)(const struct device *dev, enum knx_ph_data_ind_class p_class,
			  uint8_t p_data);

	/** NCN5130 has entered Normal state; L2 may now send U_SetAddress and init. */
	void (*reset_con)(const struct device *dev, enum knx_ph_status p_status);
};

/* ============================================================
 * Downcalls: subsys/knx -> driver
 * ============================================================
 *
 * The only way subsys/knx talks to an L1 driver. No function in this file is
 * ever resolved by symbol name across the subsys/knx <-> driver boundary —
 * always through this vtable, via the inline wrappers below.
 */

__subsystem struct knx_phy_driver_api {
	/** Trigger NCN5130 reset (U_Reset.req). */
	void (*reset_req)(const struct device *dev);

	/** Program the NCN5130's auto-ACK address. */
	void (*set_address_req)(const struct device *dev, uint8_t addr_low, uint8_t addr_high);

	/**
	 * Send a complete KNX wire frame as a single DMA burst.
	 *
	 * @param frame Wire bytes: CTRL, SA, DA, AT|HC|LG, TPDU..., FCS
	 *              (caller has already appended the FCS byte at frame[len-1]).
	 * @param len   Total wire byte count (header + LSDU + FCS).
	 *              Range: 7..263 (standard 7..24, extended 25..263 — KNX
	 *              spec 3/2/2: LG field max = 254, 255 is a reserved escape
	 *              code, so 9 + 254 = 263 is the true extended-frame max).
	 * @return 0 on positive bus ACK, -EBUSY (bus not free), -EMSGSIZE (len out of range),
	 *         -ECONNREFUSED (negative L_Data.con — no device ACKed, not a collision),
	 *         -EIO (genuine NCN5130-reported collision/transceiver error),
	 *         -ETIMEDOUT (no L_Data.con at all).
	 */
	int (*send_frame)(const struct device *dev, const uint8_t *frame, uint16_t len);

	/** Register the struct knx_phy_callbacks subsys/knx wants upcalls on. */
	void (*set_callbacks)(const struct device *dev, const struct knx_phy_callbacks *callbacks);
};

static inline void knx_ph_reset_req(const struct device *dev)
{
	const struct knx_phy_driver_api *api = DEVICE_API_GET(knx_phy, dev);

	api->reset_req(dev);
}

static inline void knx_u_set_address_req(const struct device *dev, uint8_t addr_low,
					  uint8_t addr_high)
{
	const struct knx_phy_driver_api *api = DEVICE_API_GET(knx_phy, dev);

	api->set_address_req(dev, addr_low, addr_high);
}

static inline int knx_l1_send_frame(const struct device *dev, const uint8_t *frame, uint16_t len)
{
	const struct knx_phy_driver_api *api = DEVICE_API_GET(knx_phy, dev);

	return api->send_frame(dev, frame, len);
}

static inline void knx_phy_set_callbacks(const struct device *dev,
					  const struct knx_phy_callbacks *callbacks)
{
	const struct knx_phy_driver_api *api = DEVICE_API_GET(knx_phy, dev);

	api->set_callbacks(dev, callbacks);
}

#endif /* ZEPHYR_INCLUDE_KNX_L1_H_ */
