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

#include "console_init.h"
#include "flash_init.h"
#include "soc_flash_init.h"
#include "soc_init.h"
#include "soc_random.h"

#define TAG "boot"

#define CHECKSUM_ALIGN 16
#define IS_PADD(addr)  (addr == 0)
#define IS_DRAM(addr)  (addr >= SOC_DRAM_LOW && addr < SOC_DRAM_HIGH)
#define IS_IRAM(addr)  (addr >= SOC_IRAM_LOW && addr < SOC_IRAM_HIGH)
#define IS_IROM(addr)  (addr >= SOC_IROM_LOW && addr < SOC_IROM_HIGH)
#define IS_DROM(addr)  (addr >= SOC_DROM_LOW && addr < SOC_DROM_HIGH)
#define IS_SRAM(addr)  (IS_IRAM(addr) || IS_DRAM(addr))
#define IS_MMAP(addr)  (IS_IROM(addr) || IS_DROM(addr))
#define IS_NONE(addr)                                                                              \
	(!IS_IROM(addr) && !IS_DROM(addr) && !IS_IRAM(addr) && !IS_DRAM(addr) && !IS_PADD(addr))

#define HDR_ATTR __attribute__((section(".entry_addr"))) __attribute__((used))

#define IS_MAX_REV_SET(max_chip_rev_full)                                                          \
	(((max_chip_rev_full) != 65535) && ((max_chip_rev_full) != 0))

void __start(void);
static HDR_ATTR void (*_entry_point)(void) = &__start;

esp_image_header_t WORD_ALIGNED_ATTR bootloader_image_hdr;
extern uint32_t _image_irom_start, _image_irom_size, _image_irom_vaddr;
extern uint32_t _image_drom_start, _image_drom_size, _image_drom_vaddr;

#ifndef CONFIG_MCUBOOT
static uint32_t _app_irom_start =
	(FIXED_PARTITION_OFFSET(slot0_partition) + (uint32_t)&_image_irom_start);
static uint32_t _app_irom_size = (uint32_t)&_image_irom_size;

static uint32_t _app_drom_start =
	(FIXED_PARTITION_OFFSET(slot0_partition) + (uint32_t)&_image_drom_start);
static uint32_t _app_drom_size = (uint32_t)&_image_drom_size;
#endif

static uint32_t _app_irom_vaddr = ((uint32_t)&_image_irom_vaddr);
static uint32_t _app_drom_vaddr = ((uint32_t)&_image_drom_vaddr);

#ifndef CONFIG_BOOTLOADER_MCUBOOT
static int spi_flash_read(uint32_t address, void *buffer, size_t length)
{
	return esp_flash_read(NULL, buffer, address, length);
}
#endif /* CONFIG_BOOTLOADER_MCUBOOT */

