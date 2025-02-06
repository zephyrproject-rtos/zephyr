# Copyright (c) 2024 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(../)

# Main command
zephyr_sources_ifdef(CONFIG_KERNEL_THREAD_SHELL thread.c)

# Subcommands
zephyr_sources_ifdef(CONFIG_KERNEL_THREAD_SHELL_LIST list.c)

zephyr_sources_ifdef(CONFIG_KERNEL_THREAD_SHELL_MASK mask.c)

zephyr_sources_ifdef(CONFIG_KERNEL_THREAD_SHELL_MASK pin.c)

zephyr_sources_ifdef(CONFIG_KERNEL_THREAD_SHELL_STACKS stacks.c)

zephyr_sources_ifdef(CONFIG_KERNEL_THREAD_SHELL_UNWIND unwind.c)

zephyr_sources_ifdef(CONFIG_KERNEL_THREAD_SHELL_SUSPEND suspend.c)

zephyr_sources_ifdef(CONFIG_KERNEL_THREAD_SHELL_RESUME resume.c)

zephyr_sources_ifdef(CONFIG_KERNEL_THREAD_SHELL_KILL kill.c)
