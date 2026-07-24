#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_MP_FS)
    zephyr_library_sources(
        mp_filesrc.c
        mp_filesink.c
    )
endif()
