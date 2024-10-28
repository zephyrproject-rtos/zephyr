# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

#
# Sysbuild default list of CMake modules to include in a regular sysbuild session.
#
include(extensions)
include(sysbuild_extensions)
include(python)
include(west)
include(yaml)
include(sysbuild_root)
include(zephyr_module)
include(boards)
include(shields)
include(hwm_v2)
include(sysbuild_kconfig)
include(native_simulator_sb_extensions)
include(sysbuild_images)
