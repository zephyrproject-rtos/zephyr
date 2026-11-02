/*
 * Copyright (c) 2026 iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TLC6C59XX MFD child-driver interface.
 * @ingroup mfd_interface_tlc6c59xx
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_TLC6C59XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_TLC6C59XX_H_

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_tlc6c59xx TLC6C59XX MFD interface
 * @ingroup mfd_interfaces
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/**
 * @brief Write the output word of one chip in a TLC6C59XX chain.
 *
 * Shift the complete chain, preserving the last successfully written words of
 * the other chips. Words are transmitted most significant bit first, starting
 * with the chip farthest from the SPI controller. A set bit enables the
 * corresponding current-sink output.
 *
 * This function serializes accesses to the parent device and may block. It is
 * intended for child drivers. Applications using GPIO children must use the
 * GPIO API instead, so the children's cached output state remains consistent.
 *
 * On a transfer error, the cached output word is restored. The physical output
 * state is not guaranteed after a partially completed transfer; a subsequent
 * successful write transmits the complete cached chain again.
 *
 * @param dev Ready TLC6C59XX parent device. Must not be NULL.
 * @param pos Zero-based chain position, smaller than the number of chips.
 *            Position 0 is nearest the SPI controller.
 * @param value Output word. Only the low 8 or 12 bits, according to the chip's
 *              ngpios property, may be set.
 *
 * @retval 0 Output word written successfully.
 * @retval -EINVAL Chain position is out of range or the output word
 *                 contains bits outside the configured width.
 * @retval -EWOULDBLOCK Called from interrupt context.
 * @return Negative error code from the mutex or SPI driver on failure.
 */
int tlc6c59xx_write_state(const struct device *dev, uint8_t pos, uint16_t value);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_TLC6C59XX_H_ */
