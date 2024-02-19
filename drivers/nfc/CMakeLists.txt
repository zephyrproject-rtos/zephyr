#
# Copyright (c) 2023 Sendrato
#
# SPDX-License-Identifier: Apache-2.0
#

if (CONFIG_NFC_NT3H2X11)
    zephyr_sources(nt3h2x11.c)
endif()

if (CONFIG_NFC_NRFX)
    zephyr_sources(nrfxnfc.c)
endif()
