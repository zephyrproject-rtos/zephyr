# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if( CONFIG_SOC_SERIES_AM261X_R5F )
  zephyr_sources(r5/soc.c)

  zephyr_include_directories(r5)
  zephyr_include_directories(common)

  zephyr_sources(common/ctrl_partitions.c)

  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/r5/linker.ld CACHE INTERNAL "")

endif()