void map_rom_segments(uint32_t app_drom_start, uint32_t app_drom_vaddr, uint32_t app_drom_size,
		      uint32_t app_irom_start, uint32_t app_irom_vaddr, uint32_t app_irom_size)
{
	uint32_t app_irom_start_aligned = app_irom_start & MMU_FLASH_MASK;
	uint32_t app_irom_vaddr_aligned = app_irom_vaddr & MMU_FLASH_MASK;

	uint32_t app_drom_start_aligned = app_drom_start & MMU_FLASH_MASK;
	uint32_t app_drom_vaddr_aligned = app_drom_vaddr & MMU_FLASH_MASK;

#ifndef CONFIG_BOOTLOADER_MCUBOOT
	esp_image_segment_header_t WORD_ALIGNED_ATTR segment_hdr;
	size_t offset = FIXED_PARTITION_OFFSET(boot_partition);
	bool checksum = false;
	unsigned int segments = 0;
	unsigned int ram_segments = 0;

	/* Using already fetched bootloader image header from bootloader_init */
	offset += sizeof(esp_image_header_t);

	while (segments++ < 16) {

		if (spi_flash_read(offset, &segment_hdr, sizeof(esp_image_segment_header_t)) != 0) {
			ESP_EARLY_LOGE(TAG, "Failed to read segment header at %x", offset);
			abort();
		}

		/* TODO: Find better end-of-segment detection */
		if (IS_NONE(segment_hdr.load_addr)) {
			/* Total segment count = (segments - 1) */
			break;
		}

		ESP_EARLY_LOGI(TAG, "%s: lma 0x%08x vma 0x%08x len 0x%-6x (%u)",
			       IS_NONE(segment_hdr.load_addr) ? "???"
			       : IS_MMAP(segment_hdr.load_addr)
				       ? IS_IROM(segment_hdr.load_addr) ? "IMAP" : "DMAP"
			       : IS_PADD(segment_hdr.load_addr) ? "padd"
			       : IS_DRAM(segment_hdr.load_addr) ? "DRAM"
								: "IRAM",
			       offset + sizeof(esp_image_segment_header_t), segment_hdr.load_addr,
			       segment_hdr.data_len, segment_hdr.data_len);

		/* Fix drom and irom produced be the linker, as it could
		 * be invalidated by the elf2image and flash load offset
		 */
		if (segment_hdr.load_addr == _app_drom_vaddr) {
			app_drom_start = offset + sizeof(esp_image_segment_header_t);
			app_drom_start_aligned = app_drom_start & MMU_FLASH_MASK;
		}
		if (segment_hdr.load_addr == _app_irom_vaddr) {
			app_irom_start = offset + sizeof(esp_image_segment_header_t);
			app_irom_start_aligned = app_irom_start & MMU_FLASH_MASK;
		}
		if (IS_SRAM(segment_hdr.load_addr)) {
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
	Cache_Read_Disable(0);
	Cache_Flush(0);
#else
	cache_hal_disable(CACHE_TYPE_ALL);
#endif /* CONFIG_SOC_SERIES_ESP32 */

	/* Clear the MMU entries that are already set up,
	 * so the new app only has the mappings it creates.
	 */
	mmu_hal_unmap_all();

#if CONFIG_SOC_SERIES_ESP32
	int rc = 0;
	uint32_t drom_page_count =
		(app_drom_size + CONFIG_MMU_PAGE_SIZE - 1) / CONFIG_MMU_PAGE_SIZE;

	rc |= cache_flash_mmu_set(0, 0, app_drom_vaddr_aligned, app_drom_start_aligned, 64,
				  drom_page_count);
	rc |= cache_flash_mmu_set(1, 0, app_drom_vaddr_aligned, app_drom_start_aligned, 64,
				  drom_page_count);

	uint32_t irom_page_count =
		(app_irom_size + CONFIG_MMU_PAGE_SIZE - 1) / CONFIG_MMU_PAGE_SIZE;

	rc |= cache_flash_mmu_set(0, 0, app_irom_vaddr_aligned, app_irom_start_aligned, 64,
				  irom_page_count);
	rc |= cache_flash_mmu_set(1, 0, app_irom_vaddr_aligned, app_irom_start_aligned, 64,
				  irom_page_count);
	if (rc != 0) {
		ESP_EARLY_LOGE(TAG, "Failed to setup XIP, aborting");
		abort();
	}
#else
	uint32_t actual_mapped_len = 0;

	mmu_hal_map_region(0, MMU_TARGET_FLASH0, app_drom_vaddr_aligned, app_drom_start_aligned,
			   app_drom_size, &actual_mapped_len);

	mmu_hal_map_region(0, MMU_TARGET_FLASH0, app_irom_vaddr_aligned, app_irom_start_aligned,
			   app_irom_size, &actual_mapped_len);
#endif /* CONFIG_SOC_SERIES_ESP32 */

	/* ----------------------Enable corresponding buses---------------- */
	cache_bus_mask_t bus_mask = cache_ll_l1_get_bus(0, app_drom_vaddr_aligned, app_drom_size);

	cache_ll_l1_enable_bus(0, bus_mask);
	bus_mask = cache_ll_l1_get_bus(0, app_irom_vaddr_aligned, app_irom_size);
	cache_ll_l1_enable_bus(0, bus_mask);
#if CONFIG_MP_MAX_NUM_CPUS > 1
	bus_mask = cache_ll_l1_get_bus(1, app_drom_vaddr_aligned, app_drom_size);
	cache_ll_l1_enable_bus(1, bus_mask);
	bus_mask = cache_ll_l1_get_bus(1, app_irom_vaddr_aligned, app_irom_size);
	cache_ll_l1_enable_bus(1, bus_mask);
#endif

	/* ----------------------Enable Cache---------------- */
#if CONFIG_SOC_SERIES_ESP32
	/* Application will need to do Cache_Flush(1) and Cache_Read_Enable(1) */
	Cache_Read_Enable(0);
#else
	cache_hal_enable(CACHE_TYPE_ALL);
#endif /* CONFIG_SOC_SERIES_ESP32 */

#if !defined(CONFIG_SOC_SERIES_ESP32) && !defined(CONFIG_SOC_SERIES_ESP32S2)
	/* Configure the Cache MMU size for instruction and rodata in flash. */
	uint32_t cache_mmu_irom_size =
		((app_irom_size + CONFIG_MMU_PAGE_SIZE - 1) / CONFIG_MMU_PAGE_SIZE) *
		sizeof(uint32_t);

	/* Split the cache usage by the segment sizes */
	Cache_Set_IDROM_MMU_Size(cache_mmu_irom_size, CACHE_DROM_MMU_MAX_END - cache_mmu_irom_size);
#endif
	/* Show map segments continue using same log format as during MCUboot phase */
	ESP_EARLY_LOGI(TAG, "%s segment: paddr=%08xh, vaddr=%08xh, size=%05Xh (%6d) map", "DROM",
		       app_drom_start_aligned, app_drom_vaddr_aligned, app_drom_size,
		       app_drom_size);
	ESP_EARLY_LOGI(TAG, "%s segment: paddr=%08xh, vaddr=%08xh, size=%05Xh (%6d) map", "IROM",
		       app_irom_start_aligned, app_irom_vaddr_aligned, app_irom_size,
		       app_irom_size);
	ets_printf("\n\r");
	esp_rom_uart_tx_wait_idle(0);
}

#ifndef CONFIG_BOOTLOADER_MCUBOOT
#ifndef CONFIG_SOC_SERIES_ESP32
static void super_wdt_auto_feed(void)
{
#if defined(CONFIG_SOC_SERIES_ESP32S3) || defined(CONFIG_SOC_SERIES_ESP32C2) ||                    \
	defined(CONFIG_SOC_SERIES_ESP32C3)
	REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, RTC_CNTL_SWD_WKEY_VALUE);
	REG_SET_BIT(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_AUTO_FEED_EN);
	REG_WRITE(RTC_CNTL_SWD_WPROTECT_REG, 0);
#elif CONFIG_SOC_SERIES_ESP32S2
	REG_SET_BIT(RTC_CNTL_SWD_CONF_REG, RTC_CNTL_SWD_AUTO_FEED_EN);
#elif CONFIG_SOC_SERIES_ESP32C6
	REG_WRITE(LP_WDT_SWD_WPROTECT_REG, LP_WDT_SWD_WKEY_VALUE);
	REG_SET_BIT(LP_WDT_SWD_CONFIG_REG, LP_WDT_SWD_AUTO_FEED_EN);
	REG_WRITE(LP_WDT_SWD_WPROTECT_REG, 0);
#endif
}
#endif /* !CONFIG_SOC_SERIES_ESP32 */

void print_banner(void)
{
#ifdef CONFIG_MCUBOOT
	ESP_EARLY_LOGI(TAG, "MCUboot 2nd stage bootloader");
#elif CONFIG_BOOTLOADER_MCUBOOT
	ESP_EARLY_LOGI(TAG, "MCUboot Application image");
#else /* CONFIG_ESP_SIMPLE_BOOT */
	ESP_EARLY_LOGI(TAG, "ESP Simple boot");
#endif
	ESP_EARLY_LOGI(TAG, "compile time " __DATE__ " " __TIME__);
#ifndef CONFIG_SMP
#if (SOC_CPU_CORES_NUM > 1)
	ESP_EARLY_LOGW(TAG, "Unicore bootloader");
#endif
#else
	ESP_EARLY_LOGI(TAG, "Multicore bootloader");
#endif
}

int read_bootloader_header(void)
{
	/* load bootloader image header */
	if (bootloader_flash_read(FIXED_PARTITION_OFFSET(boot_partition), &bootloader_image_hdr,
				  sizeof(esp_image_header_t), true) != 0) {
		ESP_EARLY_LOGE(TAG, "failed to load bootloader image header!");
		return -EIO;
	}
	return 0;
}

int check_chip_validity(const esp_image_header_t *img_hdr, esp_image_type type)
{
	int err = 0;
	esp_chip_id_t chip_id = CONFIG_IDF_FIRMWARE_CHIP_ID;

	if (chip_id != img_hdr->chip_id) {
		ESP_EARLY_LOGE(TAG, "mismatch chip ID, expected %d, found %d", chip_id,
			       img_hdr->chip_id);
		err = -EIO;
	} else {
		unsigned int revision = efuse_hal_chip_revision();
		unsigned int major_rev = revision / 100;
		unsigned int minor_rev = revision % 100;
		unsigned int min_rev = img_hdr->min_chip_rev_full;

		if (type == ESP_IMAGE_BOOTLOADER || type == ESP_IMAGE_APPLICATION) {
			if (!ESP_CHIP_REV_ABOVE(revision, min_rev)) {
				ESP_EARLY_LOGE(
					TAG,
					"Image requires chip rev >= v%d.%d, but chip is v%d.%d",
					min_rev / 100, min_rev % 100, major_rev, minor_rev);
				err = ESP_FAIL;
			}
		}
		if (type == ESP_IMAGE_APPLICATION) {
			unsigned int max_rev = img_hdr->max_chip_rev_full;

			if ((IS_MAX_REV_SET(max_rev) && (revision > max_rev) &&
			     !efuse_ll_get_disable_wafer_version_major())) {
				ESP_EARLY_LOGE(
					TAG,
					"Image requires chip rev <= v%d.%d, but chip is v%d.%d",
					max_rev / 100, max_rev % 100, major_rev, minor_rev);
				err = ESP_FAIL;
			}
		}
	}

	return err;
}

int check_bootloader_validity(void)
{
	unsigned int revision = efuse_hal_chip_revision();
	unsigned int major = revision / 100;
	unsigned int minor = revision % 100;

	ESP_EARLY_LOGI(TAG, "chip revision: v%d.%d", major, minor);

#if defined(CONFIG_SOC_SERIES_ESP32)
	if (major < 3) {
		ESP_EARLY_LOGE(TAG,
			       "You are using ESP32 chip revision (%d) that is unsupported. While "
			       "it may work, it could cause unexpected behavior or issues.",
			       major);
		ESP_EARLY_LOGE(TAG,
			       "Proceeding with this ESP32 chip revision is not recommended unless "
			       "you fully understand the potential risk and limitations.");
#if !defined(CONFIG_ESP32_USE_UNSUPPORTED_REVISION)
		ESP_EARLY_LOGE(
			TAG,
			"If you choose to continue, please enable the "
			"'CONFIG_ESP32_USE_UNSUPPORTED_REVISION' in your project configuration.");
		return -EIO;
#endif
	}
#endif
	/* compare with the one set in bootloader image header */
	if (check_chip_validity(&bootloader_image_hdr, ESP_IMAGE_BOOTLOADER) != 0) {
		return ESP_FAIL;
	}
	return 0;
}

#if defined(CONFIG_SOC_SERIES_ESP32S3) || defined(CONFIG_SOC_SERIES_ESP32)
static void wdt_reset_info_dump(int cpu)
{
	uint32_t inst = 0, pid = 0, stat = 0, data = 0, pc = 0, lsstat = 0, lsaddr = 0, lsdata = 0,
		 dstat = 0;
	const char *cpu_name = cpu ? "APP" : "PRO";

	stat = 0xdeadbeef;
	pid = 0;

	if (cpu == 0) {
#ifdef CONFIG_SOC_SERIES_ESP32S3
		inst = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGINST_REG);
		dstat = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGSTATUS_REG);
		data = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGDATA_REG);
		pc = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGPC_REG);
		lsstat = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGLS0STAT_REG);
		lsaddr = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGLS0ADDR_REG);
		lsdata = REG_READ(ASSIST_DEBUG_CORE_0_RCD_PDEBUGLS0DATA_REG);
