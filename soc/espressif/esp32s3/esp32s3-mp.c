/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>

#include <soc.h>
#include <esp_log.h>
#include <esp_cpu.h>
#include <esp_rom_serial_output.h>

#include <esp_mcuboot_image.h>
#include <esp_memory_utils.h>
#include <hw_init.h>
#include <memory.h>

#define TAG "amp"

/* AMP support */
#ifdef CONFIG_SOC_ENABLE_APPCPU

#include <bootloader_flash_priv.h>

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
		ESP_EARLY_LOGE(TAG, "%s: mmap failed", __func__);
		return -EIO;
	}

	volatile uint32_t *dst = (volatile uint32_t *)dst_addr;

	for (int i = 0; i < src_len / 4; i++) {
		dst[i] = data[i];
	}

	sys_munmap(data);

	return 0;
}

/* A segment that is not fully inside its APPCPU region would overwrite the
 * PROCPU sections.
 */
static bool IRAM_ATTR segment_in_region(uint32_t addr, uint32_t size, uint32_t start, uint32_t end)
{
	return (addr >= start) && (addr <= end) && (size <= end - addr);
}

int IRAM_ATTR esp_appcpu_image_load(unsigned int hdr_offset, unsigned int *entry_addr)
{
	const uint32_t fa_offset = PARTITION_OFFSET(slot0_appcpu_partition);
	const uint32_t fa_size = PARTITION_SIZE(slot0_appcpu_partition);
	const uint8_t fa_id = PARTITION_ID(slot0_appcpu_partition);

	if (entry_addr == NULL) {
		ESP_EARLY_LOGE(TAG, "Can't return the entry address: APPCPU not started");
		return -EINVAL;
	}

	uint32_t mcuboot_header[8] = {0};
	esp_image_load_header_t image_header = {0};

	const uint32_t *data = (const uint32_t *)sys_mmap(fa_offset, 0x80);

	if (data == NULL) {
		ESP_EARLY_LOGE(TAG, "Can't map the load header: APPCPU not started");
		return -EIO;
	}

	memcpy((void *)&mcuboot_header, data, sizeof(mcuboot_header));
	memcpy((void *)&image_header, data + (hdr_offset / sizeof(uint32_t)),
	       sizeof(esp_image_load_header_t));

	sys_munmap(data);

	if (image_header.header_magic == ESP_LOAD_HEADER_MAGIC) {
		ESP_EARLY_LOGI(TAG,
			"APPCPU image, area id: %d, offset: 0x%x, hdr.off: 0x%x, size: %d kB",
			fa_id, fa_offset, hdr_offset, fa_size / 1024);
	} else if ((image_header.header_magic & 0xff) == 0xE9) {
		ESP_EARLY_LOGE(TAG, "ESP image format is not supported");
		return -ENOEXEC;
	} else {
		ESP_EARLY_LOGE(TAG, "Unknown or empty image detected: APPCPU not started");
		return -ENOEXEC;
	}

	if (!segment_in_region(image_header.iram_dest_addr, image_header.iram_size,
			       APPCPU_IRAM_START, APPCPU_IRAM_END)) {
		ESP_EARLY_LOGE(TAG, "IRAM region in load header is not valid: APPCPU not started");
		return -ENOEXEC;
	}

	if (!segment_in_region(image_header.dram_dest_addr, image_header.dram_size,
			       APPCPU_DRAM_START, APPCPU_DRAM_END)) {
		ESP_EARLY_LOGE(TAG, "DRAM region in load header is not valid: APPCPU not started");
		return -ENOEXEC;
	}

	if ((image_header.entry_addr < APPCPU_IRAM_START) ||
	    (image_header.entry_addr >= APPCPU_IRAM_END)) {
		ESP_EARLY_LOGE(TAG,
			       "Application entry point (%xh) is not in IRAM: APPCPU not started",
			       image_header.entry_addr);
		return -ENOEXEC;
	}

	if (image_header.iram_flash_offset > fa_size ||
	    image_header.iram_size > fa_size - image_header.iram_flash_offset ||
	    image_header.dram_flash_offset > fa_size ||
	    image_header.dram_size > fa_size - image_header.dram_flash_offset ||
	    image_header.irom_flash_offset > fa_size ||
	    image_header.irom_size > fa_size - image_header.irom_flash_offset ||
	    image_header.drom_flash_offset > fa_size ||
	    image_header.drom_size > fa_size - image_header.drom_flash_offset) {
		ESP_EARLY_LOGE(TAG,
			       "Load header segments exceed the partition: APPCPU not started");
		return -ENOEXEC;
	}

	ESP_EARLY_LOGI(TAG, "IRAM segment: paddr=%08xh, vaddr=%08xh, size=%05xh (%6d) load",
		   (fa_offset + image_header.iram_flash_offset), image_header.iram_dest_addr,
		   image_header.iram_size, image_header.iram_size);

	if (load_segment(fa_offset + image_header.iram_flash_offset, image_header.iram_size,
			 image_header.iram_dest_addr) != 0) {
		ESP_EARLY_LOGE(TAG, "IRAM segment not loaded: APPCPU not started");
		return -EIO;
	}

	ESP_EARLY_LOGI(TAG, "DRAM segment: paddr=%08xh, vaddr=%08xh, size=%05xh (%6d) load",
		   (fa_offset + image_header.dram_flash_offset), image_header.dram_dest_addr,
		   image_header.dram_size, image_header.dram_size);

	if (load_segment(fa_offset + image_header.dram_flash_offset, image_header.dram_size,
			 image_header.dram_dest_addr) != 0) {
		ESP_EARLY_LOGE(TAG, "DRAM segment not loaded: APPCPU not started");
		return -EIO;
	}

	ESP_EARLY_LOGI(TAG, "IROM segment: paddr=%08xh, vaddr=%08xh, size=%05xh (%6d) map",
		   (fa_offset + image_header.irom_flash_offset), image_header.irom_map_addr,
		   image_header.irom_size, image_header.irom_size);

	ESP_EARLY_LOGI(TAG, "DROM segment: paddr=%08xh, vaddr=%08xh, size=%05xh (%6d) map",
		   (fa_offset + image_header.drom_flash_offset), image_header.drom_map_addr,
		   image_header.drom_size, image_header.drom_size);

	struct rom_segments rom = {
		.irom_map_addr = image_header.irom_map_addr,
		.irom_flash_offset = image_header.irom_flash_offset + fa_offset,
		.irom_size = image_header.irom_size,
		.drom_map_addr = image_header.drom_map_addr,
		.drom_flash_offset = image_header.drom_flash_offset + fa_offset,
		.drom_size = image_header.drom_size,
	};

	map_rom_segments(1, &rom);

	ESP_EARLY_LOGI(TAG, "Application start=%xh\n\n", image_header.entry_addr);
	esp_rom_output_tx_wait_idle(0);

	assert(entry_addr != NULL);
	*entry_addr = image_header.entry_addr;

	return 0;
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
	if (esp_appcpu_image_load(hdr_offset, &entry_addr) != 0) {
		return;
	}

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

#if !defined(CONFIG_MCUBOOT)
extern int esp_appcpu_init(void);
SYS_INIT(esp_appcpu_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif

#endif /* CONFIG_SOC_ENABLE_APPCPU */
