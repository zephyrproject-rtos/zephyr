/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_FWCFG_H
#define ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_FWCFG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#define FW_CFG_SIGNATURE 0x0000
#define FW_CFG_ID        0x0001
#define FW_CFG_FILE_DIR  0x0019

#define FW_CFG_ID_F_TRADITIONAL BIT(0)
#define FW_CFG_ID_F_DMA         BIT(1)

int fwcfg_read_item(const struct device *dev, uint16_t key, void *buf, size_t len);
/* Writes are supported only through DMA transport. */
int fwcfg_write_item(const struct device *dev, uint16_t key, const void *buf, size_t len);
int fwcfg_get_features(const struct device *dev, uint32_t *features);
int fwcfg_find_file(const struct device *dev, const char *file, uint16_t *select);
bool fwcfg_dma_supported(const struct device *dev);

#endif
