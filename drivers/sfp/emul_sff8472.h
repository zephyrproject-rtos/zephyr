/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SFP_EMUL_SFF8472_H_
#define ZEPHYR_DRIVERS_SFP_EMUL_SFF8472_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

/** Restore the emulated module's canned memory image. */
void emul_sff8472_reset(const struct emul *target);

/**
 * Change one byte of the emulated module's memory.
 *
 * @param target Emulator instance.
 * @param diag_page true for the diagnostics page (A2h), false for A0h.
 * @param offset Byte offset in the page.
 * @param value New byte value.
 *
 * @retval 0 Always.
 */
int emul_sff8472_set_byte(const struct emul *target, bool diag_page, uint8_t offset,
			  uint8_t value);

#endif /* ZEPHYR_DRIVERS_SFP_EMUL_SFF8472_H_ */
