# SPDX-License-Identifier: Apache-2.0

zephyr_sources(stm32cube_hal.c)

zephyr_linker_sources_ifdef(CONFIG_STM32_CCM SECTIONS ccm.ld)

zephyr_sources_ifdef(CONFIG_STM32_BACKUP_SRAM stm32_backup_sram.c)
zephyr_linker_sources_ifdef(CONFIG_STM32_BACKUP_SRAM SECTIONS stm32_backup_sram.ld)

zephyr_sources(soc_config.c)
if(CONFIG_PM)
	zephyr_sources_ifdef(CONFIG_DT_HAS_ST_STM32_PWR_ENABLED
		soc_pwr_config.c
	)
endif()
