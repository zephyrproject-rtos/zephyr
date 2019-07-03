# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(lvgl)

target_include_directories(lvgl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()

zephyr_library_sources_ifdef( CONFIG_LVGL_COLOR_DEPTH_32
    lvgl_color_32.c
)

zephyr_library_sources_ifdef( CONFIG_LVGL_COLOR_DEPTH_16
    lvgl_color_16.c
)

zephyr_library_sources_ifdef( CONFIG_LVGL_COLOR_DEPTH_8
    lvgl_color_8.c
)

zephyr_library_sources_ifdef( CONFIG_LVGL_COLOR_DEPTH_1
    lvgl_color_1.c
)

zephyr_library_sources_ifdef( CONFIG_LVGL_FILESYSTEM
    lvgl_fs.c
)

zephyr_library_sources(lvgl.c)

zephyr_library_sources_ifdef( CONFIG_LVGL_MEM_POOL_USER lvgl_mem_user.c)

zephyr_library_sources_ifdef( CONFIG_LVGL_MEM_POOL_KERNEL lvgl_mem_kernel.c)

zephyr_library_link_libraries(lvgl)

target_link_libraries(lvgl INTERFACE zephyr_interface)
