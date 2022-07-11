# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_RTIO)
	zephyr_library()

	zephyr_include_directories(${ZEPHYR_BASE}/subsys/rtio)

	zephyr_library_sources_ifdef(
		CONFIG_RTIO_EXECUTOR_SIMPLE
		rtio_executor_simple.c
	)

	zephyr_library_sources_ifdef(
		CONFIG_RTIO_EXECUTOR_CONCURRENT
		rtio_executor_concurrent.c
	)

endif()
