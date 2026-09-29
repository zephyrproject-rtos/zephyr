/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define TX_FIFO_SIZE DT_PROP(DT_NODELABEL(nus_uart), tx_fifo_size)

/* Written by the DUT before any peer is subscribed. Longer than the TX FIFO,
 * so the writer finishes only if the driver drops what does not fit, and the
 * first subscriber receives the first TX_FIFO_SIZE bytes of it.
 */
#define UNSUBSCRIBED_MSG "unsubscribed-unsubscribed-unsubscribed-unsubscribed-unsubscribed-"
BUILD_ASSERT(sizeof(UNSUBSCRIBED_MSG) - 1 > TX_FIFO_SIZE);

#define SUBSCRIBED_MSG "subscribed"

#define FIRST_SESSION_MSG "first-session"

/* Queued while the first peer is subscribed and still queued when it
 * disconnects; must never be delivered. Longer than one notification at the
 * default ATT MTU and no longer than the TX FIFO.
 */
#define HELD_MSG "held-held-held-held-held-held-"

/* Written while no peer is connected; delivered to the next subscriber. */
#define DISCONNECTED_MSG "disconnected-disconnected-"

#define SECOND_SESSION_MSG "second-session"

/* Written until the driver stops taking it, because the peer has stopped
 * reading notifications and one is waiting for a buffer.
 */
#define STALLED_BYTE 'o'

/* Written after the peer unsubscribes and subscribes again while that
 * notification is still waiting; delivered exactly once.
 */
#define RESUBSCRIBED_BYTE 'n'
#define RESUBSCRIBED_LEN  TX_FIFO_SIZE
