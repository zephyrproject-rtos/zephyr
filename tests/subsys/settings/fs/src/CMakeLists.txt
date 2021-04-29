# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Nordic Semiconductor ASA

zephyr_library_named(settings_test_fs)

# zephyr_library() is here in "app-mode", see
# https://github.com/zephyrproject-rtos/zephyr/issues/19582
# Random build failures without this, depends on the number of threads.
add_dependencies(settings_test_fs
  zephyr_generated_headers
  )

FILE(GLOB fssources *.c )
zephyr_library_sources(${fssources})
zephyr_library_link_libraries(kernel)
