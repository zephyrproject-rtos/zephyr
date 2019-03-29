# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ap.c)
zephyr_library_sources(sm.c)

zephyr_library_link_libraries(subsys__wifi)
