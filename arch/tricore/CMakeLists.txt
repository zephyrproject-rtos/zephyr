# Copyright (c) 2024 Infineon Technologies AG
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  zephyr_link_libraries(-Wl,--no-warn-rwx-segments)
endif()
