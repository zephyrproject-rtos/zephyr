# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_SERIES_SIWG917 soc.c)
zephyr_sources_ifdef(CONFIG_SILABS_SIWX91X_NWP siwx91x_nwp.c)
zephyr_sources_ifdef(CONFIG_PM power.c)
zephyr_sources_ifdef(CONFIG_SIWX91X_FIRMWARE_UPGRADE_SHELL siwx91x_fota_shell.c)
zephyr_sources_ifdef(CONFIG_SIWX91X_VERSION_SHELL siwx91x_version_shell.c)
zephyr_sources_ifdef(CONFIG_SIWX91X_CLOCK_SHELL siwx91x_clock_shell.c)

# Optional wiring diagram from the build devicetree (see siwx91x_clock_tree_report.py).
if(CONFIG_CLOCK_CONTROL_SILABS_SIWX91X)
  add_custom_target(
    clock_tree
    COMMAND ${PYTHON_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/../siwx91x_clock_tree_report.py
            --zephyr-base ${ZEPHYR_BASE}
            --edt-pickle ${EDT_PICKLE}
    DEPENDS devicetree_target
    USES_TERMINAL
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