#elif CONFIG_SOC_SERIES_ESP32
		stat = DPORT_REG_READ(DPORT_PRO_CPU_RECORD_STATUS_REG);
		pid = DPORT_REG_READ(DPORT_PRO_CPU_RECORD_PID_REG);
		inst = DPORT_REG_READ(DPORT_PRO_CPU_RECORD_PDEBUGINST_REG);
		dstat = DPORT_REG_READ(DPORT_PRO_CPU_RECORD_PDEBUGSTATUS_REG);
		data = DPORT_REG_READ(DPORT_PRO_CPU_RECORD_PDEBUGDATA_REG);
		pc = DPORT_REG_READ(DPORT_PRO_CPU_RECORD_PDEBUGPC_REG);
		lsstat = DPORT_REG_READ(DPORT_PRO_CPU_RECORD_PDEBUGLS0STAT_REG);
		lsaddr = DPORT_REG_READ(DPORT_PRO_CPU_RECORD_PDEBUGLS0ADDR_REG);
		lsdata = DPORT_REG_READ(DPORT_PRO_CPU_RECORD_PDEBUGLS0DATA_REG);
#endif
	} else {
		ESP_EARLY_LOGE(TAG, "WDT reset info: %s CPU not support!\n", cpu_name);
		return;
	}

	ESP_EARLY_LOGD(TAG, "WDT reset info: %s CPU STATUS        0x%08" PRIx32, cpu_name, stat);
	ESP_EARLY_LOGD(TAG, "WDT reset info: %s CPU PID           0x%08" PRIx32, cpu_name, pid);
	ESP_EARLY_LOGD(TAG, "WDT reset info: %s CPU PDEBUGINST    0x%08" PRIx32, cpu_name, inst);
	ESP_EARLY_LOGD(TAG, "WDT reset info: %s CPU PDEBUGSTATUS  0x%08" PRIx32, cpu_name, dstat);
	ESP_EARLY_LOGD(TAG, "WDT reset info: %s CPU PDEBUGDATA    0x%08" PRIx32, cpu_name, data);
	ESP_EARLY_LOGD(TAG, "WDT reset info: %s CPU PDEBUGPC      0x%08" PRIx32, cpu_name, pc);
	ESP_EARLY_LOGD(TAG, "WDT reset info: %s CPU PDEBUGLS0STAT 0x%08" PRIx32, cpu_name, lsstat);
	ESP_EARLY_LOGD(TAG, "WDT reset info: %s CPU PDEBUGLS0ADDR 0x%08" PRIx32, cpu_name, lsaddr);
	ESP_EARLY_LOGD(TAG, "WDT reset info: %s CPU PDEBUGLS0DATA 0x%08" PRIx32, cpu_name, lsdata);
}
#endif /* CONFIG_SOC_SERIES_ESP32S3 || CONFIG_SOC_SERIES_ESP32 */

