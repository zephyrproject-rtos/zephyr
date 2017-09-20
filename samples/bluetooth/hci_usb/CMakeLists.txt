if(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})
else()
  set(BOARD quark_se_c1000_devboard)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_link_libraries(app subsys__bluetooth)

target_sources(app PRIVATE src/main.c)
