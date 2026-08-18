/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Peer of the "ping" applet.
 *
 * A single thread that folds its own seed into every value the "ping" applet
 * puts on the shared link. It never returns on its own, so it is the applet to
 * use when demonstrating "applet kill".
 */

#include <zephyr/kernel.h>

#include "applet_link.h"

/** Delay between two polls of the shared link, in milliseconds. */
#define PONG_POLL_MS 5

/*
 * k_msleep() would leave a call to the 64-bit division helper of libgcc, which
 * the extension loader cannot resolve. Converting at the call site keeps the
 * conversion constant.
 */
#define PONG_SLEEP(ms) k_sleep(K_MSEC(ms))

void applet_main(void *arg)
{
	struct applet_link *link = applet_link_get();
	uint32_t seed = (uint32_t)(uintptr_t)arg;

	printk("[pong] folding %u into every exchange until killed\n", seed);

	while (true) {
		if (link->state != APPLET_LINK_TO_PONG) {
			PONG_SLEEP(PONG_POLL_MS);
			continue;
		}

		link->token = link->token + seed;
		link->exchanges = link->exchanges + 1u;
		link->state = APPLET_LINK_TO_PING;
	}
}
