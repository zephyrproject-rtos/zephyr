/*
 * Copyright (c) 2020 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *  Run this test from zephyr directory as:
 *
 *     ./scripts/twister --coverage -p native_posix -v -T tests/bluetooth/ctrl_isoal/
 *
 */

#ifndef _ISOAL_TEST_DEBUG_H_
#define _ISOAL_TEST_DEBUG_H_

/*#define DEBUG_TEST			(1)*/
/*#define DEBUG_TRACE			(1)*/

extern void isoal_test_debug_print_rx_pdu(struct isoal_pdu_rx *pdu_meta);
extern void isoal_test_debug_print_rx_sdu(const struct isoal_sink             *sink_ctx,
					  const struct isoal_emitted_sdu_frag *sdu_frag,
					  const struct isoal_emitted_sdu      *sdu);
extern void isoal_test_debug_print_tx_pdu(struct node_tx_iso *node_tx);
extern void isoal_test_debug_print_tx_sdu(struct isoal_sdu_tx *tx_sdu);

extern void isoal_test_debug_trace_func_call(const uint8_t *func, const uint8_t *status);

#endif /* _ISOAL_TEST_DEBUG_H_ */
