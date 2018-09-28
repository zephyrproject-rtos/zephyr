/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Utility functions used by the UART and shell mcumgr transports.
 *
 * Mcumgr packets sent over serial are fragmented into frames of 128 bytes or
 * fewer.
 *
 * The initial frame in a packet has the following format:
 *     offset 0:    0x06 0x09
 *     === Begin base64 encoding ===
 *     offset 2:    {16-bit packet-length}
 *     offset ?:    {body}
 *     offset ?:    {crc16} (if final frame)
 *     === End base64 encoding ===
 *     offset ?:    0x0a (newline)
 *
 * All subsequent frames have the following format:
 *     offset 0:    0x04 0x14
 *     === Begin base64 encoding ===
 *     offset 2:    {body}
 *     offset ?:    {crc16} (if final frame)
 *     === End base64 encoding ===
 *     offset ?:    0x0a (newline)
 *
 * All integers are represented in big-endian.  The packet fields are described
 * below:
 *
 * | Field          | Description                                             |
 * | -------------- | ------------------------------------------------------- |
 * | 0x06 0x09      | Byte pair indicating the start of a packet.             |
 * | 0x04 0x14      | Byte pair indicating the start of a continuation frame. |
 * | Packet length  | The combined total length of the *unencoded* body.      |
 * | Body           | The actual SMP data (i.e., 8-byte header and CBOR       |
 * |                | key-value map).                                         |
 * | CRC16          | A CRC16 of the *unencoded* body of the entire packet.   |
 * |                | This field is only present in the final frame of a      |
 * |                | packet.                                                 |
 * | Newline        | A 0x0a byte; terminates a frame.                        |
 *
 * The packet is fully received when {packet-length} bytes of body has been
 * received.
 *
 * ## CRC details
 *
 * The CRC16 should be calculated with the following parameters:
 *
 * | Field         | Value         |
 * | ------------- | ------------- |
 * | Polynomial    | 0x1021        |
 * | Initial Value | 0             |
 */

#ifndef ZEPHYR_INCLUDE_MGMT_SERIAL_H_
#define ZEPHYR_INCLUDE_MGMT_SERIAL_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MCUMGR_SERIAL_HDR_PKT       0x0609
#define MCUMGR_SERIAL_HDR_FRAG      0x0414
#define MCUMGR_SERIAL_MAX_FRAME     128

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
	u16_t pkt_len;
};

/** @typedef mcumgr_serial_tx_cb
 * @brief Transmits a chunk of raw response data.
 *
 * @param data                  The data to transmit.
 * @param len                   The number of bytes to transmit.
 * @param arg                   An optional argument.
 *
 * @return                      0 on success; negative error code on failure.
 */
typedef int (*mcumgr_serial_tx_cb)(const void *data, int len, void *arg);

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
	const u8_t *frag, int frag_len);

/**
 * @brief Encodes and transmits an mcumgr packet over serial.
 *
 * @param data                  The mcumgr packet data to send.
 * @param len                   The length of the unencoded mcumgr packet.
 * @param cb                    A callback used to transmit raw bytes.
 * @param arg                   An optional argument to pass to the callback.
 *
 * @return                      0 on success; negative error code on failure.
 */
int mcumgr_serial_tx_pkt(const u8_t *data, int len, mcumgr_serial_tx_cb cb,
			 void *arg);

#ifdef __cplusplus
}
#endif

#endif
