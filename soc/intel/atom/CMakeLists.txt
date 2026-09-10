# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2011-2014, Wind River Systems, Inc.

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_POWEROFF ../common/power.c)
set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
