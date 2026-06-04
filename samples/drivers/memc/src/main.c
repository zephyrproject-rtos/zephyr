/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/memc.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_imx_flexspi)
/* Use memc API to get AHB base address for the device */
#include "memc_mcux_flexspi.h"
#define FLEXSPI_DEV DEVICE_DT_GET(DT_PARENT(DT_ALIAS(sram_ext)))
#define MEMC_DEV DT_ALIAS(sram_ext)
#define MEMC_PORT DT_REG_ADDR(DT_ALIAS(sram_ext))
#define MEMC_BASE ((void *)memc_flexspi_get_ahb_address(FLEXSPI_DEV, MEMC_PORT, 0))
#define MEMC_SIZE (DT_PROP(DT_ALIAS(sram_ext), size) / 8)
#define MEMC_MEMMAP 1
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_smartbond_nor_psram)
#include <da1469x_config.h>
#define MEMC_DEV DT_ALIAS(sram_ext)
#define MEMC_BASE ((void *)MCU_QSPIR_M_BASE)
#define MEMC_SIZE (DT_PROP(DT_ALIAS(sram_ext), dev_size) / 8)
#define MEMC_MEMMAP 1
#elif DT_HAS_COMPAT_STATUS_OKAY(ambiq_mspi_controller)
#define MEMC_DEV DT_ALIAS(psram0)
#define MSPI_BUS DT_BUS(MEMC_DEV)
#define mspi_get_xip_address(controller) DT_REG_ADDR_BY_IDX(controller, 1)
#define MEMC_BASE (void *)(mspi_get_xip_address(MSPI_BUS))
#define MEMC_SIZE (DT_PROP(MEMC_DEV, size) / 8)
#define MEMC_MEMMAP 1
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_fmc_sdram) || \
	DT_HAS_COMPAT_STATUS_OKAY(st_stm32_xspi_psram)
#define MEMC_DEV  DT_ALIAS(sram_ext)
#define MEMC_BASE DT_REG_ADDR(MEMC_DEV)
#define MEMC_SIZE DT_REG_SIZE(MEMC_DEV)
#define MEMC_MEMMAP 1
#elif DT_HAS_COMPAT_STATUS_OKAY(nxp_xspi_psram)
#define MEMC_DEV DT_ALIAS(sram_ext)
#define MSPI_BUS DT_BUS(MEMC_DEV)
#define MEMC_BASE DT_REG_ADDR_BY_IDX(MSPI_BUS, 1)
#define MEMC_SIZE (DT_PROP(MEMC_DEV, size) / 8)
#define MEMC_MEMMAP 1
#elif DT_HAS_COMPAT_STATUS_OKAY(silabs_siwx91x_qspi_memory)
#define MEMC_DEV  DT_ALIAS(sram_ext)
#define MEMC_BASE ((void *)DT_REG_ADDR(MEMC_DEV))
#define MEMC_SIZE (DT_REG_SIZE(MEMC_DEV) / 8)
#define MEMC_MEMMAP 1
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_xspi_controller)
#define MEMC_DEV DT_ALIAS(psram0)
#define MSPI_BUS DT_BUS(MEMC_DEV)
#define MEMC_BASE ((void *)DT_REG_ADDR_BY_IDX(MSPI_BUS, 1))
#define MEMC_SIZE (DT_PROP(MEMC_DEV, size) / 8)
#define MEMC_MEMMAP 1
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_ospi_controller)
#define MEMC_DEV DT_ALIAS(psram0)
#define MSPI_BUS DT_BUS(MEMC_DEV)
#define MEMC_BASE ((void *)DT_REG_ADDR_BY_IDX(MSPI_BUS, 1))
#define MEMC_SIZE (DT_PROP(MEMC_DEV, size) / 8)
#define MEMC_MEMMAP 1
#elif DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_mspi)
#define MEMC_DEV DT_ALIAS(psram0)
#define MEMC_SIZE (DT_PROP(MEMC_DEV, size) / 8)
#define MEMC_MEMMAP 0
#else
#error At least one driver should be selected!
#endif

static inline int sample_write(const struct device *dev, uint32_t offset,
			       const uint8_t *buf, size_t len)
{
#if MEMC_MEMMAP
	ARG_UNUSED(dev);
	memcpy((uint8_t *)MEMC_BASE + offset, buf, len);
	return 0;
#else
	return memc_write(dev, offset, buf, len);
#endif
}

