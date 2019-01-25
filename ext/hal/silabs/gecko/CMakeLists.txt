# Makefile - Gecko SDK
#
# Copyright (c) 2017, Christian Taedcke
#
# SPDX-License-Identifier: Apache-2.0
#

# Translate the SoC name and part number into the gecko device and cpu name
# respectively.
string(TOUPPER ${CONFIG_SOC_SERIES} SILABS_GECKO_DEVICE)

set(SILABS_GECKO_PART_NUMBER ${CONFIG_SOC_PART_NUMBER})

zephyr_include_directories(
  Device/SiliconLabs/${SILABS_GECKO_DEVICE}/Include
  emlib/inc
  )

# The gecko SDK uses the cpu name to include the matching device header.
# See Device/SiliconLabs/$(SILABS_GECKO_DEVICE)/Include/em_device.h for an example.
zephyr_compile_definitions(
  ${SILABS_GECKO_PART_NUMBER}
  )

zephyr_sources(                                    emlib/src/em_system.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_CMU          emlib/src/em_cmu.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_CORE         emlib/src/em_core.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_CRYOTIMER    emlib/src/em_cryotimer.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_EMU          emlib/src/em_emu.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_GPIO         emlib/src/em_gpio.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_I2C          emlib/src/em_i2c.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_LETIMER      emlib/src/em_letimer.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_LEUART       emlib/src/em_leuart.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_MSC          emlib/src/em_msc.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_PRS          emlib/src/em_prs.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_RMU          emlib/src/em_rmu.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_RTC          emlib/src/em_rtc.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_RTCC         emlib/src/em_rtcc.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_TIMER        emlib/src/em_timer.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_USART        emlib/src/em_usart.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_WDOG         emlib/src/em_wdog.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_EFM32WG     Device/SiliconLabs/EFM32WG/Source/system_efm32wg.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_EFR32FG1P   Device/SiliconLabs/EFR32FG1P/Source/system_efr32fg1p.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_EFM32HG     Device/SiliconLabs/EFM32HG/Source/system_efm32hg.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_EFR32MG12P  Device/SiliconLabs/EFR32MG12P/Source/system_efr32mg12p.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_EFM32PG12B  Device/SiliconLabs/EFM32PG12B/Source/system_efm32pg12b.c)
