# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(common)

zephyr_sources(
  common/start.S
  common/soc_irq.S
)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/common/linker.ld CACHE INTERNAL "")
