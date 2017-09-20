if(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})
else()
  set(BOARD quark_se_c1000_devboard)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/subsys/net/ip)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
