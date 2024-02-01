# Copyright (c) 2024 Freedom Veiculos Eletricos
# Copyright (c) 2024 OS Systems
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(morse_code.c)
zephyr_library_sources_ifdef(CONFIG_MORSE_CODE_SHELL morse_code_shell.c)
