# Add an absolute directory path to the CMake variable
# SYSCALL_INCLUDE_DIRS. This ensures that the syscall machinery will
# be able to find application-defined syscalls.
list(APPEND SYSCALL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE
  src/main.c
  src/hello_world_driver.c
  )
