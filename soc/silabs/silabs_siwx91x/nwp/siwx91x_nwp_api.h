/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_NWP_API_H_
#define SIWX91X_NWP_API_H_
#include <stdint.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/wifi.h>
#include "device/silabs/si91x/wireless/inc/sl_wifi_device.h"
#include "protocol/wifi/inc/sl_wifi_constants.h"
#include "protocol/wifi/inc/sl_wifi_types.h"
#include "sli_wifi/inc/sli_wifi_types.h"

struct device;

/* sl_wifi_firmware_version_t is buggy because it lacks of __packed */
struct siwx91x_nwp_version {
	uint8_t chip_id;
	uint8_t rom_id;
	uint8_t major;
	uint8_t minor;
	uint8_t security_version;
	uint8_t patch_num;
	uint8_t customer_id;
	uint16_t build_num;
} __packed;

/* NWP APIs */
void siwx91x_nwp_soft_reset(const struct device *dev);
void siwx91x_nwp_force_assert(const struct device *dev);
void siwx91x_nwp_opermode(const struct device *dev,
			  sl_wifi_system_boot_configuration_t *params);
void siwx91x_nwp_feature(const struct device *dev, bool enable_pll);
void siwx91x_nwp_dynamic_pool(const struct device *dev, int tx, int rx, int global);
void siwx91x_nwp_get_firmware_version(const struct device *dev,
				      struct siwx91x_nwp_version *version);
int siwx91x_nwp_flash_erase(const struct device *dev, uint32_t dest, size_t len);
int siwx91x_nwp_flash_write(const struct device *dev, uint32_t dest, const void *buf, size_t len);
int siwx91x_nwp_fw_upgrade_start(const struct device *dev, const void *hdr);
int siwx91x_nwp_fw_upgrade_write(const struct device *dev, const void *buf, size_t len);

#endif
