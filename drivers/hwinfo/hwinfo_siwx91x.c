/*
 * Copyright (c) 2025 Noah Pendleton
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	/*
	 * The SiWX91x chips include an undocumented memory-mapped "efusecopy"
	 * region that contains the factory programmed BLE and WiFi MAC values. The
	 * offsets for these values were experimentally derived by using the SiLabs
	 * "Simplicity Commander" utility on an siwx917_rb4338a board, specifically
	 * these commands:

	 * Read device info:
	 * $ commander device info
	 *   Part Number    : SiWG917M111MGTBA
	 *   Product Rev    : B0
	 *   Flash Size     : 8192 kB
	 *   SRAM Size      : 672 kB
	 *   Unique ID      : 0000d448671c1504
	 *   DONE

	 * Dump the manufacturing data, which includes the "efusecopy" region:
	 * $ commander mfg917 dump data.zip
	 */
	uint8_t *wifi_mac = (uint8_t *)(0x040003E0 + 0x22);
	const ssize_t id_length = MIN(6, length);

	memcpy(buffer, wifi_mac, id_length);

	return id_length;
}
