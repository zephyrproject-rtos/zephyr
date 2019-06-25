# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SOF sof.c)
zephyr_library_link_libraries(SOF)
