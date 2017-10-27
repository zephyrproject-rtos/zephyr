include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)
set(QEMU_EXTRA_FLAGS -serial unix:/tmp/bt-server-bredr -s)

zephyr_library_include_directories($ENV{ZEPHYR_BASE}/samples/bluetooth)
target_sources(app PRIVATE src/main.c)
if(CONFIG_BOARD_BBC_MICROBIT)
target_sources(app PRIVATE src/microbit.c)
endif()
