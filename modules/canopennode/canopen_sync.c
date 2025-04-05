/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <CANopen.h>
#include "OD.h"

/**
 * @brief CANopen sync thread.
 *
 * The CANopen real-time sync thread processes SYNC RPDOs and TPDOs
 * through the CANopenNode stack with an interval of 1 millisecond.
 *
 * @param p1 Unused
 * @param p2 Unused
 * @param p3 Unused
 */
static void canopen_sync_thread(void *p1, void *p2, void *p3)
{
	uint32_t start; /* cycles */
	uint32_t stop;  /* cycles */
	uint32_t delta; /* cycles */
	uint32_t elapsed = 0; /* microseconds */
	CO_SYNC_status_t sync;
	CO_t *co = CO_new(NULL, NULL); /* FIXME */

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		uint32_t next = 1000;
		start = k_cycle_get_32();

		CO_LOCK_OD();
		sync = CO_process_SYNC(co, elapsed, &next);
		CO_process_RPDO(co, sync == CO_SYNC_RX_TX, elapsed, &next);
		CO_process_TPDO(co, sync == CO_SYNC_RX_TX, elapsed, &next);
		CO_UNLOCK_OD();

		k_sleep(K_MSEC(1));
		stop = k_cycle_get_32();
		delta = stop - start;
		elapsed = (uint32_t)k_cyc_to_ns_floor64(delta) / NSEC_PER_USEC;
	}
}

K_THREAD_DEFINE(canopen_sync, CONFIG_CANOPENNODE_SYNC_THREAD_STACK_SIZE,
		canopen_sync_thread, NULL, NULL, NULL,
		CONFIG_CANOPENNODE_SYNC_THREAD_PRIORITY, 0, 1);
