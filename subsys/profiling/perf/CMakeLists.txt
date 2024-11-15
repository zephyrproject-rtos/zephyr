# Copyright (c) 2023 KNS Group LLC (YADRO)
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(backends)

zephyr_library()

zephyr_library_sources(
  perf.c
)
