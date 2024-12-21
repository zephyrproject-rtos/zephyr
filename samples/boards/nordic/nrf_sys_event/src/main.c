/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf_sys_event.h>
#include <stdio.h>

int main(void)
{
	printf("request global constant latency mode\n");
	if (nrf_sys_event_request_global_constlat()) {
		printf("failed to request global constant latency mode\n");
		return 0;
	}
	printf("constant latency mode enabled\n");

	printf("request global constant latency mode again\n");
	if (nrf_sys_event_request_global_constlat()) {
		printf("failed to request global constant latency mode\n");
		return 0;
	}

	printf("release global constant latency mode\n");
	printf("constant latency mode will remain enabled\n");
	if (nrf_sys_event_release_global_constlat()) {
		printf("failed to release global constant latency mode\n");
		return 0;
	}

	printf("release global constant latency mode again\n");
	printf("constant latency mode will be disabled\n");
	if (nrf_sys_event_release_global_constlat()) {
		printf("failed to release global constant latency mode\n");
		return 0;
	}

	printf("constant latency mode disabled\n");
	return 0;
}
