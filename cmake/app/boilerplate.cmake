
# This file must be included into the toplevel CMakeLists.txt file of
# Zephyr applications, e.g. zephyr/samples/hello_world/CMakeLists.txt
# must start with the line:
#
# include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake)
#
# It exists to reduce boilerplate code that Zephyr expects to be in
# application CMakeLists.txt code.
#
# Omitting it is permitted, but doing so incurs a maintenance cost as
# the application must manage upstream changes to this file.

add_library(app STATIC "")
target_link_libraries(app zephyr)

set(__build_dir ${CMAKE_CURRENT_BINARY_DIR}/zephyr)
add_subdirectory($ENV{ZEPHYR_BASE} ${__build_dir})

# To reduce boilerplate in application CMakeLists.txt code we choose
# to omit cmake_minimum_version()
cmake_policy(SET CMP0000 OLD)
