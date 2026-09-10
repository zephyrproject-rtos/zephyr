/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_emul.h
 * @brief Recording SPI emulator for the CS47L63, for the driver's own tests
 */

#ifndef CS47L63_EMUL_H_
#define CS47L63_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

struct cs47l63_emul_xfer {
	uint32_t addr;
	/** Value written, or value served back on a read. */
	uint32_t val;
	bool write;
};

/** Installs a healthy part: the datasheet DEVID, OTP variant 8, boot-done latched. */
void cs47l63_emul_reset(const struct emul *target);

void cs47l63_emul_set_reg(const struct emul *target, uint32_t addr, uint32_t val);

uint32_t cs47l63_emul_get_reg(const struct emul *target, uint32_t addr);

uint32_t cs47l63_emul_xfer_count(const struct emul *target);

bool cs47l63_emul_xfer_get(const struct emul *target, uint32_t idx, struct cs47l63_emul_xfer *out);

uint32_t cs47l63_emul_write_count(const struct emul *target, uint32_t addr);

uint32_t cs47l63_emul_read_count(const struct emul *target, uint32_t addr);

/** Returns -1 for no such write; indices order transactions against each other. */
int cs47l63_emul_write_index(const struct emul *target, uint32_t addr, uint32_t nth);

int cs47l63_emul_read_index(const struct emul *target, uint32_t addr, uint32_t nth);

bool cs47l63_emul_nth_write(const struct emul *target, uint32_t addr, uint32_t nth, uint32_t *val);

/** Armed until the next reset, for one address at a time. */
void cs47l63_emul_fail_at(const struct emul *target, uint32_t addr);

#endif /* CS47L63_EMUL_H_ */
