/*
 * Copyright (c) 2026 Maximilian Zimmermann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FIRMWARE_QEMU_FWCFG_QEMU_FWCFG_H_
#define ZEPHYR_DRIVERS_FIRMWARE_QEMU_FWCFG_QEMU_FWCFG_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/firmware/qemu_fwcfg/qemu_fwcfg.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

/* DMA result is normally available instantly, but just in case polling is
 * supported.
 */
#define FWCFG_DMA_POLL_WAIT_US  1
#define FWCFG_DMA_POLL_MAX_ITER 1024

struct fwcfg_api {
	int (*select)(const struct device *dev, uint16_t key);
	int (*read)(const struct device *dev, void *dst, size_t len);
	int (*dma_kick)(const struct device *dev, uint64_t descriptor_addr);
};

struct fwcfg_dma_access {
	uint32_t control;
	uint32_t length;
	uint64_t address;
} __packed __aligned(8);

enum fwcfg_transport {
	FWCFG_MMIO,
	FWCFG_IOPORT,
};

struct fwcfg_config {
	DEVICE_MMIO_ROM;
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
	struct k_mutex lock;
	struct fwcfg_dma_access dma_desc;
	uint8_t dma_bounce[CONFIG_QEMU_FWCFG_DMA_BOUNCE_SIZE];
	uint32_t features;
};

static inline const struct fwcfg_api *fwcfg_get_api(const struct device *dev)
{
	return (const struct fwcfg_api *)dev->api;
}

static inline int fwcfg_select(const struct device *dev, uint16_t key)
{
	const struct fwcfg_api *api = fwcfg_get_api(dev);

	__ASSERT(api != NULL, "fwcfg api must be present");
	__ASSERT(api->select != NULL, "fwcfg select callback must be present");

	return api->select(dev, key);
}

static inline int fwcfg_read_selected(const struct device *dev, void *dst, size_t len)
{
	const struct fwcfg_api *api = fwcfg_get_api(dev);

	__ASSERT(api != NULL, "fwcfg api must be present");
	__ASSERT(api->read != NULL, "fwcfg read callback must be present");

	return api->read(dev, dst, len);
}

static inline int fwcfg_dma_kick(const struct device *dev, uint64_t descriptor_addr)
{
	const struct fwcfg_api *api = fwcfg_get_api(dev);

	__ASSERT(api != NULL, "fwcfg api must be present");
	__ASSERT(api->dma_kick != NULL, "fwcfg dma_kick callback must be present");

	return api->dma_kick(dev, descriptor_addr);
}

int fwcfg_init_common(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_FIRMWARE_QEMU_FWCFG_QEMU_FWCFG_H_ */
