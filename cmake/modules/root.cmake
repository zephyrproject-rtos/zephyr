# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Convert Zephyr roots to absolute paths.
#
# This CMake module will convert all relative paths in existing ROOT lists to
# absolute path relative from APPLICATION_SOURCE_DIR.
#
# Optional variables:
# - ARCH_ROOT:       CMake list of arch roots containing arch implementations
# - SOC_ROOT:        CMake list of SoC roots containing SoC implementations
# - BOARD_ROOT:      CMake list of board roots containing board and shield implementations
# - MODULE_EXT_ROOT: CMake list of module external roots containing module glue code
#
# If a root is defined it will check the list of paths in the root and convert
# any relative path to absolute path and update the root list.
# If a root is undefined it will still be undefined when this module has loaded.

include_guard(GLOBAL)

include(extensions)

# Convert paths to absolute, relative from APPLICATION_SOURCE_DIR
zephyr_file(APPLICATION_ROOT MODULE_EXT_ROOT)

# Convert paths to absolute, relative from APPLICATION_SOURCE_DIR
zephyr_file(APPLICATION_ROOT BOARD_ROOT)

# Convert paths to absolute, relative from APPLICATION_SOURCE_DIR
zephyr_file(APPLICATION_ROOT SOC_ROOT)

# Convert paths to absolute, relative from APPLICATION_SOURCE_DIR
zephyr_file(APPLICATION_ROOT ARCH_ROOT)

# Convert paths to absolute, relative from APPLICATION_SOURCE_DIR
zephyr_file(APPLICATION_ROOT SCA_ROOT)

if(unittest IN_LIST Zephyr_FIND_COMPONENTS)
  # Zephyr used in unittest mode, use dedicated unittest root.
  set(BOARD_ROOT ${ZEPHYR_BASE}/subsys/testsuite)
  set(ARCH_ROOT  ${ZEPHYR_BASE}/subsys/testsuite)
  set(SOC_ROOT   ${ZEPHYR_BASE}/subsys/testsuite)
endif()
