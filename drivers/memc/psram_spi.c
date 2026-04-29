//
// Created by Kirill Shypachov on 22.02.2026.
//

#include "psram_spi.h"

#include <stdint.h>

const char *psram_get_manufacturer_name(uint8_t manufacturer_id)
{
	switch (manufacturer_id) {

	case ISSI_ID_SPI:
	case ISSI_ID:
		return "ISSI (Integrated Silicon Solution Inc.)";
	case AP_MEMORY_ID:
		return "AP Memory";

	default:
		return "UNKNOWN";
	}
}

const char *psram_get_device_name(uint8_t manufacturer_id, uint16_t device_id)
{
	switch (manufacturer_id) {

	case ISSI_ID_SPI:
	case ISSI_ID:

		switch (device_id) {

	case ISSI_PSRAM_IS66WVS4M8BLL:
			return "IS66WVS4M8BLL (4MB Octal PSRAM)";

	default:
			return "UNKNOWN ISSI PSRAM";
		}

	case AP_MEMORY_ID:
		switch (device_id) {

	case AP_MEMORY_ESP_PSRAM64H:
			return "ESP-PSRAM64H (8MB Octal PSRAM)";

	default:
			return "UNKNOWN AP Memory PSRAM";
		}

	default:
		return "UNKNOWN DEVICE";
	}
}
