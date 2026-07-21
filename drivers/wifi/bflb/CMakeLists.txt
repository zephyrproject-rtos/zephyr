# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

set(BFLB_HAL ${ZEPHYR_HAL_BOUFFALOLAB_MODULE_DIR})

zephyr_include_directories(
  ${BFLB_HAL}/include/bouffalolab/${SOC_SERIES}
  ${BFLB_HAL}/include/bouffalolab/${SOC_SERIES}/wifi
  ${BFLB_HAL}/include/bouffalolab/${SOC_SERIES}/rfparam
  ${CMAKE_CURRENT_SOURCE_DIR}
)

zephyr_library()
zephyr_library_link_libraries(hostap)

# SoC-specific driver sources.
add_subdirectory(bl61x)
