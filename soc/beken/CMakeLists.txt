# Copyright (c) 2026 Beken Corporation
# SPDX-License-Identifier: Apache-2.0

# CMake entry point for the Beken SoC family.
# Only the selected series is pulled in; anything else takes no part in the
# build.

add_subdirectory_ifdef(CONFIG_SOC_BK7258 bk7258)
