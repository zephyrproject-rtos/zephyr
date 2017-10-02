if(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})
else()
  set(BOARD qemu_x86)
endif()

set(CONF_FILE prj_${BOARD}.conf)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)


FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
