/*
 * Copyright (c) 2026 Hsiu-Chi Tsai
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_EMUL_ES8311_H_
#define ZEPHYR_DRIVERS_AUDIO_EMUL_ES8311_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/kernel.h>

/* Fail the next n transfers with -EIO. */
void emul_es8311_set_fail(const struct emul *target, int n);

/* Fail the zero-based transfer idx once; a negative index disables the fault. */
void emul_es8311_fail_at(const struct emul *target, int idx);

/* Fail transfer idx and all later transfers; a negative index disables the fault. */
void emul_es8311_fail_from(const struct emul *target, int idx);

/* Apply writes to reg before returning -EIO; a negative register disables the fault. */
void emul_es8311_fail_write_landed(const struct emul *target, int reg);

/* Reject writes to reg; a negative register disables the fault. */
void emul_es8311_fail_write_to(const struct emul *target, int reg);

/* Reject reads while allowing writes. */
void emul_es8311_fail_reads(const struct emul *target, bool fail);

void emul_es8311_set_chip_id(const struct emul *target, uint8_t id1, uint8_t id2);

/* Transfer-level faults return before logging; register-specific faults are logged. */
void emul_es8311_reset_log(const struct emul *target);
int emul_es8311_write_count(const struct emul *target);

/* Read a logged register or value; unavailable entries return -1. */
int emul_es8311_write_at(const struct emul *target, int idx);
int emul_es8311_wval_at(const struct emul *target, int idx);

/* Pause before writing reg, in the caller's context, until release(). */
void emul_es8311_pause_before(const struct emul *target, uint8_t reg);
int emul_es8311_wait_paused(const struct emul *target, k_timeout_t timeout);
void emul_es8311_release(const struct emul *target);

/* Reset registers, fault injection and logging; driver state is unaffected. */
void emul_es8311_reset(const struct emul *target);

#endif /* ZEPHYR_DRIVERS_AUDIO_EMUL_ES8311_H_ */
