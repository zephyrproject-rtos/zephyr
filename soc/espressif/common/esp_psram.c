/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <rom/ets_sys.h>
#include <esp_psram.h>
#include <esp_private/esp_psram_extram.h>
#include <hal/cache_hal.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#if DT_NODE_HAS_PROP(DT_NODELABEL(psram0), vin_supply)
#include <hal/ldo_ll.h>
#endif
#if defined(CONFIG_SOC_FAMILY_ESPRESSIF_SPI_MEM_SUPPORT_TIMING_TUNING)
#include <esp_flash.h>
#include <esp_private/spi_flash_os.h>
#include <hal/spi_flash_hal.h>
#endif

extern int _instruction_reserved_start;
extern int _instruction_reserved_end;
extern int _rodata_reserved_start;
extern int _rodata_reserved_end;

extern int _ext_ram_bss_start;
extern int _ext_ram_bss_end;
extern int _ext_ram_heap_start;
extern int _ext_ram_heap_end;

struct shared_multi_heap_region smh_psram = {
	.attr = SMH_REG_ATTR_EXTERNAL,
};

int esp_psram_smh_init(void)
{
	shared_multi_heap_pool_init();
	return shared_multi_heap_add(&smh_psram, NULL);
}

#if defined(CONFIG_SOC_FAMILY_ESPRESSIF_SPI_MEM_SUPPORT_TIMING_TUNING)
/*
 * PSRAM timing tuning reprograms the MSPI core clock, which is shared
 * between flash and PSRAM. The esp_flash driver latched the flash
 * timing into its HAL context back in esp_flash_config(), so those
 * cached values are stale once tuning has run and SPI1 accesses
 * (erase, write) fail. Re-read the timing the tuning code settled on.
 */
static void esp_psram_refresh_flash_timing(void)
{
	spi_flash_hal_context_t *host;
	spi_flash_hal_timing_config_t timing = {0};

	if (!spi_flash_timing_is_tuned() || esp_flash_default_chip == NULL) {
		return;
	}

	host = (spi_flash_hal_context_t *)esp_flash_default_chip->host;

	spi_timing_get_flash_timing_param(&timing);

	host->clock_conf = timing.clock_config;
	host->extra_dummy = timing.extra_dummy;
	host->cs_setup = timing.cs_setup;
	host->cs_hold = timing.cs_hold;
}
#endif

#define PSRAM_NODE DT_NODELABEL(psram0)

#if DT_NODE_HAS_PROP(PSRAM_NODE, vin_supply)
#define PSRAM_LDO_NODE DT_PHANDLE(PSRAM_NODE, vin_supply)
#define PSRAM_LDO_MV   (DT_PROP_OR(PSRAM_LDO_NODE, regulator_init_microvolt, 0) / 1000)

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_PARENT(PSRAM_LDO_NODE)) &&
		     DT_NODE_HAS_PROP(PSRAM_LDO_NODE, regulator_init_microvolt) &&
		     DT_PROP(PSRAM_LDO_NODE, regulator_always_on),
	     "PSRAM vin-supply needs the ldo node enabled and the channel with "
	     "regulator-init-microvolt and regulator-always-on");
BUILD_ASSERT(PSRAM_LDO_MV >= LDO_LL_RECOMMEND_MIN_VOLTAGE_MV &&
		     PSRAM_LDO_MV <= LDO_LL_RAIL_VOLTAGE_MV,
	     "PSRAM vin-supply regulator-init-microvolt is out of range");

/* The regulator driver applies the same settings at PRE_KERNEL_1. */
static void psram_ldo_init(void)
{
	int unit = LDO_ID2UNIT(DT_REG_ADDR(PSRAM_LDO_NODE));
	uint8_t dref = 0U;
	uint8_t mul = 0U;
	bool use_rail = false;

	ldo_ll_enable_current_limit(unit, true);
	ldo_ll_voltage_to_dref_mul(unit, PSRAM_LDO_MV, &dref, &mul, &use_rail);
	ldo_ll_adjust_voltage(unit, dref, mul, use_rail);
	ldo_ll_set_owner(unit, LDO_LL_UNIT_OWNER_SW);
	ldo_ll_enable_ripple_suppression(unit, true);
	ldo_ll_enable(unit, true);
	ldo_ll_enable_current_limit(unit, false);
}
#endif

void esp_init_psram(void)
{
	intptr_t mapped_vaddr = 0;
	size_t mapped_size = 0;

#if DT_NODE_HAS_PROP(PSRAM_NODE, vin_supply)
	psram_ldo_init();
#endif

	/*
	 * PSRAM chip init transitions the MSPI clock through low and high
	 * speed modes and reprograms the flash/PSRAM tuning registers.
	 * Cache lines fetched during that window can hold garbage. Run the
	 * chip-init step first, invalidate the flash IROM/DROM ranges, then
	 * run the rest of PSRAM init so the next flash fetch reloads with
	 * the final MSPI settings. ESP32 cache has no per-address
	 * invalidate primitive, so the invalidate step is skipped there.
	 */
	if (esp_psram_chip_init()) {
		ets_printf("Failed to Initialize external RAM, aborting.\n");
		return;
	}

#if defined(CONFIG_SOC_FAMILY_ESPRESSIF_SPI_MEM_SUPPORT_TIMING_TUNING)
	esp_psram_refresh_flash_timing();
#endif

#if !defined(CONFIG_SOC_SERIES_ESP32)
	cache_hal_invalidate_addr((uint32_t)&_instruction_reserved_start,
				  (uint32_t)&_instruction_reserved_end -
					  (uint32_t)&_instruction_reserved_start);
	cache_hal_invalidate_addr((uint32_t)&_rodata_reserved_start,
				  (uint32_t)&_rodata_reserved_end -
					  (uint32_t)&_rodata_reserved_start);
#endif

	if (esp_psram_init()) {
		ets_printf("Failed to Initialize external RAM, aborting.\n");
		return;
	}

	if (esp_psram_get_size() < CONFIG_SOC_FAMILY_ESPRESSIF_SPIRAM_SIZE) {
		ets_printf("External RAM size is less than configured.\n");
	}

	esp_psram_get_mapped_region(&mapped_vaddr, &mapped_size);
	ARG_UNUSED(mapped_vaddr);
	ARG_UNUSED(mapped_size);

	/*
	 * Use the linker-reserved heap window inside the PSRAM-backed
	 * .ext_ram.data output section. The linker places ext_ram_noinit
	 * and ext_ram_bss content before the heap, so starting the SMH
	 * region at the raw MMU-mapped base would overlap and clobber
	 * those allocations (e.g. relocated thread stacks and net packet
	 * pools under CONFIG_ESP32_WIFI_NET_ALLOC_SPIRAM).
	 */
	smh_psram.addr = (uintptr_t)&_ext_ram_heap_start;
	smh_psram.size = (uintptr_t)&_ext_ram_heap_end -
			 (uintptr_t)&_ext_ram_heap_start;

	if (IS_ENABLED(CONFIG_SOC_FAMILY_ESPRESSIF_SPIRAM_MEMTEST)) {
		if (esp_psram_is_initialized()) {
			if (!esp_psram_extram_test()) {
				ets_printf("External RAM failed memory test!");
				return;
			}
		}
	}

	memset(&_ext_ram_bss_start, 0,
	       (&_ext_ram_bss_end - &_ext_ram_bss_start) * sizeof(_ext_ram_bss_start));
}
