# Copyright (c) 2024 Michael Hope
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)
add_subdirectory(${SOC_SERIES})

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/common/linker.ld CACHE INTERNAL "")
