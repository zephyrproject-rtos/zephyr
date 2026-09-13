/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file cs47l63_boot.h
 * @brief CS47L63 reset, boot-done wait, identification and trim (internal)
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_BOOT_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_BOOT_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Take the part from power-on to a register file that can be trusted.
 *
 * Drives reset, waits for the part to report boot-done, identifies it, and
 * applies the OTP-variant trim block. Nothing analog is enabled here and no
 * datapath register is written: after this returns the caller still has to
 * configure the clock, the serial port and the output.
 *
 * The three ways this can fail are told apart by errno, because at bring-up
 * they call for completely different next moves:
 *
 * @param dev Codec device
 * @retval 0 on success
 * @retval -ETIMEDOUT boot-done never asserted - the part is not running
 * @retval -ENODEV the device ID read back is not a plausible one - usually
 *         wiring, chip select, or SPI mode
 * @retval other negative errno propagated from the failing bus transaction
 */
int cs47l63_boot_bringup(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_BOOT_H_ */
