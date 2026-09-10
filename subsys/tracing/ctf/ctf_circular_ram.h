/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_TRACING_CTF_CIRCULAR_RAM_H
#define SUBSYS_TRACING_CTF_CIRCULAR_RAM_H

#include <stdint.h>

/**
 * @brief Store one complete CTF event in the circular RAM capture buffer.
 *
 * This is intentionally a CTF-local output path, rather than a tracing
 * backend. @p data must contain exactly one event as assembled by
 * CTF_GATHER_FIELDS().
 */
void ctf_circular_ram_write(const uint8_t *data, uint32_t length);

#endif /* SUBSYS_TRACING_CTF_CIRCULAR_RAM_H */
