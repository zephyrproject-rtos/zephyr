# SPDX-FileCopyrightText: FoBE Studio
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tailscale_client_derp_worker)

zephyr_include_directories(${ZEPHYR_BASE}/tests/crypto/psa/include)

set(src_dir "${ZEPHYR_BASE}/subsys/net/lib/tailscale")
target_include_directories(app PRIVATE
  ${src_dir}
  ${ZEPHYR_BASE}/subsys/net/lib/wireguard/crypto
)
target_sources(app PRIVATE
  main.c
  ${src_dir}/transport.c
  ${src_dir}/control_noise.c
  ${src_dir}/disco_box.c
)
# Supply the transport configuration to this isolated worker fixture.
target_compile_definitions(app PRIVATE
  CONFIG_TAILSCALE_TRANSPORT=1
  CONFIG_TAILSCALE_CONTROL_HOST="control.invalid"
)
option(TAILSCALE_TEST_DISCO "Test direct UDP discovery" ON)
if(TAILSCALE_TEST_DISCO)
  target_compile_definitions(app PRIVATE CONFIG_TAILSCALE_DISCO=1)
  if(CONFIG_TAILSCALE_DERP_MAP)
    target_compile_definitions(app PRIVATE CONFIG_TAILSCALE_DISCOVER_ENDPOINT=1)
  endif()
endif()
if(CONFIG_TAILSCALE_DERP_MAP)
  target_compile_definitions(app PRIVATE CONFIG_TAILSCALE_DERP=1)
endif()
zephyr_link_libraries(-Wl,--wrap=wireguard_peer_receive_datagram)
