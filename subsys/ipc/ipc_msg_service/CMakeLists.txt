# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ipc_msg_service.c)

add_subdirectory(backends)
