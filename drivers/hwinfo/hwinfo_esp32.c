/*
 * Copyright (c) 2019 Leandro A. F. Pereira
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc/efuse_reg.h>
#include <soc/reset_reasons.h>
#include "esp_system.h"
#include "rtc.h"

#include <zephyr/drivers/hwinfo.h>
#include <string.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
#if !defined(CONFIG_SOC_ESP32) && !defined(CONFIG_SOC_ESP32_NET)
	uint32_t rdata1 = sys_read32(EFUSE_RD_MAC_SPI_SYS_0_REG);
	uint32_t rdata2 = sys_read32(EFUSE_RD_MAC_SPI_SYS_1_REG);
#else
	uint32_t rdata1 = sys_read32(EFUSE_BLK0_RDATA1_REG);
	uint32_t rdata2 = sys_read32(EFUSE_BLK0_RDATA2_REG);
#endif
	uint8_t id[6];

	/* The first word provides the lower 32 bits of the MAC
	 * address; the low 16 bits of the second word provide the
	 * upper 16 bytes of the MAC address.  The upper 16 bits are
	 * (apparently) a checksum, and reserved.  See ESP32 Technical
	 * Reference Manual V4.1 section 20.5.
	 */
	id[0] = (uint8_t)(rdata2 >> 8);
	id[1] = (uint8_t)(rdata2 >> 0);
	id[2] = (uint8_t)(rdata1 >> 24);
	id[3] = (uint8_t)(rdata1 >> 16);
	id[4] = (uint8_t)(rdata1 >> 8);
	id[5] = (uint8_t)(rdata1 >> 0);

	if (length > sizeof(id)) {
		length = sizeof(id);
	}
	memcpy(buffer, id, length);

	return length;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_POR
		      | RESET_PIN
		      | RESET_SOFTWARE
		      | RESET_WATCHDOG
		      | RESET_LOW_POWER_WAKE
		      | RESET_CPU_LOCKUP
		      | RESET_BROWNOUT);

	return 0;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t reason = esp_reset_reason();

	switch (reason) {
	case ESP_RST_POWERON:
		*cause = RESET_POR;
		break;
	case ESP_RST_EXT:
		*cause = RESET_PIN;
		break;
	case ESP_RST_SW:
		*cause = RESET_SOFTWARE;
		break;
	case ESP_RST_INT_WDT:
	case ESP_RST_TASK_WDT:
	case ESP_RST_WDT:
		*cause = RESET_WATCHDOG;
		break;
	case ESP_RST_DEEPSLEEP:
		*cause = RESET_LOW_POWER_WAKE;
		break;
	case ESP_RST_PANIC:
		*cause = RESET_CPU_LOCKUP;
	case ESP_RST_BROWNOUT:
		*cause = RESET_BROWNOUT;
		break;
	default:
		*cause = 0;
		break;
	}

	return 0;
}