static void wdt_reset_cpu0_info_enable(void)
{
#ifdef CONFIG_SOC_SERIES_ESP32S3
	REG_SET_BIT(SYSTEM_CPU_PERI_CLK_EN_REG, SYSTEM_CLK_EN_ASSIST_DEBUG);
	REG_CLR_BIT(SYSTEM_CPU_PERI_RST_EN_REG, SYSTEM_RST_EN_ASSIST_DEBUG);
	REG_WRITE(ASSIST_DEBUG_CORE_0_RCD_PDEBUGENABLE_REG, 1);
	REG_WRITE(ASSIST_DEBUG_CORE_0_RCD_RECORDING_REG, 1);
#elif CONFIG_SOC_SERIES_ESP32S2
	DPORT_REG_SET_BIT(DPORT_PERI_CLK_EN_REG, DPORT_PERI_EN_ASSIST_DEBUG);
	DPORT_REG_CLR_BIT(DPORT_PERI_RST_EN_REG, DPORT_PERI_EN_ASSIST_DEBUG);
	REG_WRITE(ASSIST_DEBUG_PRO_PDEBUGENABLE, 1);
	REG_WRITE(ASSIST_DEBUG_PRO_RCD_RECORDING, 1);
#elif CONFIG_SOC_SERIES_ESP32
	DPORT_REG_SET_BIT(DPORT_PRO_CPU_RECORD_CTRL_REG,
			  DPORT_PRO_CPU_PDEBUG_ENABLE | DPORT_PRO_CPU_RECORD_ENABLE);
	DPORT_REG_CLR_BIT(DPORT_PRO_CPU_RECORD_CTRL_REG, DPORT_PRO_CPU_RECORD_ENABLE);
#elif CONFIG_SOC_ESP32C2
	REG_SET_BIT(SYSTEM_CPU_PERI_CLK_EN_REG, SYSTEM_CLK_EN_ASSIST_DEBUG);
	REG_CLR_BIT(SYSTEM_CPU_PERI_RST_EN_REG, SYSTEM_RST_EN_ASSIST_DEBUG);
	REG_WRITE(ASSIST_DEBUG_CORE_0_RCD_EN_REG,
		  ASSIST_DEBUG_CORE_0_RCD_PDEBUGEN | ASSIST_DEBUG_CORE_0_RCD_RECORDEN);
#elif CONFIG_SOC_SERIES_ESP32C3
	REG_SET_BIT(SYSTEM_CPU_PERI_CLK_EN_REG, SYSTEM_CLK_EN_ASSIST_DEBUG);
	REG_CLR_BIT(SYSTEM_CPU_PERI_RST_EN_REG, SYSTEM_RST_EN_ASSIST_DEBUG);
	REG_WRITE(ASSIST_DEBUG_CORE_0_RCD_EN_REG,
		  ASSIST_DEBUG_CORE_0_RCD_PDEBUGEN | ASSIST_DEBUG_CORE_0_RCD_RECORDEN);
#elif CONFIG_SOC_SERIES_ESP32C6
	REG_SET_BIT(PCR_ASSIST_CONF_REG, PCR_ASSIST_CLK_EN);
	REG_CLR_BIT(PCR_ASSIST_CONF_REG, PCR_ASSIST_RST_EN);
	REG_WRITE(ASSIST_DEBUG_CORE_0_RCD_EN_REG,
		  ASSIST_DEBUG_CORE_0_RCD_PDEBUGEN | ASSIST_DEBUG_CORE_0_RCD_RECORDEN);
#endif
}

