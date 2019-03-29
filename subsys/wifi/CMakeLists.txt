# SPDX-License-Identifier: Apache-2.0

add_library(subsys__wifi INTERFACE)

target_include_directories(subsys__wifi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_WIFIMGR_AP ap)
add_subdirectory_ifdef(CONFIG_WIFIMGR_STA sta)
add_subdirectory_ifdef(CONFIG_WIFIMGR_SHELL shell)

zephyr_sources_ifdef(CONFIG_WIFIMGR wifimgr.c)

target_link_libraries(subsys__wifi INTERFACE zephyr_interface)
