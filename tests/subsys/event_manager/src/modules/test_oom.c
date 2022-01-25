/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>

#include <test_events.h>
#include <data_event.h>
#include "test_oom.h"

#define MODULE test_oom
#define TEST_EVENTS_CNT 150

static struct data_event *event_tab[TEST_EVENTS_CNT];

/* Custom reboot handler to check if OOM error is handled. */
void oom_error_handler(void)
{
	int i = 0;

	/* Freeing memory to enable further testing. */
	while (event_tab[i] != NULL) {
		k_free(event_tab[i]);
		i++;
	}

	ztest_test_pass();

	while (true) {
		;
	}
}

void test_oom_reset(void)
{
	/* Sending large number of events to cause out of memory error. */
	int i;

	for (i = 0; i < ARRAY_SIZE(event_tab); i++) {
		event_tab[i] = new_data_event();
		if (event_tab[i] == NULL) {
			break;
		}
	}

	/* This shall only be executed if OOM is not triggered. */
	zassert_true(i < ARRAY_SIZE(event_tab),
		     "No OOM detected, increase TEST_EVENTS_CNT");
	zassert_unreachable("OOM error not detected");
}
