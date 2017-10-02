
if(BOARD STREQUAL bbc_microbit)
  set(CONF_FILE microbit.conf)
elseif(BOARD STREQUAL nrf51-16k.conf)
  set(CONF_FILE nrf51-16k.conf)
endif()



set(QEMU_EXTRA_FLAGS -serial unix:/tmp/bt-server-bredr -s)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_link_libraries(app subsys__bluetooth)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_BOARD_BBC_MICROBIT app PRIVATE src/microbit.c)
