# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_include_directories(.)

zephyr_library_sources(
  ssh_transport.c
  ssh_kex.c
  ssh_kex_ecdh.c
  ssh_auth.c
  ssh_connection.c
  ssh_pkt.c
  ssh_host_key.c
)
zephyr_library_sources_ifdef(CONFIG_SSH_SERVER ssh_server.c)
zephyr_library_sources_ifdef(CONFIG_SSH_CLIENT ssh_client.c)

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
