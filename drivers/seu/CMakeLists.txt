# Copyright (c) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/seu/seu.h)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SEU_INTEL_SOCFPGA seu_intel_socfpga.c)
