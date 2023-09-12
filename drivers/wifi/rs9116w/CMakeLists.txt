# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_RS9116W)
    zephyr_library_include_directories(
        ${ZEPHYR_BASE}/subsys/net/lib/tls_credentials
    )

    zephyr_library_sources(
        rs9116w_mgmt.c
        rs9116w_socket.c
    )
    zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_OFFLOAD rs9116w_socket_offload.c)
endif()
