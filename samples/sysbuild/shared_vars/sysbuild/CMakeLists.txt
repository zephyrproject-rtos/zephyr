# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Configure user shared variables.
# The variables with this name would be passed from sysbuild directly to the cmake call
# of the application and external projects.
list(APPEND shared_vars_USER_SHARED_VARS SHARE_ME_APP1 SHARE_ME_APP2)
list(APPEND remote_USER_SHARED_VARS SHARE_ME_REMOTE1 SHARE_ME_REMOTE2)

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sysbuild LANGUAGES)
