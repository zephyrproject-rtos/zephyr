if(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})
else()
  set(BOARD nrf52_pca10040)
endif()

if(BOARD EQUAL bbc_microbit)
  set(CONF_FILE microbit.conf)
else()
  set(CONF_FILE nrf5.conf)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_link_libraries(app subsys__bluetooth)

target_sources(app PRIVATE src/main.c)
