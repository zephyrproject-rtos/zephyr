# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(POSIX_VERSION 200809L)

if(CONFIG_POSIX_C_LANG_SUPPORT_R)
  zephyr_compile_definitions(-D_POSIX_THREAD_SAFE_FUNCTIONS=${POSIX_VERSION})
endif()
