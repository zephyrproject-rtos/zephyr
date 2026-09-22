/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aesc_spi_flash_controller

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(flash_aesc, CONFIG_FLASH_LOG_LEVEL);

#define SOC_NV_FLASH_NODE	DT_INST(0, soc_nv_flash)
#define FLASH_BASE		DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define FLASH_SIZE		DT_REG_SIZE(SOC_NV_FLASH_NODE)

/*
 * XIP read-mode configuration lives on the controller's XIP config bus, which
 * sits 0x1000 into the controller register block:
 *   +0x08  CONFIGURE  write strobe that latches the MODE register and, for
 *                     quad, writes EVCR into the flash device
 *   +0x0c  MODE       [3:0] read mode, [15:8] dummy cycles, [23:16] flash EVCR
 *
 * Quad needs all three fields, not just the mode: the mode selects the quad
 * read command, the dummy-cycle count must match the flash's quad fast-read
 * latency, and EVCR is programmed into the flash to switch the device itself
 * into quad. (mode=0x2 alone would read with zero dummy cycles and write
 * EVCR=0x00 to the flash, returning garbage.)
 */
#define XIP_CFG_OFFSET		0x1000
#define XIP_REG_CONFIGURE	(XIP_CFG_OFFSET + 0x08)
#define XIP_REG_MODE		(XIP_CFG_OFFSET + 0x0c)

#define XIP_MODE_MASK		0xf	/* mode field is modeWidth = 4 bits */
#define XIP_MODE_SINGLE		0x0
#define XIP_MODE_DUAL		0x1
#define XIP_MODE_QUAD		0x2
#define XIP_DUMMY_QUAD		7
#define XIP_EVCR_QUAD		0x7f
#define XIP_CONFIG_QUAD		(XIP_MODE_QUAD | (XIP_DUMMY_QUAD << 8) | \
				 (XIP_EVCR_QUAD << 16))

static const struct flash_parameters flash_aesc_parameters = {
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_value = 0xff,
};

struct flash_aesc_config {
	DEVICE_MMIO_ROM;
	bool quad_mode;
};

struct flash_aesc_data {
	DEVICE_MMIO_RAM;
};

static int flash_aesc_read(const struct device *dev, off_t offset, void *data,
			   size_t len)
{
	ARG_UNUSED(dev);

	if (offset < 0 || offset >= FLASH_SIZE || (FLASH_SIZE - offset) < len) {
		return -EINVAL;
	}

	if (len == 0U) {
		return 0;
	}

	/* The flash is execute-in-place mapped; read straight from the window. */
	memcpy(data, (uint8_t *)FLASH_BASE + offset, len);

	return 0;
}

static int flash_aesc_write(const struct device *dev, off_t offset,
			    const void *data, size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(offset);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	/* Programming over the XIP controller is not supported yet. */
	return -ENOSYS;
}

static int flash_aesc_erase(const struct device *dev, off_t offset, size_t size)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(offset);
	ARG_UNUSED(size);

	return -ENOSYS;
}

static const struct flash_parameters *flash_aesc_get_parameters(
				const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_aesc_parameters;
}

static int flash_aesc_init(const struct device *dev)
{
	const struct flash_aesc_config *cfg = dev->config;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (cfg->quad_mode) {
		uintptr_t base = DEVICE_MMIO_GET(dev);
		uint32_t mode = sys_read32(base + XIP_REG_MODE) & XIP_MODE_MASK;

		if (mode == XIP_MODE_SINGLE || mode == XIP_MODE_DUAL) {
			/* Latch the quad read settings, then strobe configure. */
			sys_write32(XIP_CONFIG_QUAD, base + XIP_REG_MODE);
			sys_write32(0, base + XIP_REG_CONFIGURE);
		}
	}

	return 0;
}

static DEVICE_API(flash, flash_aesc_driver_api) = {
	.read = flash_aesc_read,
	.write = flash_aesc_write,
	.erase = flash_aesc_erase,
	.get_parameters = flash_aesc_get_parameters,
};

static struct flash_aesc_data flash_aesc_data_0;
static const struct flash_aesc_config flash_aesc_config_0 = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
	.quad_mode = DT_INST_PROP(0, aesc_quad_mode),
};

DEVICE_DT_INST_DEFINE(0, flash_aesc_init, NULL,
		      &flash_aesc_data_0, &flash_aesc_config_0, PRE_KERNEL_1,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_aesc_driver_api);
