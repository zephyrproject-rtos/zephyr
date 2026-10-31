/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BCM2712 MSI interrupt proxy (MIP) driver.
 *
 * The MIP turns each endpoint MSI write into a GIC SPI. The RP1 driver
 * consumes this driver through the generic MSI parent API, so the RP1
 * driver itself has no BCM2712 or GIC dependency.
 */

#define DT_DRV_COMPAT brcm_bcm2712_mip

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree/map.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/types.h>

#include "msi_proxy.h"

LOG_MODULE_REGISTER(bcm2712_mip, CONFIG_INTC_LOG_LEVEL);

#define MIP_INT_CFGL_HOST  0x20U
#define MIP_INT_CFGH_HOST  0x30U
#define MIP_INT_MASKL_HOST 0x40U
#define MIP_INT_MASKH_HOST 0x50U
#define MIP_INT_MASKL_VPU  0x60U
#define MIP_INT_MASKH_VPU  0x70U

#define MIP_FIRST_CHANNEL(node_id) DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, interrupt_map, 0, 0)

#define MIP_CHANNEL_CHECK(node_id, prop, i)                                                        \
	BUILD_ASSERT(DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0) <                    \
			     DT_PROP(node_id, brcm_msi_num_spis),                                  \
		     "MIP channel is out of range");                                               \
	BUILD_ASSERT(DT_MAP_ENTRY_CHILD_SPECIFIER_BY_IDX(node_id, prop, i, 0) ==                   \
			     MIP_FIRST_CHANNEL(node_id) + (i),                                     \
		     "MIP interrupt-map channels must be consecutive");

struct mip_config {
	uintptr_t base;
	size_t size;
	uint32_t first_channel;
	uint32_t num_channels;
	uint64_t pci_addr;
};

struct mip_data {
	mm_reg_t base;
	struct k_spinlock lock;
};

static int mip_init(const struct device *dev)
{
	const struct mip_config *config = dev->config;
	struct mip_data *data = dev->data;

	device_map(&data->base, config->base, config->size, K_MEM_CACHE_NONE);

	/* Mask the host path until a vector is enabled. */
	sys_write32(0xffffffffU, data->base + MIP_INT_MASKL_HOST);
	sys_write32(0xffffffffU, data->base + MIP_INT_MASKH_HOST);

	/* The VPU is not used by Zephyr. */
	sys_write32(0xffffffffU, data->base + MIP_INT_MASKL_VPU);
	sys_write32(0xffffffffU, data->base + MIP_INT_MASKH_VPU);

	/* Interrupts are edge-triggered at the host interrupt controller. */
	sys_write32(0xffffffffU, data->base + MIP_INT_CFGL_HOST);
	sys_write32(0xffffffffU, data->base + MIP_INT_CFGH_HOST);

	return 0;
}

static bool mip_channel_valid(const struct mip_config *config, unsigned int channel)
{
	return channel >= config->first_channel &&
	       channel - config->first_channel < config->num_channels;
}

static int mip_compose(const struct device *dev, unsigned int channel,
		       struct msi_proxy_message *message)
{
	const struct mip_config *config = dev->config;

	if (!mip_channel_valid(config, channel)) {
		return -EINVAL;
	}

	message->address = config->pci_addr;
	message->data = channel;

	return 0;
}

static int mip_enable(const struct device *dev, unsigned int channel)
{
	const struct mip_config *config = dev->config;
	struct mip_data *data = dev->data;
	uint32_t reg;
	uint32_t mask;
	k_spinlock_key_t key;

	if (!mip_channel_valid(config, channel)) {
		return -EINVAL;
	}

	reg = (channel < 32U) ? MIP_INT_MASKL_HOST : MIP_INT_MASKH_HOST;
	mask = BIT(channel & 31U);

	key = k_spin_lock(&data->lock);
	sys_write32(sys_read32(data->base + reg) & ~mask, data->base + reg);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static void mip_disable(const struct device *dev, unsigned int channel)
{
	const struct mip_config *config = dev->config;
	struct mip_data *data = dev->data;
	uint32_t reg;
	uint32_t mask;
	k_spinlock_key_t key;

	if (!mip_channel_valid(config, channel)) {
		return;
	}

	reg = (channel < 32U) ? MIP_INT_MASKL_HOST : MIP_INT_MASKH_HOST;
	mask = BIT(channel & 31U);

	key = k_spin_lock(&data->lock);
	sys_write32(sys_read32(data->base + reg) | mask, data->base + reg);
	k_spin_unlock(&data->lock, key);
}

static DEVICE_API(msi_proxy, mip_api) = {
	.compose = mip_compose,
	.enable = mip_enable,
	.disable = mip_disable,
};

#define MIP_INIT(n)                                                                                \
	BUILD_ASSERT(DT_MAP_LEN(DT_DRV_INST(n), interrupt_map) > 0, "MIP interrupt-map is empty"); \
	DT_FOREACH_MAP_ENTRY(DT_DRV_INST(n), interrupt_map, MIP_CHANNEL_CHECK)                     \
	static struct mip_data mip_data_##n;                                                       \
	static const struct mip_config mip_config_##n = {                                          \
		.base = DT_INST_REG_ADDR_BY_IDX(n, 0),                                             \
		.size = DT_INST_REG_SIZE_BY_IDX(n, 0),                                             \
		.first_channel = MIP_FIRST_CHANNEL(DT_DRV_INST(n)),                                \
		.num_channels = DT_MAP_LEN(DT_DRV_INST(n), interrupt_map),                         \
		.pci_addr = ((uint64_t)DT_INST_PROP_BY_IDX(n, brcm_msi_pci_addr, 0) << 32) |       \
			    DT_INST_PROP_BY_IDX(n, brcm_msi_pci_addr, 1),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, mip_init, NULL, &mip_data_##n, &mip_config_##n, PRE_KERNEL_1,     \
			      CONFIG_BCM2712_MIP_INIT_PRIORITY, &mip_api);

DT_INST_FOREACH_STATUS_OKAY(MIP_INIT)
