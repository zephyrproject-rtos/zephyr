/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ESPRESSIF_ESP32P4_CACHE_H_
#define ZEPHYR_SOC_ESPRESSIF_ESP32P4_CACHE_H_

/* PMA entry used to describe the non-cacheable window. Entries 0-2 and 15 are
 * used by the ROM's startup PMA map on ESP32-P4, so a mid-range entry is
 * chosen to avoid disturbing it.
 */
#define NOCACHE_PMA_ENTRY 8

/*
 * Apply the PMA entry that marks the reserved nocache window non-cacheable.
 * Called at boot and re-applied on wake from standby, since the PMA CSRs are
 * volatile across CPU power-down sleep.
 */
void nocache_region_init(void);

#endif /* ZEPHYR_SOC_ESPRESSIF_ESP32P4_CACHE_H_ */
