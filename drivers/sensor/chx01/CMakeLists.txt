# Copyright (c) 2023 Google LLC
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(.)

add_subdirectory(chirpmicro)
zephyr_library_sources(
    chbsp_impl.c
)

zephyr_library_sources_ifdef(CONFIG_CH101 ch101.c)
