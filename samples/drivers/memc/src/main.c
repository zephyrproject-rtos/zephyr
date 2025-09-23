/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/pm/device_runtime.h>

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_imx_flexspi)
/* Use memc API to get AHB base address for the device */
#include "memc_mcux_flexspi.h"
#define FLEXSPI_DEV DEVICE_DT_GET(DT_PARENT(DT_ALIAS(sram_ext)))
#define MEMC_DEV DT_ALIAS(sram_ext)
#define MEMC_PORT DT_REG_ADDR(DT_ALIAS(sram_ext))
#define MEMC_BASE ((void *)memc_flexspi_get_ahb_address(FLEXSPI_DEV, MEMC_PORT, 0))
#define MEMC_SIZE (DT_PROP(DT_ALIAS(sram_ext), size) / 8)
#elif DT_HAS_COMPAT_STATUS_OKAY(renesas_smartbond_nor_psram)
#include <da1469x_config.h>
#define MEMC_DEV DT_ALIAS(sram_ext)
#define MEMC_BASE ((void *)MCU_QSPIR_M_BASE)
#define MEMC_SIZE (DT_PROP(DT_ALIAS(sram_ext), dev_size) / 8)
#elif DT_HAS_COMPAT_STATUS_OKAY(ambiq_mspi_controller)
#define MEMC_DEV DT_ALIAS(psram0)
#define MSPI_BUS DT_BUS(MEMC_DEV)
#define mspi_get_xip_address(controller) DT_REG_ADDR_BY_IDX(controller, 1)
#define MEMC_BASE (void *)(mspi_get_xip_address(MSPI_BUS))
#define MEMC_SIZE (DT_PROP(MEMC_DEV, size) / 8)
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_fmc_sdram)
#define MEMC_DEV  DT_ALIAS(sram_ext)
#define MEMC_BASE DT_REG_ADDR(MEMC_DEV)
#define MEMC_SIZE DT_REG_SIZE(MEMC_DEV)
#else
#error At least one driver should be selected!
#endif

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
	uint8_t *memc = (uint8_t *)MEMC_BASE;
	uint32_t i, j;

	/* Initialize write buffer */
	for (i = 0; i < BUF_SIZE; i++) {
		memc_write_buffer[i] = (uint8_t)i;
	}

	pm_device_runtime_get(DEVICE_DT_GET(MEMC_DEV));

	printk("Writing to memory region with base %p, size 0x%0x\n\n",
		memc, MEMC_SIZE);
	/* Copy write buffer into memc region */
	for (i = 0, j = 0; j < (MEMC_SIZE / BUF_SIZE); i += BUF_SIZE, j++) {
		memcpy(memc + i, memc_write_buffer, BUF_SIZE);
	}
	/* Copy any remaining space bytewise */
	for (; i < MEMC_SIZE; i++) {
		memc[i] = memc_write_buffer[i];
	}
	/* Read from memc region into buffer */
	for (i = 0, j = 0; j < (MEMC_SIZE / BUF_SIZE); i += BUF_SIZE, j++) {
		memcpy(memc_read_buffer, memc + i, BUF_SIZE);
		/* Compare memory */
		if (memcmp(memc_read_buffer, memc_write_buffer, BUF_SIZE)) {
			printk("Error: read data differs in range [0x%x- 0x%x]\n",
				i, i + (BUF_SIZE - 1));
			dump_memory(memc_write_buffer, BUF_SIZE);
			dump_memory(memc_read_buffer, BUF_SIZE);
			return 0;
		}
		printk("Check (%i/%i) passed!\n", j, (MEMC_SIZE / BUF_SIZE) - 1);
	}
	/* Copy any remaining space bytewise */
	for (; i < MEMC_SIZE; i++) {
		memc_read_buffer[i] = memc[i];
		if (memc_write_buffer[i] != memc_read_buffer[i]) {
			printk("Error: read data differs at offset 0x%x\n", i);
			return 0;
		}
	}
	printk("First 1KB of Data in memory:\n");
	printk("===========================\n");
	dump_memory(memc, MIN(MEMC_SIZE, KB(1)));
	pm_device_runtime_put(DEVICE_DT_GET(MEMC_DEV));
	printk("Read data matches written data\n");
	return 0;
}
