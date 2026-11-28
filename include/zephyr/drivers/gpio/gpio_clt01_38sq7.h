/*
 * Copyright (c) 2026 Sebastian Balling
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended API of the ST CLT01-38SQ7 GPIO driver.
 *
 * The inputs are used through the regular GPIO API. This header adds access
 * to the control byte of the 16-bit SPI frame (datasheet DS14432, table 8).
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_CLT01_38SQ7_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_CLT01_38SQ7_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** /UVA: low when the field supply is below the undervoltage threshold (~17 V). */
#define CLT01_38SQ7_STATUS_NUVA BIT(7)
/** /OTA: low when the junction temperature is above the alarm threshold (~150 °C). */
#define CLT01_38SQ7_STATUS_NOTA BIT(6)
/** Parity check bits PC1..PC4. */
#define CLT01_38SQ7_STATUS_PC1  BIT(5)
#define CLT01_38SQ7_STATUS_PC2  BIT(4)
#define CLT01_38SQ7_STATUS_PC3  BIT(3)
#define CLT01_38SQ7_STATUS_PC4  BIT(2)
/** Fixed low bit. */
#define CLT01_38SQ7_STATUS_LOW  BIT(1)
/** Stop bit, always high. Reads low when the chip has lost its supply. */
#define CLT01_38SQ7_STATUS_STOP BIT(0)

/**
 * @brief Get the control byte of the last valid SPI frame.
 *
 * @param dev CLT01-38SQ7 device.
 * @param status Filled with the control byte (see CLT01_38SQ7_STATUS_*).
 *
 * @retval 0 on success.
 * @retval -ENODATA if no valid frame has been received yet.
 */
int clt01_38sq7_get_status(const struct device *dev, uint8_t *status);

/**
 * @brief Check a control byte for alarm conditions.
 *
 * @return true if neither the undervoltage nor the over-temperature alarm is active.
 */
static inline bool clt01_38sq7_status_ok(uint8_t status)
{
	const uint8_t alarms = CLT01_38SQ7_STATUS_NUVA | CLT01_38SQ7_STATUS_NOTA;

	return (status & alarms) == alarms;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_CLT01_38SQ7_H_ */
