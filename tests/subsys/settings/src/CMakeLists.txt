# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Intel Corporation

zephyr_library_named(settings_test)

# zephyr_library() is here in "app-mode", see
# https://github.com/zephyrproject-rtos/zephyr/issues/19582
# Random build failures without this, depends on the number of threads.
add_dependencies(    settings_test offsets_h)

FILE(GLOB mysources *.c )
zephyr_library_sources(${mysources})
