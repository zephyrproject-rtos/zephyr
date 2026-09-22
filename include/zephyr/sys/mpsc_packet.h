/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_MPSC_PACKET_H_
#define ZEPHYR_INCLUDE_SYS_MPSC_PACKET_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Multi producer, single consumer packet header
 * @defgroup mpsc_packet MPSC (Multi producer, single consumer) packet header
 * @ingroup mpsc_buf
 * @{
 */

/** @brief Number of bits in the first word which are used by the buffer. */
#define MPSC_PBUF_HDR_BITS 2

/** @brief Header that must be added to the first word in each packet.
 *
 * This fields are controlled by the packet buffer and unless specified must
 * not be used. Fields must be added at the top of the packet header structure.
 */
#define MPSC_PBUF_HDR \
	uint32_t valid: 1; \
	uint32_t busy: 1

/** @brief Generic packet header. */
struct mpsc_pbuf_hdr {
	MPSC_PBUF_HDR;
	uint32_t data: 32 - MPSC_PBUF_HDR_BITS;
};

/** @brief Skip packet used internally by the packet buffer. */
struct mpsc_pbuf_skip {
	MPSC_PBUF_HDR;
	uint32_t len: 32 - MPSC_PBUF_HDR_BITS;
};

/** @brief Generic packet header. */
union mpsc_pbuf_generic {
	struct mpsc_pbuf_hdr hdr;
	struct mpsc_pbuf_skip skip;
	uint32_t raw;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_MPSC_PACKET_H_ */
