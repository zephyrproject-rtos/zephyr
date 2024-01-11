# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_NATIVE_EXTRA_CMDLINE_ARGS)
	zephyr_library()
	zephyr_library_sources(extra_args.c)
endif()