static void check_wdt_reset(void)
{
	int wdt_rst = 0;
	soc_reset_reason_t rst_reas;

	rst_reas = esp_rom_get_reset_reason(0);
	if (rst_reas == RESET_REASON_CORE_RTC_WDT || rst_reas == RESET_REASON_CORE_MWDT0 ||
#ifndef CONFIG_SOC_SERIES_ESP32C2
	    rst_reas == RESET_REASON_CORE_MWDT1 ||
#endif
	    rst_reas == RESET_REASON_CPU0_MWDT0 ||
#if defined(CONFIG_SOC_SERIES_ESP32S2) || defined(CONFIG_SOC_SERIES_ESP32C3) ||                    \
	defined(CONFIG_SOC_SERIES_ESP32C6)
	    rst_reas == RESET_REASON_CPU0_MWDT1 ||
#endif
	    rst_reas == RESET_REASON_CPU0_RTC_WDT) {
		ESP_EARLY_LOGW(TAG, "PRO CPU has been reset by WDT.");
		wdt_rst = 1;
	}

#if defined(CONFIG_SOC_SERIES_ESP32S3) || defined(CONFIG_SOC_SERIES_ESP32)
	if (wdt_rst) {
		wdt_reset_info_dump(0);
	}
#endif
	wdt_reset_cpu0_info_enable();
}

