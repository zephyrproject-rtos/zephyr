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

# The cross clang auto-discovers hexagon-unknown-none-elf.cfg, which injects
# picolibc's crt0-semihost.o and picolibc.ld (the latter pulls in -lh2).
# Zephyr supplies its own startup and linker script and adds
# --no-default-config for the real build in arch/hexagon/CMakeLists.txt.
# The toolchain-capability probes (zephyr_check_compiler_flag) run earlier and
# use CMAKE_REQUIRED_FLAGS, so add the flag here too; otherwise the "dummy C
# file" link test fails with "unable to find library -lh2".
list(APPEND CMAKE_REQUIRED_FLAGS --no-default-config)
