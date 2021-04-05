# Copyright (c) 2021 Emil Lindqvist
# SPDX-License-Identifier: Apache-2.0


zephyr_include_directories(.)
zephyr_library_sources_ifdef(CONFIG_MBEDTLS_BUILTIN mbedts_init.c)
zephyr_library_sources_ifdef(CONFIG_MBEDTLS_DEBUG mbedtls_debug.c)