void config_wdt(void)
{
	/*
	 * At this point, the flashboot protection of RWDT and MWDT0 will have been
	 * automatically enabled. We can disable flashboot protection as it's not
	 * needed anymore. If configured to do so, we also initialize the RWDT to
	 * protect the remainder of the bootloader process.
	 */
	wdt_hal_context_t rwdt_ctx = RWDT_HAL_CONTEXT_DEFAULT();

	wdt_hal_write_protect_disable(&rwdt_ctx);
	wdt_hal_set_flashboot_en(&rwdt_ctx, false);
	wdt_hal_write_protect_enable(&rwdt_ctx);

	/* Disable MWDT0 flashboot protection. But only after we've enabled the RWDT first so that
	 * there's not gap in WDT protection.
	 */
	wdt_hal_context_t mwdt_ctx = {.inst = WDT_MWDT0, .mwdt_dev = &TIMERG0};

	wdt_hal_write_protect_disable(&mwdt_ctx);
	wdt_hal_set_flashboot_en(&mwdt_ctx, false);
	wdt_hal_write_protect_enable(&mwdt_ctx);
}

#ifdef CONFIG_SOC_SERIES_ESP32
static void reset_mmu(void)
{
	/* completely reset MMU in case serial bootloader was running */
	Cache_Read_Disable(0);
	Cache_Flush(0);
	mmu_init(0);

	/* normal ROM boot exits with DROM0 cache unmasked,
	 * but serial bootloader exits with it masked.
	 */
	DPORT_REG_CLR_BIT(DPORT_PRO_CACHE_CTRL1_REG, DPORT_PRO_CACHE_MASK_DROM0);
}
#endif /* CONFIG_SOC_SERIES_ESP32 */

