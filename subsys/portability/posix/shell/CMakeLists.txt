# Copyright (c) 2024 Meta
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
# For setenv() and unsetenv()
zephyr_library_compile_options(-U_POSIX_C_SOURCE -D_POSIX_C_SOURCE=200809L)
# For getenv_r() visibility
zephyr_library_compile_definitions(_BSD_SOURCE)

zephyr_library_sources_ifdef(CONFIG_POSIX_SHELL posix_shell.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_UNAME_SHELL uname.c)
zephyr_library_sources_ifdef(CONFIG_POSIX_ENV_SHELL env.c)
