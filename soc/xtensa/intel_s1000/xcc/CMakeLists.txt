# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_CPLUSPLUS cpp_fixes.c)
zephyr_library_sources_ifdef(CONFIG_NEWLIB_LIBC newlib_fixes.c)

if(NOT (CC STREQUAL "clang"))
# These two are due to IS_ENABLED() not being parsed correctly
# in old GCC 4.2 based XCC. Clang is not affected.
zephyr_library_sources_ifdef(CONFIG_LOG_MINIMAL log_minimal_fixes.c)
zephyr_library_sources_ifdef(CONFIG_TEST ztest_fixes.c)
endif()
