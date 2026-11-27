/*
 * Copyright (c) 2026 Kent Hauser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TEST_LLL_META_H_
#define TEST_LLL_META_H_

#include <stddef.h>
#include <stdint.h>

/* TX descriptor: a 16-bit length, then the PDU. pdu[] starts 2 bytes in: 2-byte aligned, not
 * 4-byte aligned.
 */
typedef struct {
	uint16_t length;
	uint8_t data[];
} lll_tx_pdu_meta_t;

/* RX descriptor with a fixed-size last field, data[2], as a vendor SDK's buffer header may
 * declare it. pdu[] starts at data and runs past the end of this type: the PDU is in the node,
 * not in the descriptor.
 */
typedef struct {
	uint16_t length;
	uint8_t data[2];
} lll_rx_pdu_meta_t;

#define LLL_TX_PDU_META_PDU_OFFSET offsetof(lll_tx_pdu_meta_t, data)
#define LLL_RX_PDU_META_PDU_OFFSET offsetof(lll_rx_pdu_meta_t, data)

/* Arbitrary non-zero values: they exercise the pool overhead paths. */
#define BT_CTLR_USER_TX_BUFFER_OVERHEAD 4U
#define BT_CTLR_USER_RX_BUFFER_OVERHEAD 4U

#endif /* TEST_LLL_META_H_ */
