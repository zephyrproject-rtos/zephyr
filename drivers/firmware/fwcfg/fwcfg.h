/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FIRMWARE_FWCFG_FWCFG_H
#define ZEPHYR_DRIVERS_FIRMWARE_FWCFG_FWCFG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#define FW_CFG_SIGNATURE 0x0000
#define FW_CFG_ID        0x0001
#define FW_CFG_FILE_DIR  0x0019

#define FW_CFG_ID_F_TRADITIONAL BIT(0)
#define FW_CFG_ID_F_DMA         BIT(1)

/* DMA result is normally available instantly, but just in case polling is
 * supported.
 */
#ifndef FWCFG_DMA_POLL_WAIT_US
#define FWCFG_DMA_POLL_WAIT_US 5
#endif

#ifndef FWCFG_DMA_POLL_MAX_ITER
#define FWCFG_DMA_POLL_MAX_ITER 1024
#endif

struct fwcfg_ops {
	int (*select)(const struct device *dev, uint16_t key);
	int (*read)(const struct device *dev, uint8_t *dst, size_t len);
	int (*read_dma)(const struct device *dev, uint16_t key, uint8_t *dst, size_t len);
	int (*write_dma)(const struct device *dev, uint16_t key, const uint8_t *src, size_t len);
};

enum fwcfg_transport {
	FWCFG_MMIO,
	FWCFG_IOPORT,
};

struct fwcfg_config {
	DEVICE_MMIO_ROM;
	const struct fwcfg_ops *ops;
	enum fwcfg_transport transport;
	union {
		struct {
			uintptr_t base;
		} mmio;
		struct {
			uint16_t sel_port;
			uint16_t data_port;
		} io;
	} u;
};

struct fwcfg_data {
	DEVICE_MMIO_RAM;
	bool present;
	uint32_t features;
	bool dma_ok;
};

int fwcfg_probe(const struct device *dev);
int fwcfg_read_item(const struct device *dev, uint16_t key, void *buf, size_t len);
int fwcfg_write_item(const struct device *dev, uint16_t key, const void *buf, size_t len);
int fwcfg_get_features(const struct device *dev, uint32_t *features);
int fwcfg_find_file(const struct device *dev, const char *file, uint16_t *select);
bool fwcfg_dma_supported(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_FIRMWARE_FWCFG_FWCFG_H */
