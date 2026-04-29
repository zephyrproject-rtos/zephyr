//
// Created by Kirill Shypachov on 22.02.2026.
//

#ifndef ZEPHYR_DRIVERS_PSRAM_H
#define ZEPHYR_DRIVERS_PSRAM_H

#include <stdint.h>
// Manufacturer IDs
#define ISSI_ID			0xD5	/* ISSI Integrated Silicon Solutions, see also PMC. */
#define ISSI_ID_SPI		0x9D	/* ISSI ID used for SPI flash, see also PMC_ID_NOPREFIX */
#define AP_MEMORY_ID            0x0d
// Device IDs
#define ISSI_PSRAM_IS66WVS4M8BLL	0x5d40	/* IS66WVS4M8BLL */
#define AP_MEMORY_ESP_PSRAM64H          0x5d53  /* ESP-PSRAM64, */

#ifdef __cplusplus
extern "C" {
#endif

const char *psram_get_manufacturer_name(uint8_t manufacturer_id);

const char *psram_get_device_name(uint8_t manufacturer_id, uint16_t device_id);

#ifdef __cplusplus
}
#endif


#endif //ZEPHYR_DRIVERS_PSRAM_H