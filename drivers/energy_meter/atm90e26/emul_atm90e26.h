/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ENERGY_METER_ATM90E26_EMUL_ATM90E26_H_
#define ZEPHYR_DRIVERS_ENERGY_METER_ATM90E26_EMUL_ATM90E26_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

/** Set a register without going through the SPI protocol. */
void atm90e26_emul_set_reg(const struct emul *target, uint8_t reg, uint16_t val);

/** Get a register without going through the SPI protocol. */
uint16_t atm90e26_emul_get_reg(const struct emul *target, uint8_t reg);

/** Number of soft resets received since the emulator was created. */
uint32_t atm90e26_emul_reset_count(const struct emul *target);

/**
 * Corrupt the data word of the next @p count register reads on the wire. LastData keeps the
 * value the chip sent; on UART the frame checksum still covers the original value.
 */
void atm90e26_emul_corrupt_reads(const struct emul *target, uint32_t count);

/** Drop the next @p count register writes. */
void atm90e26_emul_drop_writes(const struct emul *target, uint32_t count);

/** Simulate a missing chip: reads return 0xFFFF and writes are ignored. */
void atm90e26_emul_set_present(const struct emul *target, bool present);

#endif /* ZEPHYR_DRIVERS_ENERGY_METER_ATM90E26_EMUL_ATM90E26_H_ */
