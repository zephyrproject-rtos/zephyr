/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Two-thread applet extension.
 *
 * applet_main() expands the seed passed in "arg" into a stream of samples and
 * hands them over to ping_sum(), a second thread attached to the same applet
 * with "applet add_thread ping ping_sum". Both threads run out of this
 * extension, so they share its memory and can talk through plain globals.
 *
 * ping_sum() passes every running total to the "pong" applet through the
 * shared link. That link lives in a partition of the shell application which
 * is added to both applets, so the two applets stay isolated from each other
 * except for this one window.
 */

#include <zephyr/kernel.h>

#include "applet_link.h"

/** Number of samples produced before the applet terminates. */
#define PING_ROUNDS     8
/** Delay between two samples, in milliseconds. */
#define PING_PERIOD_MS  250
/** Delay between two polls of a shared flag, in milliseconds. */
#define PING_POLL_MS    5
/** Number of polls before giving up on the peer applet. */
#define PING_PEER_TRIES 200

/*
 * k_msleep() would leave a call to the 64-bit division helper of libgcc, which
 * the extension loader cannot resolve. Converting at the call site keeps the
 * conversion constant.
 */
#define PING_SLEEP(ms) k_sleep(K_MSEC(ms))

/* Private to this extension, so only the threads of this applet can see it. */
static volatile uint32_t sample;
static volatile bool sample_ready;
static volatile bool sampling_done;

void applet_main(void *arg)
{
	uint32_t x = (uint32_t)(uintptr_t)arg;

	if (x == 0u) {
		x = 1u;
	}

	for (unsigned int i = 0; i < PING_ROUNDS; i++) {
		/* Numerical Recipes linear congruential generator. */
		x = (x * 1664525u) + 1013904223u;

		while (sample_ready) {
			PING_SLEEP(PING_POLL_MS);
		}

		sample = (x >> 24) & 0xffu;
		sample_ready = true;

		PING_SLEEP(PING_PERIOD_MS);
	}

	while (sample_ready) {
		PING_SLEEP(PING_POLL_MS);
	}
	sampling_done = true;

	printk("[ping/gen] produced %u samples\n", (unsigned int)PING_ROUNDS);
}

void ping_sum(void *arg)
{
	struct applet_link *link = applet_link_get();
	uint32_t total = 0u;
	unsigned int tries;

	ARG_UNUSED(arg);

	while (true) {
		if (!sample_ready) {
			if (sampling_done) {
				break;
			}
			PING_SLEEP(PING_POLL_MS);
			continue;
		}

		total += sample;
		sample_ready = false;

		link->token = total;
		link->state = APPLET_LINK_TO_PONG;

		tries = 0;
		while (link->state != APPLET_LINK_TO_PING) {
			if (++tries > PING_PEER_TRIES) {
				printk("[ping/sum] no answer from the pong applet\n");
				link->state = APPLET_LINK_IDLE;
				return;
			}
			PING_SLEEP(PING_POLL_MS);
		}

		total = link->token;
		link->state = APPLET_LINK_IDLE;

		printk("[ping/sum] exchange %u: total %u\n", link->exchanges, total);
	}

	printk("[ping/sum] final total %u\n", total);
}
