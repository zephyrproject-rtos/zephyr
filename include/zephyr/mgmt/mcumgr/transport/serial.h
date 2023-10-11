/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SERIAL_H_
#define ZEPHYR_INCLUDE_MGMT_SERIAL_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MCUMGR_SERIAL_HDR_PKT       0x0609
#define MCUMGR_SERIAL_HDR_FRAG      0x0414
#define MCUMGR_SERIAL_MAX_FRAME     127

#define MCUMGR_SERIAL_HDR_PKT_1     (MCUMGR_SERIAL_HDR_PKT >> 8)
#define MCUMGR_SERIAL_HDR_PKT_2     (MCUMGR_SERIAL_HDR_PKT & 0xff)
#define MCUMGR_SERIAL_HDR_FRAG_1    (MCUMGR_SERIAL_HDR_FRAG >> 8)
#define MCUMGR_SERIAL_HDR_FRAG_2    (MCUMGR_SERIAL_HDR_FRAG & 0xff)

/**
 * @brief Maintains state for an incoming mcumgr request packet.
 */
struct mcumgr_serial_rx_ctxt {
	/* Contains the partially- or fully-received mcumgr request.  Data
	 * stored in this buffer has already been base64-decoded.
	 */
	struct net_buf *nb;

	/* Length of full packet, as read from header. */
	uint16_t pkt_len;
};

/** @typedef mcumgr_serial_tx_cb
 * @brief Transmits a chunk of raw response data.
 *
 * @param data                  The data to transmit.
 * @param len                   The number of bytes to transmit.
 *
 * @return                      0 on success; negative error code on failure.
 */
typedef int (*mcumgr_serial_tx_cb)(const void *data, int len);

/**
 * @brief Processes an mcumgr request fragment received over a serial
 *        transport.
 *
 * Processes an mcumgr request fragment received over a serial transport.  If
 * the fragment is the end of a valid mcumgr request, this function returns a
 * net_buf containing the decoded request.  It is the caller's responsibility
 * to free the net_buf after it has been processed.
 *
 * @param rx_ctxt               The receive context associated with the serial
 *                                  transport being used.
 * @param frag                  The incoming fragment to process.
 * @param frag_len              The length of the fragment, in bytes.
 *
 * @return                      A net_buf containing the decoded request if a
 *                                  complete and valid request has been
 *                                  received.
 *                              NULL if the packet is incomplete or invalid.
 */
struct net_buf *mcumgr_serial_process_frag(
	struct mcumgr_serial_rx_ctxt *rx_ctxt,
	const uint8_t *frag, int frag_len);

/**
 * @brief Encodes and transmits an mcumgr packet over serial.
 *
 * @param data                  The mcumgr packet data to send.
 * @param len                   The length of the unencoded mcumgr packet.
 * @param cb                    A callback used to transmit raw bytes.
 *
 * @return                      0 on success; negative error code on failure.
 */
int mcumgr_serial_tx_pkt(const uint8_t *data, int len, mcumgr_serial_tx_cb cb);

#ifdef __cplusplus
}
#endif

#endif
