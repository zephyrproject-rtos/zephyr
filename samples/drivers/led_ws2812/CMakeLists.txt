macro(set_conf_file)
  set(CONF_FILE prj.conf)

  if(EXISTS               ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    list(APPEND CONF_FILE ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
  endif()
endmacro()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
