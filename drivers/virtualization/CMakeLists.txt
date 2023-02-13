# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_IVSHMEM		virt_ivshmem.c)
zephyr_library_sources_ifdef(CONFIG_IVSHMEM_SHELL	virt_ivshmem_shell.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE		virt_ivshmem_handlers.c)
