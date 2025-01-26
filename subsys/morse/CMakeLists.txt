# Copyright (c) 2024-2025 Freedom Veiculos Eletricos
# Copyright (c) 2024-2025 O.S. Systems Software LTDA.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MORSE)

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/morse/morse_device.h)

zephyr_library()

zephyr_library_sources(morse.c)
zephyr_library_sources_ifdef(CONFIG_MORSE_SHELL morse_shell.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE   morse_handlers.c)

add_subdirectory(backends)

endif()
