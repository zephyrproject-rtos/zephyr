# SPDX-FileCopyrightText: FoBE Studio
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tailscale_client_identity)

set(src_dir "${ZEPHYR_BASE}/subsys/net/lib/tailscale")
target_include_directories(app PRIVATE
  ${src_dir}
)
target_sources(app PRIVATE
  main.c
)
zephyr_link_libraries(-Wl,--wrap=settings_save_one)
