/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <hal/mmu_hal.h>
#include <hal/mmu_types.h>
#include <hal/cache_types.h>
#include <hal/cache_ll.h>
#include <hal/cache_hal.h>
#include <rom/cache.h>
#include <esp_rom_sys.h>
#include <esp_err.h>

#include <esp_app_format.h>
#include <zephyr/storage/flash_map.h>
#include <esp_rom_uart.h>
#include <esp_flash.h>
#include <esp_log.h>
#include <bootloader_clock.h>
#include <bootloader_common.h>
#include <esp_cpu.h>

#include <zephyr/linker/linker-defs.h>
#include <kernel_internal.h>

#if CONFIG_SOC_SERIES_ESP32C6
#include <soc/hp_apm_reg.h>
#include <soc/lp_apm_reg.h>
#include <soc/lp_apm0_reg.h>
#include <soc/pcr_reg.h>
#endif /* CONFIG_SOC_SERIES_ESP32C6 */

#include <esp_flash_internal.h>
#include <bootloader_flash.h>
#include <bootloader_flash_priv.h>
#include <hal/efuse_ll.h>
#include <hal/efuse_hal.h>
#include <hal/wdt_hal.h>
#include <soc/chip_revision.h>
#include <soc/rtc.h>
#ifndef CONFIG_SOC_SERIES_ESP32
#include <soc/assist_debug_reg.h>
#include <soc/system_reg.h>
#endif

#include "hw_init.h"
#include "soc_init.h"
#include "soc_random.h"

#if defined(CONFIG_SOC_ESP32S3_APPCPU) || defined(CONFIG_SOC_ESP32_APPCPU)
#error "APPCPU does not need this file!"
#endif

#define TAG "boot"

#define CHECKSUM_ALIGN 16
#define IS_PADD(addr) (addr == 0)
#define IS_DRAM(addr) (addr >= SOC_DRAM_LOW && addr < SOC_DRAM_HIGH)
#define IS_IRAM(addr) (addr >= SOC_IRAM_LOW && addr < SOC_IRAM_HIGH)
#define IS_IROM(addr) (addr >= SOC_IROM_LOW && addr < SOC_IROM_HIGH)
#define IS_DROM(addr) (addr >= SOC_DROM_LOW && addr < SOC_DROM_HIGH)
#ifdef SOC_RTC_MEM_SUPPORTED
#define IS_RTC(addr) (addr >= SOC_RTC_DRAM_LOW && addr < SOC_RTC_DRAM_HIGH)
#else
#define IS_RTC(addr) 0
#endif
#define IS_SRAM(addr) (IS_IRAM(addr) || IS_DRAM(addr))
#define IS_MMAP(addr) (IS_IROM(addr) || IS_DROM(addr))
#define IS_NONE(addr) (!IS_IROM(addr) && !IS_DROM(addr) \
			&& !IS_IRAM(addr) && !IS_DRAM(addr) && !IS_PADD(addr) && !IS_RTC(addr))

#define HDR_ATTR __attribute__((section(".entry_addr"))) __attribute__((used))

#if !defined(CONFIG_SOC_ESP32_APPCPU) && !defined(CONFIG_SOC_ESP32S3_APPCPU)
#define PART_OFFSET FIXED_PARTITION_OFFSET(slot0_partition)
#else
#define PART_OFFSET FIXED_PARTITION_OFFSET(slot0_appcpu_partition)
#endif

void __start(void);
static HDR_ATTR void (*_entry_point)(void) = &__start;

esp_image_header_t WORD_ALIGNED_ATTR bootloader_image_hdr;
extern uint32_t _image_irom_start, _image_irom_size, _image_irom_vaddr;
extern uint32_t _image_drom_start, _image_drom_size, _image_drom_vaddr;

#ifndef CONFIG_MCUBOOT

extern uint32_t _libc_heap_size;
static uint32_t libc_heap_size = (uint32_t)&_libc_heap_size;

static struct rom_segments map = {
	.irom_map_addr = (uint32_t)&_image_irom_vaddr,
	.irom_flash_offset = PART_OFFSET + (uint32_t)&_image_irom_start,
	.irom_size = (uint32_t)&_image_irom_size,
	.drom_map_addr = ((uint32_t)&_image_drom_vaddr),
	.drom_flash_offset = PART_OFFSET + (uint32_t)&_image_drom_start,
	.drom_size = (uint32_t)&_image_drom_size,
};

