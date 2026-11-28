/*
 * Copyright (c) 2026 Meta Platforms, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * USB PD Firmware Update initiator.
 *
 * The other end of pdfu.h: pushes an image to a PDFU Responder over
 * Firmware_Update_Request / Firmware_Update_Response Extended Messages, USB PD
 * Firmware Update Specification, Revision 1.0. Useful to a device that updates
 * the PD accessories it connects to, and for testing a responder.
 */

#ifndef ZEPHYR_INCLUDE_USB_C_PDFU_INITIATOR_H_
#define ZEPHYR_INCLUDE_USB_C_PDFU_INITIATOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>

/** Where the image comes from. */
struct pdfu_image_src {
	/**
	 * Describe the image now. Called once per push, so the image may
	 * change between pushes.
	 *
	 * @param size    total bytes to send
	 * @param version four version components announced in PDFU_INITIATE
	 *
	 * @return 0 if there is an image, negative errno otherwise.
	 */
	int (*open)(uint32_t *size, uint16_t version[4]);

	/** Read @p len bytes at @p offset. @return 0 or negative errno. */
	int (*read)(uint32_t offset, uint8_t *buf, uint32_t len);
};

enum pdfu_initiator_state {
	PDFU_INIT_IDLE,
	PDFU_INIT_GET_FW_ID,
	PDFU_INIT_INITIATE,
	PDFU_INIT_DATA,
	PDFU_INIT_VALIDATE,
	/** Responder validated the image and is manifesting it */
	PDFU_INIT_DONE,
	PDFU_INIT_ABORTED,
	PDFU_INIT_FAILED,
};

struct pdfu_initiator_status {
	enum pdfu_initiator_state state;
	uint32_t size;
	uint32_t bytes_sent;
	uint16_t blocks_sent;
	/** Last PDFU status byte from the responder */
	uint8_t last_status;
	/** Responder's version from GET_FW_ID */
	uint16_t responder_version[4];
	/** Duration of the push so far, or of the finished push */
	uint32_t elapsed_ms;
	/** Why it failed, when it did */
	const char *error;
};

/** Faults to inject, for testing the responder's error paths. */
struct pdfu_initiator_faults {
	/** Send PDFU_ABORT once this many blocks are acknowledged; 0 = never */
	uint16_t abort_after;
	/** Flip a byte in this block, counted from 1; 0 = never */
	uint16_t corrupt_block;
};

/**
 * Bind the initiator to a USB-C port and an image source. There is one
 * initiator; binding it again moves it.
 *
 * @return 0 on success, negative errno otherwise.
 */
int pdfu_initiator_init(const struct device *port, const struct pdfu_image_src *src);

/**
 * Start pushing the image. Returns once the first request is queued; follow
 * progress with pdfu_initiator_status().
 *
 * @param faults NULL for a normal push
 *
 * @return 0 if started, -EBUSY if a push is running, -ENOENT if there is no
 *         image, negative errno otherwise.
 */
int pdfu_initiator_start(const struct pdfu_initiator_faults *faults);

/** Send PDFU_ABORT and stop. */
void pdfu_initiator_abort(void);

/**
 * Offer a received Extended Message to the initiator.
 *
 * Intended to be called from a `policy_cb_ext_msg_received_t` callback.
 *
 * @return true if the initiator consumed it.
 */
bool pdfu_initiator_msg(const struct device *port, enum pd_ext_msg_type type, const uint8_t *data,
			uint32_t len);

void pdfu_initiator_status(struct pdfu_initiator_status *out);

const char *pdfu_initiator_state_name(enum pdfu_initiator_state state);

#endif /* ZEPHYR_INCLUDE_USB_C_PDFU_INITIATOR_H_ */
