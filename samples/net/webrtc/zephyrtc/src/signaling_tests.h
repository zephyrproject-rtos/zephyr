/* SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc. */
/* SPDX-License-Identifier: Apache-2.0 */
#ifndef ZEPHYRTC_ZEPHYR_SIGNALING_TESTS_H_
#define ZEPHYRTC_ZEPHYR_SIGNALING_TESTS_H_

/*
 * Run the deterministic signaling failure matrix, then one complete HTTP-signaled call against the
 * in-process mock answerer. Returns 0 only when every case behaves as specified.
 */
int libpeer_signaling_tests_run(void);

#endif /* ZEPHYRTC_ZEPHYR_SIGNALING_TESTS_H_ */
