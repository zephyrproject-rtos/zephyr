/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DELTA_APPLY_H_
#define DELTA_APPLY_H_

#include <zephyr/mgmt/delta/delta.h>

/**
 * @brief Initialize the delta API and apply the patch using the configured backend.
 *
 * Reboots the device after writing the new firmware.
 *
 * @param ctx Pointer to delta context structure.
 * @return 0 on success, negative error code on failure.
 */
int delta_apply(struct delta_ctx *ctx);

#endif /* DELTA_APPLY_H_ */
