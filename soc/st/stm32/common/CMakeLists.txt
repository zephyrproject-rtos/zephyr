# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
	stm32cube_hal.c
	soc_config.c
)

if(DEFINED CONFIG_STM32_CCM)
  zephyr_linker_group(NAME CCM_REGION VMA CCM LMA ROM_REGION)
  zephyr_linker_section(NAME .ccm_bss GROUP CCM_REGION SUBALIGN 4 TYPE BSS )
  zephyr_linker_section(NAME .ccm_noinit GROUP CCM_REGION SUBALIGN 4 TYPE NOLOAD NOINIT)
  zephyr_linker_section(NAME .ccm_data GROUP CCM_REGION SUBALIGN 4)

  zephyr_linker_sources(SECTIONS ccm.ld)
endif()

zephyr_sources_ifdef(CONFIG_STM32_BACKUP_SRAM stm32_backup_sram.c)
zephyr_linker_sources_ifdef(CONFIG_STM32_BACKUP_SRAM SECTIONS stm32_backup_sram.ld)

if (NOT CONFIG_DEBUG AND CONFIG_PM)
  zephyr_sources_ifdef(CONFIG_DT_HAS_SWJ_CONNECTOR_ENABLED pm_debug_swj.c)
endif()

if (CONFIG_STM32_WKUP_PINS)
  zephyr_sources(stm32_wkup_pins.c)
endif()

if (CONFIG_PM AND CONFIG_STM32_ENABLE_DEBUG_SLEEP_STOP)
  message(WARNING "\
SoC Power Management (CONFIG_PM) enabled but the DBGMCU is still enabled \
(CONFIG_STM32_ENABLE_DEBUG_SLEEP_STOP). The SoC will use more power than expected \
in STOP modes due to internal oscillators that remain active.")
endif()
