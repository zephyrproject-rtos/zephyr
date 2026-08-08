# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_BUILD_OUTPUT_UF2 touch1200reset.c)
