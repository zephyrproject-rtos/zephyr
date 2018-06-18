target_include_directories(subsys_net PRIVATE ${CMAKE_CURRENT_LIST_DIR})

if(CONFIG_NET_L2_BT OR CONFIG_NET_L2_BT_SHELL)
  include_relative(bluetooth/CMakeLists.txt)
endif()

include_relative_ifdef(CONFIG_NET_L2_DUMMY dummy/CMakeLists.txt)

include_relative_ifdef(CONFIG_NET_L2_ETHERNET ethernet/CMakeLists.txt)

include_relative_ifdef(CONFIG_NET_L2_IEEE802154 ieee802154/CMakeLists.txt)

include_relative_ifdef(CONFIG_NET_L2_OPENTHREAD openthread/CMakeLists.txt)

if(CONFIG_NET_L2_WIFI_MGMT OR CONFIG_NET_L2_WIFI_SHELL)
  include_relative(wifi/CMakeLists.txt)
endif()
