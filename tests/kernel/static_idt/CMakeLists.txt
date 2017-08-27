if(DEFINED ENV{BOARD})
  set(BOARD $ENV{BOARD})
else()
  set(BOARD qemu_x86)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
include($ENV{ZEPHYR_BASE}/cmake/test/CMakeLists.txt NO_POLICY_SCOPE)
project(NONE)


enable_language(C ASM)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources} src/test_stubs.S)
