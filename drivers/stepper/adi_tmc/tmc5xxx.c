/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "tmc5xxx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc5xxx, CONFIG_STEPPER_LOG_LEVEL);

#if defined(CONFIG_STEPPER_ADI_TMC50XX_LOG_STATUS) || defined(CONFIG_STEPPER_ADI_TMC51XX_LOG_STATUS)

void log_status(const uint8_t status_byte, const char *spi_status[])
{
	char buf[110];
	int m;

	m = snprintf(buf, sizeof(buf), "0x%02x", status_byte);
	if (m < 0) {
		return;
	}
	size_t n = m;

	for (uint8_t i = 0; i < TMC5XXX_SPI_STATUS_BITS; ++i) {
		if (status_byte & BIT(i)) {
			m = snprintf(buf + n, sizeof(buf) - n, " %s", spi_status[i]);
			if (m < 0) {
				return;
			}
			n += m;
			if (n >= sizeof(buf)) {
				buf[sizeof(buf) - 1] = '\0';
				break;
			}
		}
	}
	LOG_DBG("%s", buf);
}

#endif
