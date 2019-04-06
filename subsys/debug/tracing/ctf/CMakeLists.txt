# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_sources(ctf_top.c)

add_subdirectory_ifdef(CONFIG_TRACING_CTF_BOTTOM_POSIX bottoms/posix)
