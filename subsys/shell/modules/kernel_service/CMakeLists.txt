# Copyright (c) 2024 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  # Main command
  kernel_shell.c
  # Subcommand starts here
  cycles.c
  sleep.c
  uptime.c
  version.c
)

# Conditional subcommands
zephyr_sources_ifdef(CONFIG_SYS_HEAP_RUNTIME_STATS heap.c)

zephyr_sources_ifdef(CONFIG_LOG_RUNTIME_FILTERING log-level.c)

zephyr_sources_ifdef(CONFIG_REBOOT reboot.c)

add_subdirectory_ifdef(CONFIG_KERNEL_THREAD_SHELL thread)
