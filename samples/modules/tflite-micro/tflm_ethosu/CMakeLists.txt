# SPDX-FileCopyrightText: <text>Copyright 2021-2022, 2025 Arm Limited and/or its
# affiliates <open-source-office@arm.com></text>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tflm_ethosu_app)

# Choose model
target_include_directories(app PRIVATE src/models/keyword_spotting_cnn_small_int8)

target_sources(app PRIVATE src/main.cpp src/inference_process.cpp)

# Add platform specific linker source snippet
zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_MPS3 SECTIONS linker.ld)
zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_MPS4 SECTIONS linker.ld)

# Optional PMU integration
if(CONFIG_SAMPLE_TFLM_ETHOSU_PMU)
  target_sources(app PRIVATE src/ethosu_pmu_monitor.cpp)
  # Expose configurable PMU event selections via CMake cache.
  # Defaults are chosen per platform; users can override via -DETHOSU_PMU_EVENT_N=TOKEN
  if(CONFIG_ETHOS_U85_128 OR CONFIG_ETHOS_U85_256 OR CONFIG_ETHOS_U85_512 OR CONFIG_ETHOS_U85_1024 OR CONFIG_ETHOS_U85_2048)
    set(ETHOSU_PMU_EVENT_0 ETHOSU_PMU_SRAM_RD_DATA_BEAT_RECEIVED  CACHE STRING "PMU Event #0")
    set(ETHOSU_PMU_EVENT_1 ETHOSU_PMU_SRAM_WR_DATA_BEAT_WRITTEN   CACHE STRING "PMU Event #1")
    set(ETHOSU_PMU_EVENT_2 ETHOSU_PMU_EXT_RD_DATA_BEAT_RECEIVED   CACHE STRING "PMU Event #2")
    set(ETHOSU_PMU_EVENT_3 ETHOSU_PMU_EXT_WR_DATA_BEAT_WRITTEN    CACHE STRING "PMU Event #3")
    set(ETHOSU_PMU_EVENT_4 ETHOSU_PMU_SRAM0_RD_DATA_BEAT_RECEIVED CACHE STRING "PMU Event #4")
    set(ETHOSU_PMU_EVENT_5 ETHOSU_PMU_SRAM1_RD_DATA_BEAT_RECEIVED CACHE STRING "PMU Event #5")
    set(ETHOSU_PMU_EVENT_6 ETHOSU_PMU_SRAM2_RD_DATA_BEAT_RECEIVED CACHE STRING "PMU Event #6")
    set(ETHOSU_PMU_EVENT_7 ETHOSU_PMU_SRAM3_RD_DATA_BEAT_RECEIVED CACHE STRING "PMU Event #7")
    target_compile_definitions(app PRIVATE
      ETHOSU_PMU_EVENT_0=${ETHOSU_PMU_EVENT_0}
      ETHOSU_PMU_EVENT_1=${ETHOSU_PMU_EVENT_1}
      ETHOSU_PMU_EVENT_2=${ETHOSU_PMU_EVENT_2}
      ETHOSU_PMU_EVENT_3=${ETHOSU_PMU_EVENT_3}
      ETHOSU_PMU_EVENT_4=${ETHOSU_PMU_EVENT_4}
      ETHOSU_PMU_EVENT_5=${ETHOSU_PMU_EVENT_5}
      ETHOSU_PMU_EVENT_6=${ETHOSU_PMU_EVENT_6}
      ETHOSU_PMU_EVENT_7=${ETHOSU_PMU_EVENT_7}
    )
  else()
    set(ETHOSU_PMU_EVENT_0 ETHOSU_PMU_AXI0_RD_DATA_BEAT_RECEIVED CACHE STRING "PMU Event #0")
    set(ETHOSU_PMU_EVENT_1 ETHOSU_PMU_AXI0_WR_DATA_BEAT_WRITTEN CACHE STRING "PMU Event #1")
    set(ETHOSU_PMU_EVENT_2 ETHOSU_PMU_AXI1_RD_DATA_BEAT_RECEIVED  CACHE STRING "PMU Event #2")
    set(ETHOSU_PMU_EVENT_3 ETHOSU_PMU_AXI1_WR_DATA_BEAT_WRITTEN  CACHE STRING "PMU Event #3")
    target_compile_definitions(app PRIVATE
      ETHOSU_PMU_EVENT_0=${ETHOSU_PMU_EVENT_0}
      ETHOSU_PMU_EVENT_1=${ETHOSU_PMU_EVENT_1}
      ETHOSU_PMU_EVENT_2=${ETHOSU_PMU_EVENT_2}
      ETHOSU_PMU_EVENT_3=${ETHOSU_PMU_EVENT_3}
    )
  endif()
endif()
