# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021 EPAM Systems

# Needed to separate definitions in common Xen headers
zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:-D__ASSEMBLY__>)

zephyr_library_sources(hypercall.S)
zephyr_library_sources(enlighten.c)
