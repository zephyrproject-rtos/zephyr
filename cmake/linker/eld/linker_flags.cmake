# Copyright (c) 2025 Qualcomm Technologies, Inc. and/or its subsidiaries.
#
# SPDX-License-Identifier: Apache-2.0

# eld is mostly flag-compatible with ld, so use ld's flags as a base.
include(${ZEPHYR_BASE}/cmake/linker/ld/linker_flags.cmake)

# Ensure LLEXT extension modules also use eld for partial linking.
list(APPEND LLEXT_APPEND_FLAGS -fuse-ld=eld)

set_property(TARGET linker PROPERTY no_position_independent "${LINKERFLAGPREFIX},-no-pie")

# ld sets gcc-specific flags, so clear this out.
set_property(TARGET linker PROPERTY lto_arguments)
set_property(TARGET linker PROPERTY lto_arguments_st)
