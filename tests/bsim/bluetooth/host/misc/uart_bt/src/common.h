/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Written by the DUT before any peer is subscribed; must never be delivered.
 * Longer than the TX FIFO, so the writer needs more than one TX-ready
 * callback to hand it all over.
 */
#define UNSUBSCRIBED_MSG "unsubscribed-unsubscribed-unsubscribed-unsubscribed-unsubscribed-"

#define SUBSCRIBED_MSG "subscribed"

#define FIRST_SESSION_MSG "first-session"

/* Queued while the first peer is subscribed and still queued when it
 * disconnects; must never be delivered. Longer than one notification at the
 * default ATT MTU and no longer than the TX FIFO.
 */
#define HELD_MSG "held-held-held-held-held-held-"

#define DISCONNECTED_MSG "disconnected-disconnected-"

#define SECOND_SESSION_MSG "second-session"
