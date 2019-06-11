# SPDX-License-Identifier: Apache-2.0

# This makes the lower-level, CMake target name available in
# ${ZEPHYR_CURRENT_LIBRARY}. See /cmake/extensions.cmake.
zephyr_library_named(samples_bt_gatt)

# We use a number of include/generated/ .h files.
target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PRIVATE kernel)

zephyr_library_sources(bas.c cts.c hog.c hrs.c ipss.c hts.c)