static int hardware_init(void)
{
	int err = 0;

#if XCHAL_ERRATUM_572
	uint32_t memctl = XCHAL_CACHE_MEMCTL_DEFAULT;

	WSR(MEMCTL, memctl);
#endif /*XCHAL_ERRATUM_572*/

#if defined(CONFIG_SOC_SERIES_ESP32C3) || defined(CONFIG_SOC_SERIES_ESP32C6)
	soc_hw_init();
#endif
#ifndef CONFIG_SOC_SERIES_ESP32
#ifndef CONFIG_SOC_SERIES_ESP32S2
	ana_reset_config();
#endif /* CONFIG_SOC_SERIES_ESP32S2 */
	super_wdt_auto_feed();
#endif /* CONFIG_SOC_SERIES_ESP32 */

#if SOC_APM_SUPPORTED
	/* By default, these access path filters are enable and allow the
	 * access to masters only if they are in TEE mode. Since all masters
	 * except HP CPU boots in REE mode, default setting of these filters
	 * will deny the access to all masters except HP CPU.
	 * So, at boot disabling these filters. They will enable as per the
	 * use case by TEE initialization code.
	 */
	REG_WRITE(LP_APM_FUNC_CTRL_REG, 0);
	REG_WRITE(LP_APM0_FUNC_CTRL_REG, 0);
	REG_WRITE(HP_APM_FUNC_CTRL_REG, 0);
#endif /* SOC_APM_SUPPORTED */
#ifdef CONFIG_BOOTLOADER_REGION_PROTECTION_ENABLE
	esp_cpu_configure_region_protection();
#endif
#if CONFIG_BOOTLOADER_VDDSDIO_BOOST_1_9V
	rtc_vddsdio_config_t cfg = rtc_vddsdio_get_config();

	if (cfg.enable == 1 && cfg.tieh == RTC_VDDSDIO_TIEH_1_8V) {
		cfg.drefh = 3;
		cfg.drefm = 3;
		cfg.drefl = 3;
		cfg.force = 1;
		rtc_vddsdio_set_config(cfg);
		esp_rom_delay_us(10);
	}
#endif /* CONFIG_BOOTLOADER_VDDSDIO_BOOST_1_9V */

	bootloader_clock_configure();

	/* initialize console, from now on, we can log */
	esp_console_init();
	print_banner();

	spi_flash_init_chip_state();
	err = esp_flash_init_default_chip();
	if (err != 0) {
		ESP_EARLY_LOGE(TAG, "Failed to init flash chip, error %d", err);
		return err;
	}
#ifdef CONFIG_SOC_SERIES_ESP32
	reset_mmu();
#endif
#ifndef CONFIG_SOC_SERIES_ESP32
	cache_hal_init();
	mmu_hal_init();
#endif /* CONFIG_SOC_SERIES_ESP32 */
#ifdef CONFIG_SOC_SERIES_ESP32S2
	/* Workaround: normal ROM bootloader exits with DROM0 cache unmasked, but 2nd bootloader
	 * exits with it masked.
	 */
	REG_CLR_BIT(EXTMEM_PRO_ICACHE_CTRL1_REG, EXTMEM_PRO_ICACHE_MASK_DROM0);
#endif

	flash_update_id();

	err = bootloader_flash_xmc_startup();
	if (err != 0) {
		ESP_EARLY_LOGE(TAG, "failed when running XMC startup flow, reboot!");
		return err;
	}

	err = read_bootloader_header();
	if (err != 0) {
		return err;
	}

	err = check_bootloader_validity();
	if (err != 0) {
		return err;
	}

	err = init_spi_flash();
	if (err != 0) {
		return err;
	}

	check_wdt_reset();
	config_wdt();

#ifndef CONFIG_SOC_SERIES_ESP32C2
	soc_random_enable();
#endif
	return 0;
}
#endif /* !CONFIG_BOOTLOADER_MCUBOOT */

