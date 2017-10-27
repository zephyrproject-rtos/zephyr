set(QEMU_NET_STACK 1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if(CONFIG_BOARD_FRDM_K64F)
  include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)
endif()