static inline int sample_read(const struct device *dev, uint32_t offset,
			      uint8_t *buf, size_t len)
{
#if MEMC_MEMMAP
	ARG_UNUSED(dev);
	memcpy(buf, (const uint8_t *)MEMC_BASE + offset, len);
	return 0;
#else
	return memc_read(dev, offset, buf, len);
#endif
}

void dump_memory(uint8_t *p, uint32_t size)
{
	uint32_t i, j;

	for (i = 0, j = 0; j < (size / 16); i += 16, j++) {
		printk("%02x %02x %02x %02x %02x %02x %02x %02x ",
			p[i], p[i+1], p[i+2], p[i+3],
			p[i+4], p[i+5], p[i+6], p[i+7]);
		printk("%02x %02x %02x %02x %02x %02x %02x %02x\n",
			p[i+8], p[i+9], p[i+10], p[i+11],
			p[i+12], p[i+13], p[i+14], p[i+15]);
		/* Split dump at 256B boundaries */
		if (((i + 16) & 0xFF) == 0) {
			printk("\n");
		}
	}
	/* Dump any remaining data after 16 byte blocks */
	for (; i < size; i++) {
		printk("%02x ", p[i]);
	}
	printk("\n");
}

#define BUF_SIZE 1024

uint8_t memc_write_buffer[BUF_SIZE];
uint8_t memc_read_buffer[BUF_SIZE];

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(MEMC_DEV);
	uint32_t i, j;
	int rc;

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return -ENODEV;
	}

#if !MEMC_MEMMAP
	/* Non-XIP path requires the MEMC driver to expose read/write API */
	if (!DEVICE_API_IS(memc, dev)) {
		printk("Device does not implement the MEMC read/write API\n");
		return -ENOTSUP;
	}
#endif

	/* Initialize write buffer */
	for (i = 0; i < BUF_SIZE; i++) {
		memc_write_buffer[i] = (uint8_t)i;
	}

	pm_device_runtime_get(dev);

#if MEMC_MEMMAP
	printk("Writing to memory region with base %p, size 0x%0x\n\n",
		(void *)MEMC_BASE, MEMC_SIZE);
#else
	printk("Writing to memory via memc API, size 0x%0x\n\n",
		MEMC_SIZE);
#endif

	/* Write pattern to memory in BUF_SIZE chunks */
	for (i = 0, j = 0; j < (MEMC_SIZE / BUF_SIZE); i += BUF_SIZE, j++) {
		rc = sample_write(dev, i, memc_write_buffer, BUF_SIZE);
		if (rc) {
			printk("Write failed at offset 0x%x: %d\n", i, rc);
			return rc;
		}
	}
	/* Write any remaining bytes in one call */
	if (i < MEMC_SIZE) {
		rc = sample_write(dev, i, memc_write_buffer, MEMC_SIZE - i);
		if (rc) {
			printk("Write failed at offset 0x%x: %d\n", i, rc);
			return rc;
		}
	}

	/* Read back and verify in BUF_SIZE chunks */
	for (i = 0, j = 0; j < (MEMC_SIZE / BUF_SIZE); i += BUF_SIZE, j++) {
		rc = sample_read(dev, i, memc_read_buffer, BUF_SIZE);
		if (rc) {
			printk("Read failed at offset 0x%x: %d\n", i, rc);
			return rc;
		}
		if (memcmp(memc_read_buffer, memc_write_buffer, BUF_SIZE)) {
			printk("Error: read data differs in range [0x%x- 0x%x]\n",
				i, i + (BUF_SIZE - 1));
			dump_memory(memc_write_buffer, BUF_SIZE);
			dump_memory(memc_read_buffer, BUF_SIZE);
			return 0;
		}
		LOG_INF("Check (%i/%i) passed!", j, (MEMC_SIZE / BUF_SIZE) - 1);
	}
	/* Verify any remaining bytes in one call */
	if (i < MEMC_SIZE) {
		uint32_t remain = MEMC_SIZE - i;

		rc = sample_read(dev, i, memc_read_buffer, remain);
		if (rc) {
			printk("Read failed at offset 0x%x: %d\n", i, rc);
			return rc;
		}
		if (memcmp(memc_read_buffer, memc_write_buffer, remain)) {
			printk("Error: read data differs at tail offset 0x%x\n", i);
			return 0;
		}
	}

	printk("First 1KB of Data in memory:\n");
	printk("===========================\n");
	sample_read(dev, 0, memc_read_buffer, KB(1));
	dump_memory(memc_read_buffer, KB(1));

	pm_device_runtime_put(dev);
	printk("Read data matches written data\n");
	return 0;
}
