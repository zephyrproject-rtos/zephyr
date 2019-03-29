# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(shell.c)

zephyr_library_link_libraries(subsys__wifi)
