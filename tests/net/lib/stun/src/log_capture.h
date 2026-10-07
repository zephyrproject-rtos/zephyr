/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
 * SPDX-License-Identifier: Apache-2.0
 *
 * A log backend that keeps what the STUN library writes, for the tests to look
 * at: the level and the text of every record of the "net_stun" module. What the
 * rest of the system logs is dropped.
 */
#ifndef STUN_TEST_LOG_CAPTURE_H_
#define STUN_TEST_LOG_CAPTURE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Start keeping records; the first call does it, the later ones do nothing. */
void log_capture_start(void);

/* Forget the records kept so far. */
void log_capture_clear(void);

/* Number of records since the last clear. */
size_t log_capture_count(void);

/* Number of them that have this level, LOG_LEVEL_ERR to LOG_LEVEL_DBG. */
size_t log_capture_count_level(uint8_t level);

/* Does the text of any record contain this? */
bool log_capture_contains(const char *needle);

/* Print the records, to go with a failed assertion. */
void log_capture_dump(void);

#endif /* STUN_TEST_LOG_CAPTURE_H_ */
