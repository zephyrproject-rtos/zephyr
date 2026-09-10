/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_bus.h
 * @brief CS47L63 32-bit register access over SPI (internal)
 *
 * The only door to the chip. No other translation unit in this driver issues
 * an SPI transaction.
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_BUS_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_BUS_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check whether the codec's SPI bus is ready.
 *
 * @param dev Codec device
 * @return true when the bus is ready to transfer
 */
bool cs47l63_bus_is_ready(const struct device *dev);

/**
 * @brief Write one 32-bit register.
 *
 * On the wire: four address bytes, most significant first, then four data
 * bytes in the same order.
 *
 * @param dev  Codec device
 * @param addr Register address
 * @param val  Value to write
 * @return 0 on success, the SPI transaction's negative errno on failure
 */
int cs47l63_bus_write_reg(const struct device *dev, uint32_t addr, uint32_t val);

/**
 * @brief Read one 32-bit register.
 *
 * On the wire: four address bytes with the top bit of the first set to mark a
 * read, then four padding bytes the part uses to turn the bus around, then the
 * four data bytes. The padding is consumed here and never reaches @p val -
 * delivering it instead yields data shifted by four bytes, which reads as
 * plausible garbage rather than as an error.
 *
 * @param dev  Codec device
 * @param addr Register address. Only the low 31 bits are addressable; the top
 *             bit is this driver's read marker and is never part of an address.
 * @param[out] val Value read. Left untouched when the transaction fails.
 * @return 0 on success, the SPI transaction's negative errno on failure
 */
int cs47l63_bus_read_reg(const struct device *dev, uint32_t addr, uint32_t *val);

/**
 * @brief Read-modify-write the @p mask bits of one register.
 *
 * Registers on this part mix fields owned by different units, so an unmasked
 * field write silently undoes a neighbour's setting.
 *
 * @param dev  Codec device
 * @param addr Register address
 * @param mask Bits this call owns; every other bit keeps its value
 * @param val  New value for the masked bits, already in position
 * @return 0 on success, negative errno on failure
 */
int cs47l63_bus_update_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t val);

/**
 * @brief Poll one register until every bit of @p mask reaches @p expected.
 *
 * Bounded by construction: it sleeps @p interval_ms between reads and gives up
 * after @p max_polls of them. Used for the three places this part makes the
 * driver wait on silicon - boot-done, FLL lock, and output-stage enable.
 *
 * @param dev         Codec device
 * @param addr        Register address
 * @param mask        Bits to watch
 * @param expected    Value those bits must reach, already in position
 * @param interval_ms Delay between reads, in milliseconds
 * @param max_polls   Number of reads to make before giving up
 * @return 0 once the bits match
 * @return -ETIMEDOUT when they never do
 * @return negative errno from the failing read
 */
int cs47l63_bus_poll_reg(const struct device *dev, uint32_t addr, uint32_t mask, uint32_t expected,
			 uint32_t interval_ms, uint32_t max_polls);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_BUS_H_ */
