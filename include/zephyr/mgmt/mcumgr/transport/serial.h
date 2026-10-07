/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the MCUmgr serial transport API.
 * @ingroup mcumgr_transport_serial
 */

#ifndef ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SERIAL_H_
#define ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SERIAL_H_

/**
 * @brief This allows to use the MCUmgr SMP protocol over serial.
 * @defgroup mcumgr_transport_serial Serial transport
 * @ingroup mcumgr_transport
 * @{
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Serial packet header */
#define MCUMGR_SERIAL_HDR_PKT       0x0609
/** Serial fragment header */
#define MCUMGR_SERIAL_HDR_FRAG      0x0414
/** Maximum frame size */
#define MCUMGR_SERIAL_MAX_FRAME     127

/** First byte of packet header */
#define MCUMGR_SERIAL_HDR_PKT_1     (MCUMGR_SERIAL_HDR_PKT >> 8)
/** Second byte of packet header */
#define MCUMGR_SERIAL_HDR_PKT_2     (MCUMGR_SERIAL_HDR_PKT & 0xff)
/** First byte of fragment header */
#define MCUMGR_SERIAL_HDR_FRAG_1    (MCUMGR_SERIAL_HDR_FRAG >> 8)
/** Second byte of fragment header */
#define MCUMGR_SERIAL_HDR_FRAG_2    (MCUMGR_SERIAL_HDR_FRAG & 0xff)

/**
 * @brief Maintains state for an incoming mcumgr request packet.
 */
struct mcumgr_serial_rx_ctxt {
	/** Contains the partially- or fully-received mcumgr request.  Data
	 * stored in this buffer has already been base64-decoded.
	 */
	struct net_buf *nb;

	/** Length of full packet, as read from header. */
	uint16_t pkt_len;

#if defined(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_SMP_OVER_CONSOLE) && \
	defined(CONFIG_MCUMGR_TRANSPORT_SERIAL_HAS_RAW_BINARY_NON_SMP_OVER_CONSOLE)
	/**
	 * Set to true by transports which use raw MCUmgr over UART (not SMP over console) to know
	 * not to deal with the extra base64 etc. processing.
	 */
	const bool raw_transport;
#endif
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
 * For raw (non SMP over console) data that may hold more than one packet, use
 * mcumgr_serial_process_raw() instead.
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
 * @brief Processes received raw (non SMP over console) mcumgr data.
 *
 * Uses data only up to the end of the packet currently being received, so a
 * buffer holding the end of one packet and the start of the next can be
 * processed by calling this function again with the rest of the buffer until
 * all of it has been consumed. Invalid packets are dropped at the same points
 * as when the data is passed to mcumgr_serial_process_frag() one byte at a
 * time.
 *
 * @param rx_ctxt               The receive context associated with the serial
 *                                  transport being used.
 * @param data                  The received data.
 * @param len                   The length of the data, in bytes.
 * @param consumed              Set to the number of bytes of the data that
 *                                  were used, which is at least 1 if len is
 *                                  not 0.
 *
 * @return                      A net_buf containing the decoded request if a
 *                                  complete and valid request has been
 *                                  received. It is the caller's
 *                                  responsibility to free it.
 *                              NULL if the packet is incomplete or invalid.
 */
struct net_buf *mcumgr_serial_process_raw(struct mcumgr_serial_rx_ctxt *rx_ctxt,
					  const uint8_t *data, size_t len, size_t *consumed);

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

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_MCUMGR_TRANSPORT_SERIAL_H_ */
