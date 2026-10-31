/*
 * Copyright (c) 2026 Kent Hauser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test PDU meta data: a 16-bit length followed by the PDU. pdu[] starts 2 bytes into the
 * descriptor, so it is 2-byte aligned but not 4-byte aligned.
 */

#ifndef TEST_LLL_META_H_
#define TEST_LLL_META_H_

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint16_t length;
	uint8_t data[];
} lll_tx_pdu_meta_t;

/* data[2] stands in for an unusual SDK header structure. ULL and LLL use the pdu[] declared in
 * the node's union, which starts at data[] and extends past the end of this type.
 */
typedef struct {
	uint16_t length;
	uint8_t data[2];
} lll_rx_pdu_meta_t;

#define LLL_TX_PDU_META_PDU_OFFSET offsetof(lll_tx_pdu_meta_t, data)
#define LLL_RX_PDU_META_PDU_OFFSET offsetof(lll_rx_pdu_meta_t, data)

#define BT_CTLR_USER_TX_BUFFER_OVERHEAD 4U /* e.g. MIC room for in-place encryption */
#define BT_CTLR_USER_RX_BUFFER_OVERHEAD 4U /* e.g. status the radio appends */

#endif /* TEST_LLL_META_H_ */
