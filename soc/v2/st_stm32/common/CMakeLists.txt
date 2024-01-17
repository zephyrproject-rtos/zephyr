# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
	stm32cube_hal.c
	soc_config.c
)

zephyr_linker_sources_ifdef(CONFIG_STM32_CCM SECTIONS ccm.ld)

zephyr_sources_ifdef(CONFIG_STM32_BACKUP_SRAM stm32_backup_sram.c)
zephyr_linker_sources_ifdef(CONFIG_STM32_BACKUP_SRAM SECTIONS stm32_backup_sram.ld)
