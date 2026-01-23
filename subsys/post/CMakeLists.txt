# Copyright (c) 2026 Aerlync Labs Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  post_core.c
)

zephyr_library_sources_ifdef(CONFIG_POST_USERSPACE
  post_syscalls.c
)

zephyr_library_sources_ifdef(CONFIG_POST_SHELL
  post_shell.c
)
