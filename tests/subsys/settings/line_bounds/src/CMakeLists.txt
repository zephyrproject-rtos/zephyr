# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${ZEPHYR_BASE}/subsys/settings/src settings_line_bounds_bindir)

zephyr_include_directories(
  ${ZEPHYR_BASE}/subsys/settings/include
  ${ZEPHYR_BASE}/subsys/settings/src
  )

target_sources(app PRIVATE main.c)
