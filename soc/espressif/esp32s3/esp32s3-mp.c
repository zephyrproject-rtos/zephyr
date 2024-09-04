/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <soc.h>
#include <esp_cpu.h>
#include "esp_rom_uart.h"

#include "esp_mcuboot_image.h"
#include "esp_memory_utils.h"

#ifdef CONFIG_SOC_ENABLE_APPCPU

#include "bootloader_flash_priv.h"

#define sys_mmap   bootloader_mmap
#define sys_munmap bootloader_munmap

void esp_appcpu_start(void *entry_point)
{
	esp_cpu_unstall(1);

	if (!REG_GET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_CLKGATE_EN)) {
		REG_SET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_CLKGATE_EN);
		REG_CLR_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RUNSTALL);
		REG_SET_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RESETTING);
		REG_CLR_BIT(SYSTEM_CORE_1_CONTROL_0_REG, SYSTEM_CONTROL_CORE_1_RESETTING);
	}

	esp_rom_ets_set_appcpu_boot_addr((void *)entry_point);

	esp_cpu_reset(1);
}

static int load_segment(uint32_t src_addr, uint32_t src_len, uint32_t dst_addr)
{
	const uint32_t *data = (const uint32_t *)sys_mmap(src_addr, src_len);

	if (!data) {
		ets_printf("%s: mmap failed", __func__);
		return -1;
	}

	volatile uint32_t *dst = (volatile uint32_t *)dst_addr;

	for (int i = 0; i < src_len / 4; i++) {
		dst[i] = data[i];
	}

	sys_munmap(data);

	return 0;
}

int IRAM_ATTR esp_appcpu_image_load(unsigned int hdr_offset, unsigned int *entry_addr)
{
	const uint32_t img_off = FIXED_PARTITION_OFFSET(slot0_appcpu_partition);
	const uint32_t fa_size = FIXED_PARTITION_SIZE(slot0_appcpu_partition);
	const uint8_t fa_id = FIXED_PARTITION_ID(slot0_appcpu_partition);
	int rc = 0;

	if (entry_addr == NULL) {
		ets_printf("Can't return the entry address. Aborting!\n");
		abort();
		return -1;
	}

	ets_printf("Loading appcpu image, area id: %d, offset: 0x%x, hdr.off: 0x%x, size: %d kB\n",
		   fa_id, img_off, hdr_offset, fa_size / 1024);

	uint32_t mcuboot_header[8] = {0};
	esp_image_load_header_t image_header = {0};

	const uint32_t *data = (const uint32_t *)sys_mmap(img_off, 0x40);

	memcpy((void *)&mcuboot_header, data, sizeof(mcuboot_header));
	memcpy((void *)&image_header, data + (hdr_offset / sizeof(uint32_t)),
	       sizeof(esp_image_load_header_t));

	sys_munmap(data);

	if (image_header.header_magic == ESP_LOAD_HEADER_MAGIC) {
		ets_printf("MCUboot image format\n");
	} else if ((image_header.header_magic & 0xff) == 0xE9) {
		ets_printf("ESP image format is not supported\n");
		abort();
	} else {
		ets_printf("Unknown or empty image detected. Aborting!\n");
		abort();
	}

	if (!esp_ptr_in_iram((void *)image_header.iram_dest_addr) ||
	    !esp_ptr_in_iram((void *)(image_header.iram_dest_addr + image_header.iram_size))) {
		ets_printf("IRAM region in load header is not valid. Aborting");
		abort();
	}

	if (!esp_ptr_in_dram((void *)image_header.dram_dest_addr) ||
	    !esp_ptr_in_dram((void *)(image_header.dram_dest_addr + image_header.dram_size))) {
		ets_printf("DRAM region in load header is not valid. Aborting");
		abort();
	}

	if (!esp_ptr_in_iram((void *)image_header.entry_addr)) {
		ets_printf("Application entry point (%xh) is not in IRAM. Aborting",
			   image_header.entry_addr);
		abort();
	}

	ets_printf("IRAM segment: paddr=%08xh, vaddr=%08xh, size=%05xh (%6d) load\n",
		   (img_off + image_header.iram_flash_offset), image_header.iram_dest_addr,
		   image_header.iram_size, image_header.iram_size);

	load_segment(img_off + image_header.iram_flash_offset, image_header.iram_size,
		     image_header.iram_dest_addr);

	ets_printf("DRAM segment: paddr=%08xh, vaddr=%08xh, size=%05xh (%6d) load\n",
		   (img_off + image_header.dram_flash_offset), image_header.dram_dest_addr,
		   image_header.dram_size, image_header.dram_size);

	load_segment(img_off + image_header.dram_flash_offset, image_header.dram_size,
		     image_header.dram_dest_addr);

	ets_printf("Application start=%xh\n", image_header.entry_addr);
	esp_rom_uart_tx_wait_idle(0);

	assert(entry_addr != NULL);
	*entry_addr = image_header.entry_addr;

	return rc;
}

void esp_appcpu_image_stop(void)
{
	esp_cpu_stall(1);
}

void esp_appcpu_image_start(unsigned int hdr_offset)
{
	static int started;
	unsigned int entry_addr = 0;

	if (started) {
		printk("APPCPU already started.\r\n");
		return;
	}

	/* Input image meta header, output appcpu entry point */
	esp_appcpu_image_load(hdr_offset, &entry_addr);

	esp_appcpu_start((void *)entry_addr);
}

int esp_appcpu_init(void)
{
	/* Load APPCPU image using image header offset
	 * (skipping the MCUBoot header)
	 */
	esp_appcpu_image_start(0x20);

	return 0;
}
#endif /* CONFIG_SOC_ENABLE_APPCPU */
