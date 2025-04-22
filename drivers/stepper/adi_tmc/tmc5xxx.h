/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_5XXX_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_5XXX_H_

#include <stdint.h>

#define TMC5XXX_SPI_STATUS_BITS	8

void log_status(const uint8_t status_byte, const char *spi_status[]);

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_5XXX_H_ */
