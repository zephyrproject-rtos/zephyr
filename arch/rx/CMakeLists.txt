# Copyright (c) 2020 KT-Elektronik, Klaucke und Partner GmbH
# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)
set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-rx-le) # needed for e.g. objcopy
