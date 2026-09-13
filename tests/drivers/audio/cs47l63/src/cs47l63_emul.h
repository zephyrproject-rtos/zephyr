/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_emul.h
 * @brief Recording SPI emulator for the CS47L63, for the driver's own tests
 *
 * The part is a 32-bit register file behind a three-word SPI frame. This
 * emulator holds that register file, serves reads from it, and records every
 * transaction the driver makes in order, so a test can assert both the value a
 * register ended at and the sequence the driver reached it by.
 */

#ifndef CS47L63_EMUL_H_
#define CS47L63_EMUL_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

/** One recorded register transaction. */
struct cs47l63_emul_xfer {
	/** Register address. */
	uint32_t addr;
	/** Value written, or value served back on a read. */
	uint32_t val;
	/** true for a write, false for a read. */
	bool write;
};

/**
 * @brief Clear the register file and the transaction log, and install the
 *        power-on values a healthy part answers with.
 *
 * Those are a plausible device ID, the OTP variant whose trim block the driver
 * knows, and the boot-done edge flag already latched. A test that wants an
 * unhealthy part overrides one of them with @ref cs47l63_emul_set_reg.
 */
void cs47l63_emul_reset(const struct emul *target);

/** @brief Force a register's value, as the silicon would report it. */
void cs47l63_emul_set_reg(const struct emul *target, uint32_t addr, uint32_t val);

/** @brief Read a register's current value out of the emulated file. */
uint32_t cs47l63_emul_get_reg(const struct emul *target, uint32_t addr);

/** @brief Number of transactions recorded since the last reset. */
uint32_t cs47l63_emul_xfer_count(const struct emul *target);

/**
 * @brief Fetch one recorded transaction by index.
 *
 * @return true when @p idx named a recorded transaction.
 */
bool cs47l63_emul_xfer_get(const struct emul *target, uint32_t idx, struct cs47l63_emul_xfer *out);

/** @brief Number of writes the driver made to @p addr. */
uint32_t cs47l63_emul_write_count(const struct emul *target, uint32_t addr);

/** @brief Number of reads the driver made of @p addr. */
uint32_t cs47l63_emul_read_count(const struct emul *target, uint32_t addr);

/**
 * @brief Log index of the @p nth (zero-based) write to @p addr.
 *
 * @return the index, or -1 when there was no such write. Indices are
 *         comparable, so this is how a test states that one register was
 *         written before another.
 */
int cs47l63_emul_write_index(const struct emul *target, uint32_t addr, uint32_t nth);

/** @brief Log index of the @p nth (zero-based) read of @p addr, or -1. */
int cs47l63_emul_read_index(const struct emul *target, uint32_t addr, uint32_t nth);

/**
 * @brief Value of the @p nth (zero-based) write to @p addr.
 *
 * @return true when there was such a write.
 */
bool cs47l63_emul_nth_write(const struct emul *target, uint32_t addr, uint32_t nth, uint32_t *val);

/**
 * @brief Make every transaction touching @p addr fail with -EIO.
 *
 * Cleared by @ref cs47l63_emul_reset. Only one address is armed at a time.
 */
void cs47l63_emul_fail_at(const struct emul *target, uint32_t addr);

#endif /* CS47L63_EMUL_H_ */
