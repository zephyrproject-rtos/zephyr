if(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})
else()
  set(BOARD qemu_x86)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

set(QEMU_EXTRA_FLAGS -serial unix:/tmp/bt-server-bredr)

target_sources(app PRIVATE
  src/main.c
  src/hrs.c
  src/dis.c
  src/bas.c
  src/cts.c
)

zephyr_library_include_directories(PRIVATE $ENV{ZEPHYR_BASE}/samples/bluetooth)
