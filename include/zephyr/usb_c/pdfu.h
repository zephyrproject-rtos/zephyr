/*
 * Copyright (c) 2026 Meta Platforms, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * USB PD Firmware Update responder.
 *
 * Implements the PDFU Responder role from the USB Power Delivery Firmware
 * Update Specification, Revision 1.0 (15 September 2016). The protocol rides
 * on Firmware_Update_Request and Firmware_Update_Response Extended Messages;
 * this layer owns the phase state machine and message encoding, and calls out
 * to a backend for anything that touches storage.
 */

#ifndef ZEPHYR_INCLUDE_USB_C_PDFU_H_
#define ZEPHYR_INCLUDE_USB_C_PDFU_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>

/** Firmware update phases, specification section 2.3. */
enum pdfu_phase {
	PDFU_PHASE_ENUMERATION,
	PDFU_PHASE_RECONFIGURATION,
	PDFU_PHASE_TRANSFER,
	PDFU_PHASE_VALIDATION,
	PDFU_PHASE_MANIFESTATION,
};

/** Identity reported in a GET_FW_ID Response, specification Table 5-18. */
struct pdfu_fw_id {
	uint16_t vid;
	uint16_t pid;
	/** Bits 7:4 major, 3:0 minor */
	uint8_t hw_version;
	/** Bits 7:4 silicon base version */
	uint8_t si_version;
	/** Four version components, most significant first */
	uint16_t fw_version[4];
	/** Image bank the responder wants firmware for */
	uint8_t image_bank;
	/**
	 * Flags1 to Flags4, sent as given: what the update supports and what
	 * completing it takes, such as staying functional during the transfer
	 * or needing a power cycle to finish.
	 */
	uint8_t flags[4];
};

/**
 * Backend the responder drives. Everything that touches storage or identity
 * lives here so the protocol layer stays portable.
 *
 * All calls are made from the USB-C stack thread and must return promptly:
 * the specification allows 27 ms (tPDFUResponseSent) between the end of a
 * Request and the start of its Response.
 */
struct pdfu_ops {
	/** Fill in the identity reported by GET_FW_ID. */
	void (*get_fw_id)(struct pdfu_fw_id *id);

	/**
	 * Prepare to receive an image of unknown length.
	 *
	 * @param fw_version version the initiator is offering
	 * @param max_image_size set to the largest image that will fit
	 *
	 * @return 0 to accept, or a PDFU status code to refuse
	 */
	int (*begin)(const uint16_t fw_version[4], uint32_t *max_image_size);

	/**
	 * Append a data block.
	 *
	 * @return 0 on success, or a PDFU status code
	 */
	int (*write)(uint32_t offset, const uint8_t *data, uint32_t len);

	/**
	 * Check the image just received.
	 *
	 * @return 0 if it looks valid, or a PDFU status code
	 */
	int (*validate)(uint32_t total_len);

	/** Discard a partial transfer. */
	void (*abort)(void);

	/** Activate the new image. Does not return if it reboots. */
	void (*manifest)(void);
};

/**
 * Bind the responder to a USB-C port. There is one responder; binding it
 * again moves it.
 *
 * @return 0 on success, negative errno otherwise.
 */
int pdfu_responder_init(const struct device *port, const struct pdfu_ops *ops);

/**
 * Offer a received Extended Message to the responder.
 *
 * Intended to be called from a `policy_cb_ext_msg_received_t` callback.
 *
 * @return true if the responder consumed it.
 */
bool pdfu_responder_msg(const struct device *port, enum pd_ext_msg_type type, const uint8_t *data,
			uint32_t len);

/** Current phase, for diagnostics. */
enum pdfu_phase pdfu_responder_phase(void);

/** Human-readable phase name. */
const char *pdfu_phase_name(enum pdfu_phase phase);

/** Bytes received during the current transfer. */
uint32_t pdfu_responder_bytes(void);

#endif /* ZEPHYR_INCLUDE_USB_C_PDFU_H_ */
