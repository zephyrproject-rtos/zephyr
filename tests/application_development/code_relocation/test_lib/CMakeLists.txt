# SPDX-License-Identifier: Apache-2.0
# Copyright 2022 NXP

add_library(test_lib STATIC "")
target_sources(test_lib PRIVATE test_lib1.c test_lib2.c)
get_target_property(include_dirs app INCLUDE_DIRECTORIES)
target_link_libraries(test_lib PUBLIC zephyr_interface)