void map_rom_segments(int core, struct rom_segments *map)
{
	uint32_t app_irom_vaddr_align = map->irom_map_addr & MMU_FLASH_MASK;
	uint32_t app_irom_start_align = map->irom_flash_offset & MMU_FLASH_MASK;

	uint32_t app_drom_vaddr_align = map->drom_map_addr & MMU_FLASH_MASK;
	uint32_t app_drom_start_align = map->drom_flash_offset & MMU_FLASH_MASK;

	/* Traverse segments to fix flash offset changes due to post-build processing */
#ifndef CONFIG_BOOTLOADER_MCUBOOT
	esp_image_segment_header_t WORD_ALIGNED_ATTR segment_hdr;
	size_t offset = FIXED_PARTITION_OFFSET(boot_partition);
	bool checksum = false;
	unsigned int segments = 0;
	unsigned int ram_segments = 0;

	offset += sizeof(esp_image_header_t);

	while (segments++ < 16) {

		if (esp_rom_flash_read(offset, &segment_hdr,
					      sizeof(esp_image_segment_header_t), true) != 0) {
			ESP_EARLY_LOGE(TAG, "Failed to read segment header at %x", offset);
			abort();
		}

		/* TODO: Find better end-of-segment detection */
		if (IS_NONE(segment_hdr.load_addr)) {
			/* Total segment count = (segments - 1) */
			break;
		}

		ESP_EARLY_LOGI(TAG, "%s: lma 0x%08x vma 0x%08x len 0x%-6x (%u)",
			IS_NONE(segment_hdr.load_addr) ? "???" :
			 IS_MMAP(segment_hdr.load_addr) ?
			  IS_IROM(segment_hdr.load_addr) ? "IMAP" : "DMAP" :
			    IS_DRAM(segment_hdr.load_addr) ? "DRAM" :
				IS_RTC(segment_hdr.load_addr) ? "RTC" : "IRAM",
			offset + sizeof(esp_image_segment_header_t),
			segment_hdr.load_addr, segment_hdr.data_len, segment_hdr.data_len);

		/* Fix drom and irom produced be the linker, as it could
		 * be invalidated by the elf2image and flash load offset
		 */
		if (segment_hdr.load_addr == map->drom_map_addr) {
			map->drom_flash_offset = offset + sizeof(esp_image_segment_header_t);
			app_drom_start_align = map->drom_flash_offset & MMU_FLASH_MASK;
		}
		if (segment_hdr.load_addr == map->irom_map_addr) {
			map->irom_flash_offset = offset + sizeof(esp_image_segment_header_t);
			app_irom_start_align = map->irom_flash_offset & MMU_FLASH_MASK;
		}
		if (IS_SRAM(segment_hdr.load_addr) || IS_RTC(segment_hdr.load_addr)) {
			ram_segments++;
		}

		offset += sizeof(esp_image_segment_header_t) + segment_hdr.data_len;

		if (ram_segments == bootloader_image_hdr.segment_count && !checksum) {
			offset += (CHECKSUM_ALIGN - 1) - (offset % CHECKSUM_ALIGN) + 1;
			checksum = true;
		}
	}
	if (segments == 0 || segments == 16) {
		ESP_EARLY_LOGE(TAG, "Error parsing segments");
		abort();
	}

	ESP_EARLY_LOGI(TAG, "Image with %d segments", segments - 1);
#endif /* !CONFIG_BOOTLOADER_MCUBOOT */

#if CONFIG_SOC_SERIES_ESP32
	Cache_Read_Disable(core);
	Cache_Flush(core);
#else
	cache_hal_disable(CACHE_TYPE_ALL);
#endif /* CONFIG_SOC_SERIES_ESP32 */

	/* Clear the MMU entries that are already set up,
	 * so the new app only has the mappings it creates.
	 */
	if (core == 0) {
		mmu_hal_unmap_all();
	}

#if CONFIG_SOC_SERIES_ESP32
	int rc = 0;
	uint32_t drom_page_count =
		(map->drom_size + CONFIG_MMU_PAGE_SIZE - 1) / CONFIG_MMU_PAGE_SIZE;

	rc |= cache_flash_mmu_set(core, 0, app_drom_vaddr_align, app_drom_start_align, 64,
				  drom_page_count);

	uint32_t irom_page_count =
		(map->irom_size + CONFIG_MMU_PAGE_SIZE - 1) / CONFIG_MMU_PAGE_SIZE;

	rc |= cache_flash_mmu_set(core, 0, app_irom_vaddr_align, app_irom_start_align, 64,
				  irom_page_count);
	if (rc != 0) {
		ESP_EARLY_LOGE(TAG, "Failed to setup flash cache (e=0x%X). Aborting!", rc);
		abort();
	}
#else
	uint32_t actual_mapped_len = 0;

	mmu_hal_map_region(core, MMU_TARGET_FLASH0, app_drom_vaddr_align, app_drom_start_align,
			   map->drom_size, &actual_mapped_len);

	mmu_hal_map_region(core, MMU_TARGET_FLASH0, app_irom_vaddr_align, app_irom_start_align,
			   map->irom_size, &actual_mapped_len);
#endif /* CONFIG_SOC_SERIES_ESP32 */

	/* ----------------------Enable corresponding buses---------------- */
	cache_bus_mask_t bus_mask;

	bus_mask = cache_ll_l1_get_bus(core, app_drom_vaddr_align, map->drom_size);
	cache_ll_l1_enable_bus(core, bus_mask);
	bus_mask = cache_ll_l1_get_bus(core, app_irom_vaddr_align, map->irom_size);
	cache_ll_l1_enable_bus(core, bus_mask);

#if CONFIG_MP_MAX_NUM_CPUS > 1
	bus_mask = cache_ll_l1_get_bus(1, app_drom_vaddr_align, map->drom_size);
	cache_ll_l1_enable_bus(1, bus_mask);
	bus_mask = cache_ll_l1_get_bus(1, app_irom_vaddr_align, map->irom_size);
	cache_ll_l1_enable_bus(1, bus_mask);
#endif

	/* ----------------------Enable Cache---------------- */
#if CONFIG_SOC_SERIES_ESP32
	/* Application will need to do Cache_Flush(1) and Cache_Read_Enable(1) */
	Cache_Read_Enable(core);
#else
	cache_hal_enable(CACHE_TYPE_ALL);
#endif /* CONFIG_SOC_SERIES_ESP32 */

#if !defined(CONFIG_SOC_SERIES_ESP32) && !defined(CONFIG_SOC_SERIES_ESP32S2)
	/* Configure the Cache MMU size for instruction and rodata in flash. */
	uint32_t cache_mmu_irom_size =
		((map->irom_size + CONFIG_MMU_PAGE_SIZE - 1) / CONFIG_MMU_PAGE_SIZE) *
		sizeof(uint32_t);

	/* Split the cache usage by the segment sizes */
	Cache_Set_IDROM_MMU_Size(cache_mmu_irom_size, CACHE_DROM_MMU_MAX_END - cache_mmu_irom_size);
#endif
}
#endif /* !CONFIG_MCUBOOT */

