# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ec_host_cmd_handler.c)

add_subdirectory(backends)
