# Copyright (c) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_IO96B_INTEL_SOCFPGA io96b.c)

zephyr_library_sources_ifdef(CONFIG_SHELL_IO96B io96b_shell.c)
