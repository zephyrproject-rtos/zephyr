/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_TESTS_INTEL_ADSP_TESTS_H
#define ZEPHYR_TESTS_INTEL_ADSP_TESTS_H

#include <intel_adsp_ipc.h>
#include <cavstool.h>

/* Helper to escape from infinite polling loops with a test failure
 * instead of a hang.  Spins with a relaxation loop so that it works
 * in interrupt context and doesn't stress shared resources like SRAM
 */
#define AWAIT(expr) do {                                                \
		int i;							\
		for (i = 0; !(expr) && i < 10000; i++) {		\
			for (volatile int j = 0; j < 1000; j++) {	\
			}						\
		}							\
		zassert_true(i < 10000, "timeout waiting for %s", #expr); \
	} while (0)

/* Cached copy of the ipm_cavs_host driver's handler.  We save it at
 * the start of the test because we want to do unit testing on the
 * underlying adsp_ipc device, then recover it later.
 */
extern intel_adsp_ipc_handler_t ipm_handler;

#endif /* ZEPHYR_TESTS_INTEL_ADSP_TESTS_H */
