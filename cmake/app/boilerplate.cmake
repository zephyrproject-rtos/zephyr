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

# app is a CMake library containing all the application code and is
# modified by the entry point ${APPLICATION_SOURCE_DIR}/CMakeLists.txt
# that was specified when cmake was called.

# To reduce boilerplate in application CMakeLists.txt code we choose
# to omit cmake_minimum_version()
cmake_minimum_required(VERSION 3.7)
cmake_policy(SET CMP0000 OLD)
cmake_policy(SET CMP0002 NEW)

include($ENV{ZEPHYR_BASE}/cmake/extensions.cmake)

zephyr_library_named(app)

set(__build_dir ${CMAKE_CURRENT_BINARY_DIR}/zephyr)
add_subdirectory($ENV{ZEPHYR_BASE} ${__build_dir})

define_property(GLOBAL PROPERTY ZEPHYR_LIBS
    BRIEF_DOCS "Global list of all Zephyr CMake libs that should be linked in"
    FULL_DOCS "Global list of all Zephyr CMake libs that should be linked in. zephyr_library() appends libs to this list.")
set_property(GLOBAL PROPERTY ZEPHYR_LIBS "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES
  BRIEF_DOCS "Object files that are generated after Zephyr has been linked once."
  FULL_DOCS "\
Object files that are generated after Zephyr has been linked once.\
May include mmu tables, etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES
  BRIEF_DOCS "Source files that are generated after Zephyr has been linked once."
  FULL_DOCS "\
Object files that are generated after Zephyr has been linked once.\
May include isr_tables.c etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES "")
