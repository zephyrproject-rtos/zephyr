# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(critical_section_monitor.c)
zephyr_library_sources_ifdef(CONFIG_KERNEL_SHELL shell.c)

# Hooks execute in supervisor mode, including on userspace-enabled builds.
zephyr_library_compile_definitions(__ZEPHYR_SUPERVISOR__)
zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ARCH_DIR}/${ARCH}/include
)
