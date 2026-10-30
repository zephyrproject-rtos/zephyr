# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)
# v850-elf GCC prefixes C symbols with "_"
set_property(TARGET compiler PROPERTY symbol_prefix _)

# Registers that need 32-bit access do not accept byte-sized SET1/CLR1/NOT1
zephyr_cc_option(-fno-fuse-ops-with-volatile-access)

zephyr_link_libraries($<TARGET_PROPERTY:linker,no_warn_rwx_segments>)
