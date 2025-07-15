# SPDX-License-Identifier: Apache-2.0
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

# Hexagon-specific compiler flags

# Disable small data section optimization to prevent duplicate .CONST_* symbols
# The -G0 flag tells the compiler not to use small data sections, which can
# cause duplicate symbol errors when the same constants are used in multiple
# compilation units.
list(APPEND TOOLCHAIN_C_FLAGS -G0)
list(APPEND TOOLCHAIN_CXX_FLAGS -G0)
list(APPEND TOOLCHAIN_LD_FLAGS -G0)
