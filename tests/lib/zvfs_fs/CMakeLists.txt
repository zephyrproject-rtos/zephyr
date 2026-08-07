# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zvfs_fs)

# white-box access to the ZVFS-private zvfs_fs_priv.h (zvfs_resolve_path())
zephyr_include_directories(${ZEPHYR_BASE}/lib/os/zvfs)

target_sources(app PRIVATE src/main.c)
