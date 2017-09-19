if(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})
else()
  set(BOARD qemu_x86)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/zoap-server.c)
target_sources_ifdef(CONFIG_NET_L2_IEEE802154 app PRIVATE src/ieee802154_settings.c)
