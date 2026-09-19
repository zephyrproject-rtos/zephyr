/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file rtp_packet.h
 * @brief Internal functions to serialize and deserialize RTP packets
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_RTP_RTP_PACKET_H_
#define ZEPHYR_SUBSYS_NET_LIB_RTP_RTP_PACKET_H_

#include <zephyr/net/rtp.h>

/**
 * @brief Deserialize a raw buffer into an RTP packet.
 *
 * The packet's payload and header extension data pointers reference @p data
 * and remain valid only as long as the buffer itself.
 *
 * @param packet Packet structure to fill.
 * @param data   Buffer holding a serialized RTP packet.
 * @param len    Length of @p data in bytes.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_packet_deserialize(struct rtp_packet *packet, uint8_t *data, size_t len);

/**
 * @brief Serialize an RTP packet into a contiguous buffer.
 *
 * @param packet  Packet to serialize.
 * @param padding Number of padding bytes to append (0-255); 0 for no padding.
 * @param buf     Destination buffer.
 * @param buf_size Size of @p buf in bytes.
 *
 * @retval >=0      Total number of bytes written.
 * @retval negative Errno value on failure.
 */
int rtp_packet_serialize(const struct rtp_packet *packet, uint8_t padding, uint8_t *buf,
			 size_t buf_size);

/**
 * @brief Get the serialized RTP header length of a packet.
 *
 * Fixed header plus CSRC list plus optional header extension; this equals the
 * offset of the payload in the serialized packet.
 *
 * @param packet Packet to inspect.
 *
 * @return Header length in bytes.
 */
size_t rtp_packet_hdr_len(const struct rtp_packet *packet);

#endif /* ZEPHYR_SUBSYS_NET_LIB_RTP_RTP_PACKET_H_ */
