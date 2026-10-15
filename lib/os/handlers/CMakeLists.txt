# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# System call verifiers moved out of lib/os. Like the kernel's handlers,
# this is a Zephyr library so that it is linked with --whole-archive: the
# system call table defines a weak fallback for every handler, and an
# archive member that only defines handlers would never be extracted to
# override them.
zephyr_library_named(os_syscall_handlers)
zephyr_library_compile_definitions(__ZEPHYR_SUPERVISOR__)
zephyr_library_sources(clock.c)
zephyr_library_sources_ifdef(CONFIG_PRINTK printk.c)
