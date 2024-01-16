# Copyright (c) 2024 Meta
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_POSIX_UNAME_SHELL uname.c)
