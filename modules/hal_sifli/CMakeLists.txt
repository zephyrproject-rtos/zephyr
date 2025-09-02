# Copyright (c) 2025 Core Devices LLC
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_SIFLI_HAL)
  # Allow using CMSIS headers from HAL without compiling HAL library.
  # This gives us access to register definitions and CMSIS HAL glue code.
  zephyr_compile_definitions(SOC_BF0_HCPU)
  if(CONFIG_SOC_SERIES_SF32LB52X)
    zephyr_compile_definitions(SF32LB52X)
  endif()

  zephyr_include_directories(${ZEPHYR_HAL_SIFLI_MODULE_DIR}/cmsis/Include)
  if(CONFIG_SOC_SERIES_SF32LB52X)
    zephyr_include_directories(${ZEPHYR_HAL_SIFLI_MODULE_DIR}/cmsis/sf32lb52x)
  endif()

  if(CONFIG_USE_SIFLI_HAL)
    zephyr_library_named(hal_sifli)

    zephyr_library_compile_definitions(
      NONE_HAL_TICK_INIT
      HAL_TICK_PER_SECOND=${CONFIG_SYS_CLOCK_TICKS_PER_SEC}
    )

    zephyr_library_include_directories(${CMAKE_CURRENT_LIST_DIR})
    zephyr_include_directories(${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/include)

    zephyr_library_sources(
      ${CMAKE_CURRENT_LIST_DIR}/hal.c
      ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal.c
      ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_cortex.c
      ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_efuse.c
      ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_pmu.c
      ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_rcc.c
    )
    if(CONFIG_SOC_SERIES_SF32LB52X)
      zephyr_library_sources(
        ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/cmsis/sf32lb52x/bf0_pin_const.c
      )
    endif()

    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_ADC ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_adc.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_AON ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_hpaon.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_ATIM ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_tim_ex.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_AUDCODEC ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_audcodec.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_AUDPRC ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_audprc.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_BUSMON ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_busmon.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_CRC ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_crc.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_DMA ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_dma.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_EPIC ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_epic.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_EZIP ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_ezip.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_EXTDMA ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_ext_dma.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_GPIO ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_gpio.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_GPT ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_tim.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_HCD ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_hcd.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_I2C ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_i2c.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_I2S ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_i2s.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_LCD ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_lcdc.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_LCPU_PATCH ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_patch.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_LPTIM ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_lptim.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_MAILBOX ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_mailbox.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_MPI
        ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_mpi.c
        ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_mpi_ex.c
        ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_mpi_psram.c
        ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/flash_table.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_PCD ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_pcd.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_PDM ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_pdm.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_PINMUX ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_pinmux.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_PTC ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_ptc.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_RNG ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_rng.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_RTC ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_rtc.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_SD ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_sdmmc.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_SDHCI ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_sdhci.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_SECU ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_secu.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_SPI ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_spi.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_SYSTEM_CONFIG ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_sys_cfg.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_TSEN ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_tsen.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_UART ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_uart.c
    )
    zephyr_library_sources_ifdef(
      CONFIG_USE_SIFLI_HAL_WDT ${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_wdt.c
    )

    if(CONFIG_USE_SIFLI_HAL_AES OR CONFIG_USE_SIFLI_HAL_HASH)
      zephyr_library_sources(${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_aes.c)
    endif()

    if(CONFIG_USE_SIFLI_HAL_SD OR CONFIG_USE_SIFLI_HAL_SDHCI)
      zephyr_library_sources(${ZEPHYR_HAL_SIFLI_MODULE_DIR}/hal/src/bf0_hal_sd_ex.c)
    endif()
  endif()
endif()
