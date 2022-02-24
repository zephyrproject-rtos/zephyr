# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_CC1101		cc1101)

if(CONFIG_USERSPACE)
# The above if() is needed or else CMake would complain about
# empty library.

zephyr_library_sources_ifdef(CONFIG_USERSPACE rf_handlers.c)

endif()
