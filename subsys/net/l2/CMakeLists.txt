# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NET_L2_VIRTUAL)
  add_subdirectory(virtual)
endif()

if(CONFIG_NET_L2_DUMMY)
  add_subdirectory(dummy)
endif()

if (CONFIG_NET_OFFLOAD OR CONFIG_NET_SOCKETS_OFFLOAD)
  add_subdirectory(offloaded_netdev)
endif()

if(CONFIG_NET_L2_ETHERNET)
  add_subdirectory(ethernet)
endif()

if(CONFIG_NET_L2_PPP)
  add_subdirectory(ppp)
endif()

if(CONFIG_NET_L2_IEEE802154)
  add_subdirectory(ieee802154)
endif()

if(CONFIG_NET_L2_OPENTHREAD_IMPLEMENTATION_ZEPHYR)
  add_subdirectory(openthread)
endif()

if(CONFIG_NET_L2_WIFI_MGMT OR CONFIG_NET_L2_WIFI_SHELL)
  add_subdirectory(wifi)
endif()

if(CONFIG_NET_L2_CANBUS_RAW)
  add_subdirectory(canbus)
endif()
