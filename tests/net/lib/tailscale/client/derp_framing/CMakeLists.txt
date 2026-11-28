# SPDX-FileCopyrightText: FoBE Studio
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tailscale_client_derp_framing)

zephyr_include_directories(${ZEPHYR_BASE}/tests/crypto/psa/include)

set(src_dir "${ZEPHYR_BASE}/subsys/net/lib/tailscale")
target_include_directories(app PRIVATE
  ${src_dir}
)
target_sources(app PRIVATE
  main.c
  control_h2.c
  ${src_dir}/control_h2.c
  ${src_dir}/derp_conn.c
  ${src_dir}/underlay_dns.c
)
target_compile_definitions(app PRIVATE CONFIG_TAILSCALE_CONTROL_HOST="control.invalid")
