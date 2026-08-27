# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(src/answer.c)
# no_header.c has no SPDX tags of its own; its license/copyright come from
# REUSE.toml. Compiling it lets the SBOM tests verify REUSE.toml support.
zephyr_library_sources(src/no_header.c)
zephyr_include_directories(include)
