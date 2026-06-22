#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_UVB)

zephyr_library()
zephyr_library_sources(uvb.c)
zephyr_linker_sources(DATA_SECTIONS uvb.ld)

endif()
