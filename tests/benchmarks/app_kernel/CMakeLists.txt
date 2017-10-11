if(BOARD STREQUAL "minnowboard")
  set(CONF_FILE prj_fp.conf)
else()
  set(CONF_FILE prj_no_fp.conf)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