void __start(void)
{
#ifdef CONFIG_RISCV_GP

	__asm__ __volatile__("la t0, _esp_vector_table\n"
			     "csrw mtvec, t0\n");

	/* Disable normal interrupts. */
	csr_read_clear(mstatus, MSTATUS_MIE);

	/* Configure the global pointer register
	 * (This should be the first thing startup does, as any other piece of code could be
	 * relaxed by the linker to access something relative to __global_pointer$)
	 */
	__asm__ __volatile__(".option push\n"
			     ".option norelax\n"
			     "la gp, __global_pointer$\n"
			     ".option pop");

	z_bss_zero();

#else /* xtensa */

	extern uint32_t _init_start;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__("wsr %0, vecbase" : : "r"(&_init_start));

	z_bss_zero();

	__asm__ __volatile__("" : : "g"(&__bss_start) : "memory");

	/* Disable normal interrupts. */
	__asm__ __volatile__("wsr %0, PS" : : "r"(PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM | PS_WOE));

	/* Initialize the architecture CPU pointer.  Some of the
	 * initialization code wants a valid arch_current_thread() before
	 * arch_kernel_init() is invoked.
	 */
	__asm__ __volatile__("wsr.MISC0 %0; rsync" : : "r"(&_kernel.cpus[0]));

#endif /* CONFIG_RISCV_GP */

/* Initialize hardware only during 1st boot  */
#if defined(CONFIG_MCUBOOT) || defined(CONFIG_ESP_SIMPLE_BOOT)
	if (hardware_init()) {
		ESP_EARLY_LOGE(TAG, "HW init failed, aborting");
		abort();
	}
#endif

#if defined(CONFIG_ESP_SIMPLE_BOOT) || defined(CONFIG_BOOTLOADER_MCUBOOT)
	map_rom_segments(0, &map);

	/* Show map segments continue using same log format as during MCUboot phase */
	ESP_EARLY_LOGI(TAG, "%s segment: paddr=%08xh, vaddr=%08xh, size=%05Xh (%6d) map", "IROM",
		       map.irom_flash_offset, map.irom_map_addr, map.irom_size, map.irom_size);
	ESP_EARLY_LOGI(TAG, "%s segment: paddr=%08xh, vaddr=%08xh, size=%05Xh (%6d) map", "DROM",
		       map.drom_flash_offset, map.drom_map_addr, map.drom_size, map.drom_size);
	esp_rom_uart_tx_wait_idle(CONFIG_ESP_CONSOLE_UART_NUM);

	/* Disable RNG entropy source as it was already used */
	soc_random_disable();

	/* Disable glitch detection as it can be falsely triggered by EMI interference */
	ana_clock_glitch_reset_config(false);

	ESP_EARLY_LOGI(TAG, "libc heap size %d kB.", libc_heap_size / 1024);

	__esp_platform_app_start();
#endif /* CONFIG_ESP_SIMPLE_BOOT || CONFIG_BOOTLOADER_MCUBOOT */

#if defined(CONFIG_MCUBOOT)
	__esp_platform_mcuboot_start();
#endif
}
