# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_qspi_nor)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if(CONFIG_TEST_SOC_FLASH_NRF_PREEMPT)
  target_sources(app PRIVATE src/nrf/soc_flash_nrf.c)
  target_link_options(app PUBLIC
    -Wl,--wrap=nrfx_nvmc_word_write
    -Wl,--wrap=nrfx_nvmc_page_erase
  )
endif()
