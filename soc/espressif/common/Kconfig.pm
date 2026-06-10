# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

if SOC_FAMILY_ESPRESSIF_ESP32

menu "Espressif PM Config"

config ESP32_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP
	bool "Power down CPU in light sleep"
	depends on ESP32_SOC_PM_SUPPORT_CPU_PD
	select ESP32_PM_RESTORE_CACHE_TAGMEM_AFTER_LIGHT_SLEEP if ESP32S3_DATA_CACHE_16KB
	default y
	help
	  Allow the CPU power domain to switch off in light sleep. Hardware
	  saves and restores the CPU context so application code does not
	  need to handle the transition explicitly.

	  A small amount of internal memory is used for the retention image.
	  Light-sleep current is reduced compared to keeping the CPU domain on.

choice ESP32_PM_CPU_RETENTION_STRATEGY
	prompt "Retentive memory allocation strategy for light sleep"
	depends on ESP32_SOC_PM_CPU_RETENTION_BY_SW
	depends on ESP32_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP
	default ESP32_PM_CPU_RETENTION_DYNAMIC

config ESP32_PM_CPU_RETENTION_DYNAMIC
	bool "Dynamically allocate memory"
	help
	  Allocate retention data when preparing to enter light sleep.

config ESP32_PM_CPU_RETENTION_STATIC
	bool "Statically allocate memory"
	help
	  Pre-allocate all required retention memory during system initialization.

endchoice

config ESP32_PM_RESTORE_CACHE_TAGMEM_AFTER_LIGHT_SLEEP
	bool "Restore I/D-cache tag memory after CPU power-down light sleep"
	depends on SOC_SERIES_ESP32S3 && ESP32_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP
	default y
	help
	  Cache tag memory and the CPU share the CPU power domain. When enabled,
	  tag RAM is backed up to internal RAM before sleep and restored on wake so
	  cached external memory stays valid across light sleep.

	  When disabled, the D-cache is write-back before sleep and invalidated
	  after wake; external memory accesses miss in cache until repopulated.

config ESP32_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP
	bool "Power down digital peripherals in light sleep (EXPERIMENTAL)"
	depends on ESP32_SOC_PM_SUPPORT_TOP_PD && ESP32_SOC_PAU_SUPPORTED
	select ESP32_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP
	help
	  Allow the main digital (TOP) peripheral power domain to switch off
	  in light sleep. The PAU and REGDMA hardware save and restore
	  selected peripheral register context over sleep so that, when a
	  driver implements sleep retention, the peripheral can resume without
	  a full re-init.

	  This option lowers light-sleep current substantially. It also adds
	  retention metadata and runtime cost. Heap is used at sleep entry,
	  so ensure enough free heap or peripheral power down during sleep
	  may not complete.

	  Whether TOP actually switches off during a given sleep still depends on
	  HAL sleep settings and each driver's policy. The domain may stay on
	  if something in the system requires it.

	  Note: UART and similar peripherals may run pre-sleep flush logic that can
	  lengthen the path to sleep. If tickless idle misbehaves after wake,
	  consider increasing the minimum idle time before sleep.

config ESP32_PM_ESP_SLEEP_POWER_DOWN_CPU
	bool
	default y if ESP32_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP

config ESP32_TIMER_IN_IRAM
	bool

config ESP32_PM_SLP_IRAM_OPT
	bool "Put light sleep path code in internal RAM"
	select ESP32_TIMER_IN_IRAM
	help
	  Places part of the light sleep sequence in IRAM so it can run while the
	  cache is suspended. Enable when using SPI flash deep power-down or
	  flash supply power-down in light sleep (options below). Uses roughly
	  2 KB of IRAM.

config ESP32_SLEEP_POWER_DOWN_FLASH
	bool "Power down flash supply in light sleep (VDDSDIO path)"
	depends on ESP32_SOC_PM_SUPPORT_VDDSDIO_PD
	depends on ESP32_SOC_FLASH_SUPPORTED
	depends on !ESP_SPIRAM
	select ESP32_PM_SLP_IRAM_OPT
	help
	  Power down the flash supply during light sleep when the hardware allows
	  (VDDSDIO / flash domain). Higher wake latency and stricter constraints
	  than SPI command deep power-down. Do not enable with SPIRAM unless the
	  PSRAM supply is independent (not modeled in Zephyr Kconfig yet).

	  See Espressif sleep documentation for risks and board requirements.

config ESP32_SLEEP_SET_FLASH_DPD
	bool "SPI flash deep power-down mode in light sleep"
	depends on ESP32_SOC_FLASH_SUPPORTED
	depends on !ESP32_SLEEP_POWER_DOWN_FLASH
	select ESP32_PM_SLP_IRAM_OPT
	help
	  Puts the main SPI flash into deep power-down (typically command B9h)
	  during light sleep when sleep duration is long enough. Saves roughly
	  10 uA vs standby on many parts. Requires a compatible flash (exit
	  typically ABh). Tune enter/exit delays from the datasheet.

	  When enabled, the SoC linker script also places clk_utils in IRAM so
	  MSPI/CPU clock sync after light sleep can run as soon as the cache is
	  resumed, before HAL releases the flash from deep power-down.

	  Mutually exclusive with "Power down flash supply" above.

config ESP32_SLEEP_SPI_FLASH_ENTER_DPD_MODE_DELAY
	int "SPI flash enter deep power-down delay (us)"
	depends on ESP32_SLEEP_SET_FLASH_DPD
	default 25
	range 0 100
	help
	  Time CS must stay high after the enter-DPD command (tDP in datasheets).

config ESP32_SLEEP_SPI_FLASH_EXIT_DPD_MODE_DELAY
	int "SPI flash exit deep power-down delay (us)"
	depends on ESP32_SLEEP_SET_FLASH_DPD
	default 40
	range 0 100
	help
	  Time CS must stay high after the exit-DPD command (tRES1 in datasheets).

config ESP32_SLEEP_DCM_VSET_VAL_IN_SLEEP
	int "DCDC voltage parameter during sleep"
	depends on SOC_SERIES_ESP32P4
	default 14
	range 0 31
	help
	  Voltage of the DCDC converter during sleep. The same parameter value may
	  correspond to different voltages on different DCDC models, so adjust it
	  according to the external DCDC selected in your hardware.
	  Recommended value for TI TLV62569/TLV62569P: 14.

endmenu # Espressif PM Config

endif # SOC_FAMILY_ESPRESSIF_ESP32
