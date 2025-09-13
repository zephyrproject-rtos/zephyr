/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/__assert.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/base64.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>

static void mcumgr_serial_free_rx_ctxt(struct mcumgr_serial_rx_ctxt *rx_ctxt)
{
	if (rx_ctxt->nb != NULL) {
		smp_packet_free(rx_ctxt->nb);
		rx_ctxt->nb = NULL;
	}
}

static uint16_t mcumgr_serial_calc_crc(const uint8_t *data, int len)
{
	return crc16_itu_t(0x0000, data, len);
}

static int mcumgr_serial_extract_len(struct mcumgr_serial_rx_ctxt *rx_ctxt)
{
	if (rx_ctxt->nb->len < 2) {
		return -EINVAL;
	}

	rx_ctxt->pkt_len = net_buf_pull_be16(rx_ctxt->nb);
	return 0;
}

static int mcumgr_serial_decode_frag(struct mcumgr_serial_rx_ctxt *rx_ctxt,
				     const uint8_t *frag, int frag_len)
{
	size_t dec_len;
	int rc;

	rc = base64_decode(rx_ctxt->nb->data + rx_ctxt->nb->len,
				   net_buf_tailroom(rx_ctxt->nb), &dec_len,
				   frag, frag_len);
	if (rc != 0) {
		return -EINVAL;
	}

	rx_ctxt->nb->len += dec_len;

	return 0;
}

/**
 * Processes a received mcumgr frame.
 *
 * @return                      true if a complete packet was received;
 *                              false if the frame is invalid or if additional
 *                                  fragments are expected.
 */
struct net_buf *mcumgr_serial_process_frag(
	struct mcumgr_serial_rx_ctxt *rx_ctxt,
	const uint8_t *frag, int frag_len)
{
	struct net_buf *nb;
	uint16_t crc;
	uint16_t op;
	int rc;

	if (rx_ctxt->nb == NULL) {
		rx_ctxt->nb = smp_packet_alloc();
		if (rx_ctxt->nb == NULL) {
			return NULL;
		}
	}

	if (frag_len < sizeof(op)) {
		return NULL;
	}

	op = sys_be16_to_cpu(*(uint16_t *)frag);
	switch (op) {
	case MCUMGR_SERIAL_HDR_PKT:
		net_buf_reset(rx_ctxt->nb);
		break;

	case MCUMGR_SERIAL_HDR_FRAG:
		if (rx_ctxt->nb->len == 0U) {
			mcumgr_serial_free_rx_ctxt(rx_ctxt);
			return NULL;
		}
		break;

	default:
		return NULL;
	}

	rc = mcumgr_serial_decode_frag(rx_ctxt,
				       frag + sizeof(op),
				       frag_len - sizeof(op));
	if (rc != 0) {
		mcumgr_serial_free_rx_ctxt(rx_ctxt);
		return NULL;
	}

	if (op == MCUMGR_SERIAL_HDR_PKT) {
		rc = mcumgr_serial_extract_len(rx_ctxt);
		if (rc < 0) {
			mcumgr_serial_free_rx_ctxt(rx_ctxt);
			return NULL;
		}
	}

	if (rx_ctxt->nb->len < rx_ctxt->pkt_len) {
		/* More fragments expected. */
		return NULL;
	}

	if (rx_ctxt->nb->len > rx_ctxt->pkt_len) {
		/* Payload longer than indicated in header. */
		mcumgr_serial_free_rx_ctxt(rx_ctxt);
		return NULL;
	}

	crc = mcumgr_serial_calc_crc(rx_ctxt->nb->data, rx_ctxt->nb->len);
	if (crc != 0U) {
		mcumgr_serial_free_rx_ctxt(rx_ctxt);
		return NULL;
	}

	/* Packet is complete; strip the CRC. */
	rx_ctxt->nb->len -= 2U;

	nb = rx_ctxt->nb;
	rx_ctxt->nb = NULL;
	return nb;
}

/**
 * Base64-encodes a small chunk of data and transmits it. The data must be no larger than three
 * bytes.
 */
static int mcumgr_serial_tx_small(const struct device *const dev, const void *data,
				  int len, mcumgr_serial_tx_cb cb)
{
	uint8_t b64[4 + 1]; /* +1 required for null terminator. */
	size_t dst_len;
	int rc;

	rc = base64_encode(b64, sizeof(b64), &dst_len, data, len);
	__ASSERT_NO_MSG(rc == 0);
	__ASSERT_NO_MSG(dst_len == 4);

	return cb(dev, b64, 4);
}

