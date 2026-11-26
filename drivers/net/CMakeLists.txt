# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NET_DRIVERS)
  zephyr_library()

  zephyr_library_sources_ifdef(CONFIG_NET_LOOPBACK loopback.c)
  zephyr_library_sources_ifdef(CONFIG_NET_CANBUS canbus.c)

  if(CONFIG_NET_NATIVE)
    zephyr_library_sources_ifdef(CONFIG_SLIP slip/slip.c)
    zephyr_library_sources_ifdef(CONFIG_NET_PPP ppp/ppp.c)
  endif()

  if(CONFIG_NET_NATIVE_OFFLOADED_SOCKETS)
    zephyr_library_include_directories(
      ${ZEPHYR_BASE}/subsys/net/lib/sockets
    )
    zephyr_library_sources(nsos_netdb.c)
    zephyr_library_sources(nsos_sockets.c)
    target_compile_options(native_simulator BEFORE INTERFACE
      -I${BOARD_DIR}
      -DNSOS_EPOLL_WAIT_INTERVAL=${CONFIG_NET_NATIVE_OFFLOADED_SOCKETS_EPOLL_WAIT_INTERVAL}
    )
    target_sources(native_simulator INTERFACE nsos_adapt.c)
    target_sources(native_simulator INTERFACE nsos_netdb.c)
  endif()
endif()

add_subdirectory_ifdef(CONFIG_ETH_DRIVER ethernet)
add_subdirectory_ifdef(CONFIG_ETH_PHY_DRIVER phy)
add_subdirectory_ifdef(CONFIG_MDIO mdio)