void __start(void)
{
#ifdef CONFIG_RISCV_GP
	/* Configure the global pointer register
	 * (This should be the first thing startup does, as any other piece of code could be
	 * relaxed by the linker to access something relative to __global_pointer$)
	 */
	__asm__ __volatile__(".option push\n"
			     ".option norelax\n"
			     "la gp, __global_pointer$\n"
			     ".option pop");
#endif /* CONFIG_RISCV_GP */

#ifndef CONFIG_BOOTLOADER_MCUBOOT
	/* Init fundamental components */
	if (hardware_init()) {
		ESP_EARLY_LOGE(TAG, "HW init failed, aborting");
		abort();
	}
#endif

#ifndef CONFIG_MCUBOOT
	map_rom_segments(_app_drom_start, _app_drom_vaddr, _app_drom_size, _app_irom_start,
			 _app_irom_vaddr, _app_irom_size);
#endif
#ifndef CONFIG_SOC_SERIES_ESP32C2
	/* Disable RNG entropy source as it was already used */
	soc_random_disable();
#endif /* CONFIG_SOC_SERIES_ESP32C2 */
#if defined(CONFIG_SOC_SERIES_ESP32S3) || defined(CONFIG_SOC_SERIES_ESP32C3)
	/* Disable glitch detection as it can be falsely triggered by EMI interference */
	ESP_EARLY_LOGI(TAG, "Disabling glitch detection");
	ana_clock_glitch_reset_config(false);
#endif /* CONFIG_SOC_SERIES_ESP32S2 */
	ESP_EARLY_LOGI(TAG, "Jumping to the main image...");
	__esp_platform_start();
}
