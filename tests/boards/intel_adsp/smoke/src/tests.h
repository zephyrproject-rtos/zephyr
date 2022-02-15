/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_TESTS_INTEL_ADSP_TESTS_H
#define ZEPHYR_TESTS_INTEL_ADSP_TESTS_H

#include <cavs_ipc.h>

/* Helper to escape from infinite polling loops with a test failure
 * instead of a hang.  Spins with a relaxation loop so that it works
 * in interrupt context and doesn't stress shared resources like SRAM
 */
#define WAIT_FOR(expr) do {						\
		int i;							\
		for (i = 0; !(expr) && i < 10000; i++) {		\
			for (volatile int j = 0; j < 1000; j++) {	\
			}						\
		}							\
		zassert_true(i < 10000, "timeout waiting for %s", #expr); \
	} while (0)

/* The cavstool.py script that launched us listens for a very simple
 * set of IPC commands to help test.  Pass one of the following values
 * as the "data" argument to cavs_ipc_send_message(CAVS_HOST_DEV, ...):
 */
enum cavstool_cmd {
	/* The host takes no action, but signals DONE to complete the message */
	IPCCMD_SIGNAL_DONE,

	/* The host returns done after a short timeout */
	IPCCMD_ASYNC_DONE_DELAY,

	/* The host issues a new message with the ext_data value as its "data" */
	IPCCMD_RETURN_MSG,

	/* The host writes the given value to ADSPCS */
	IPCCMD_ADSPCS,

	/* The host emits a (real/host time) timestamp into the log stream */
	IPCCMD_TIMESTAMP,

	/* The host copies OUTBOX[ext_data >> 16] to INBOX[ext_data & 0xffff] */
	IPCCMD_WINCOPY,
};

void test_post_boot_ipi(void);
void test_smp_boot_delay(void);
void test_host_ipc(void);
void test_cpu_behavior(void);
void test_cpu_halt(void);
void test_ipm_cavs_host(void);

/* Cached copy of the ipm_cavs_host driver's handler.  We save it at
 * the start of the test because we want to do unit testing on the
 * underlying cavs_ipc device, then recover it later.
 */
extern cavs_ipc_handler_t ipm_handler;

#endif /* ZEPHYR_TESTS_INTEL_ADSP_TESTS_H */
