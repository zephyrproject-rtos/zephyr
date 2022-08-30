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
#include <zephyr/net/buf.h>
#include <zephyr/sys/base64.h>
#include <zephyr/mgmt/mcumgr/buf.h>
#include <zephyr/mgmt/mcumgr/serial.h>

static void mcumgr_serial_free_rx_ctxt(struct mcumgr_serial_rx_ctxt *rx_ctxt)
{
	if (rx_ctxt->nb != NULL) {
		mcumgr_buf_free(rx_ctxt->nb);
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
		rx_ctxt->nb = mcumgr_buf_alloc();
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
 * Base64-encodes a small chunk of data and transmits it.  The data must be no
 * larger than three bytes.
 */
static int mcumgr_serial_tx_small(const void *data, int len,
				  mcumgr_serial_tx_cb cb, void *arg)
{
	uint8_t b64[4 + 1]; /* +1 required for null terminator. */
	size_t dst_len;
	int rc;

	rc = base64_encode(b64, sizeof(b64), &dst_len, data, len);
	__ASSERT_NO_MSG(rc == 0);
	__ASSERT_NO_MSG(dst_len == 4);

	return cb(b64, 4, arg);
}

/**
 * @brief Transmits a single mcumgr frame over serial.
 *
 * @param data                  The frame payload to transmit.  This does not
 *                                  include a header or CRC.
 * @param first                 Whether this is the first frame in the packet.
 * @param len                   The number of untransmitted data bytes in the
 *                                  packet.
 * @param crc                   The 16-bit CRC of the entire packet.
 * @param cb                    A callback used for transmitting raw data.
 * @param arg                   An optional argument that gets passed to the
 *                                  callback.
 * @param out_data_bytes_txed   On success, the number of data bytes
 *                                  transmitted gets written here.
 *
 * @return                      0 on success; negative error code on failure.
 */
int mcumgr_serial_tx_frame(const uint8_t *data, bool first, int len,
			   uint16_t crc, mcumgr_serial_tx_cb cb, void *arg,
			   int *out_data_bytes_txed)
{
	uint8_t raw[3];
	uint16_t u16;
	int src_off = 0;
	int rc = 0;
	int to_process;
	bool last = false;
	int reminder;

	/*
	 * This is max input bytes that can be taken to the frame before encoding with Base64;
	 * Base64 has three-to-four ratio, which means that for each three input bytes there are
	 * going to be four bytes of output used.  The frame starts with two byte marker and ends
	 * with newline character, which are not encoded (this is the "-3" in calculation).
	 */
	int max_input = (((MCUMGR_SERIAL_MAX_FRAME - 3) >> 2) * 3);

	if (first) {
		/* First frame marker */
		u16 = sys_cpu_to_be16(MCUMGR_SERIAL_HDR_PKT);
	} else {
		/* Continuation frame marker */
		u16 = sys_cpu_to_be16(MCUMGR_SERIAL_HDR_FRAG);
	}

	rc = cb(&u16, sizeof(u16), arg);
	if (rc != 0) {
		return rc;
	}

	/*
	 * Only the first fragment contains the packet length; the packet length, which is two
	 * byte long is paired with first byte of input buffer to form triplet for Base64 encoding.
	 */
	if (first) {
		/* The size of the CRC16 should be added to packet length */
		u16 = sys_cpu_to_be16(len + 2);
		memcpy(raw, &u16, sizeof(u16));
		raw[2] = data[0];

		rc = mcumgr_serial_tx_small(raw, 3, cb, arg);
		if (rc != 0) {
			return rc;
		}

		++src_off;
		/* One triple of allowed input already used */
		max_input -= 3;
	}

	/*
	 * Only as much as fits into the frame can be processed, but we also need to fit in the
	 * two byte CRC.  The frame can not be stretched and current logic does not allow to
	 * send CRC separately so if CRC would not fit as a whole, shrink to_process by one byte
	 * forcing one byte to the next frame, with the CRC.
	 */
	to_process = MIN(max_input, len - src_off);
	reminder = max_input - (len - src_off);

	if (reminder == 0 || reminder == 1) {
		to_process -= 1;
	} else if (reminder >= 2) {
		last = true;
	}

	/*
	 * Try to process input buffer by chunks of three bytes that will be output as four byte
	 * chunks, due to Base64 encoding; the number of chunks that can be processed is calculated
	 * from number of three byte, complete, chunks in input buffer,  but can not be greater
	 * than the number of four byte, complete, chunks that the frame can accept.
	 */
	while (to_process >= 3) {
		memcpy(raw, data + src_off, 3);
		rc = mcumgr_serial_tx_small(raw, 3, cb, arg);
		if (rc != 0) {
			return rc;
		}
		src_off += 3;
		to_process -= 3;
	}

	if (last) {
		/*
		 * Process the reminder bytes of the input buffer, after sending it in three byte
		 * chunks, and CRC.
		 */
		switch (len - src_off) {
		case 0:
			raw[0] = (crc & 0xff00) >> 8;
			raw[1] = crc & 0x00ff;
			rc = mcumgr_serial_tx_small(raw, 2, cb, arg);
			break;

		case 1:
			raw[0] = data[src_off++];

			raw[1] = (crc & 0xff00) >> 8;
			raw[2] = crc & 0x00ff;
			rc = mcumgr_serial_tx_small(raw, 3, cb, arg);
			break;

		case 2:
			raw[0] = data[src_off++];
			raw[1] = data[src_off++];

			raw[2] = (crc & 0xff00) >> 8;
			rc = mcumgr_serial_tx_small(raw, 3, cb, arg);
			if (rc != 0) {
				return rc;
			}

			raw[0] = crc & 0x00ff;
			rc = mcumgr_serial_tx_small(raw, 1, cb, arg);
			break;
		}

		if (rc != 0) {
			return rc;
		}
	}

	rc = cb("\n", 1, arg);
	if (rc != 0) {
		return rc;
	}

	*out_data_bytes_txed = src_off;
	return 0;
}

int mcumgr_serial_tx_pkt(const uint8_t *data, int len, mcumgr_serial_tx_cb cb,
			 void *arg)
{
	uint16_t crc;
	int data_bytes_txed = 0;
	int rc;

	/* Calculate CRC of entire packet. */
	crc = mcumgr_serial_calc_crc(data, len);

	/* Transmit packet as a sequence of frames. */
	while (len) {
		rc = mcumgr_serial_tx_frame(data, data_bytes_txed == 0, len, crc, cb, arg,
					    &data_bytes_txed);
		if (rc != 0) {
			return rc;
		}

		data += data_bytes_txed;
		len -= data_bytes_txed;
	}

	return 0;
}
