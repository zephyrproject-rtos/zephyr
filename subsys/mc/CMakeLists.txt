# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_MC_COMM comm.c)
zephyr_library_sources_ifdef(CONFIG_MC_MSG msg.c)
