set(QEMU_NET_STACK 1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE src/main.c)

if(CONFIG_NET_L2_IEEE802154)
  target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/net/common)
endif()

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)