/**
 * @brief Transmits a single mcumgr packet over serial, splits into multiple frames as needed.
 *
 * @param data                  The packet payload to transmit. This does not include a header or
 *                                  CRC.
 * @param len                   The size of the packet payload.
 * @param cb                    A callback used for transmitting raw data.
 *
 * @return                      0 on success; negative error code on failure.
 */
int mcumgr_serial_tx_pkt(const struct device *const dev, const uint8_t *data,
			 int len, mcumgr_serial_tx_cb cb)
{
	bool first = true;
	bool last = false;
	uint8_t raw[3];
	uint16_t u16;
	uint16_t crc;
	int src_off = 0;
	int rc = 0;
	int to_process;
	int reminder;

	/*
	 * This is max input bytes that can be taken to the frame before encoding with Base64;
	 * Base64 has three-to-four ratio, which means that for each three input bytes there are
	 * going to be four bytes of output used. The frame starts with two byte marker and ends
	 * with newline character, which are not encoded (this is the "-3" in calculation).
	 */
	int max_input = (((MCUMGR_SERIAL_MAX_FRAME - 3) >> 2) * 3);

	/* Calculate the CRC16 checksum of the whole packet, prior to splitting it into frames */
	crc = mcumgr_serial_calc_crc(data, len);

	/* First frame marker */
	u16 = sys_cpu_to_be16(MCUMGR_SERIAL_HDR_PKT);

	while (src_off < len) {
		/* Send first frame or continuation frame marker */
		rc = cb(dev, &u16, sizeof(u16));
		if (rc != 0) {
			return rc;
		}

		/*
		 * Only the first fragment contains the packet length; the packet length, which is
		 * two byte long is paired with first byte of input buffer to form triplet for
		 * Base64 encoding.
		 */
		if (first) {
			/* The size of the CRC16 should be added to packet length */
			u16 = sys_cpu_to_be16(len + 2);
			memcpy(raw, &u16, sizeof(u16));
			raw[2] = data[0];

			rc = mcumgr_serial_tx_small(dev, raw, 3, cb);
			if (rc != 0) {
				return rc;
			}

			++src_off;
			/* One triple of allowed input already used */
			max_input -= 3;
		}

		/*
		 * Only as much as fits into the frame can be processed, but we also need to fit
		 * in the two byte CRC. The frame can not be stretched and current logic does not
		 * allow to send CRC separately so if CRC would not fit as a whole, shrink
		 * to_process by one byte forcing one byte to the next frame, with the CRC.
		 */
		to_process = MIN(max_input, len - src_off);
		reminder = max_input - (len - src_off);

		if (reminder == 0 || reminder == 1) {
			to_process -= 1;

			/* This is the last frame but the checksum cannot be added, remove the
			 * last packet flag until the function runs again with the final piece of
			 * data and place the checksum there
			 */
			last = false;
		} else if (reminder >= 2) {
			last = true;
		}

		/*
		 * Try to process input buffer by chunks of three bytes that will be output as
		 * four byte chunks, due to Base64 encoding; the number of chunks that can be
		 * processed is calculated from number of three byte, complete, chunks in input
		 * buffer, but can not be greater than the number of four byte, complete, chunks
		 * that the frame can accept.
		 */
		while (to_process >= 3) {
			memcpy(raw, data + src_off, 3);
			rc = mcumgr_serial_tx_small(dev, raw, 3, cb);
			if (rc != 0) {
				return rc;
			}
			src_off += 3;
			to_process -= 3;
		}

		if (last) {
			/*
			 * Process the reminder bytes of the input buffer, after sending it in
			 * three byte chunks, and CRC.
			 */
			switch (len - src_off) {
			case 0:
				raw[0] = (crc & 0xff00) >> 8;
				raw[1] = crc & 0x00ff;
				rc = mcumgr_serial_tx_small(dev, raw, 2, cb);
				break;

			case 1:
				raw[0] = data[src_off++];

				raw[1] = (crc & 0xff00) >> 8;
				raw[2] = crc & 0x00ff;
				rc = mcumgr_serial_tx_small(dev, raw, 3, cb);
				break;

			case 2:
				raw[0] = data[src_off++];
				raw[1] = data[src_off++];

				raw[2] = (crc & 0xff00) >> 8;
				rc = mcumgr_serial_tx_small(dev, raw, 3, cb);
				if (rc != 0) {
					return rc;
				}

				raw[0] = crc & 0x00ff;
				rc = mcumgr_serial_tx_small(dev, raw, 1, cb);
				break;
			}

			if (rc != 0) {
				return rc;
			}
		}

		rc = cb(dev, "\n", 1);
		if (rc != 0) {
			return rc;
		}

		/* Use a continuation frame marker for the next packet */
		u16 = sys_cpu_to_be16(MCUMGR_SERIAL_HDR_FRAG);
		first = false;
	}

	return 0;
}
