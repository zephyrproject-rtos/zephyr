# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(lvgl)

target_include_directories(lvgl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()

zephyr_library_sources(lvgl_display.c)
zephyr_library_sources(lvgl_display_mono.c)
zephyr_library_sources(lvgl_display_16bit.c)
zephyr_library_sources(lvgl_display_24bit.c)
zephyr_library_sources(lvgl_display_32bit.c)

zephyr_library_sources_ifdef( CONFIG_LVGL_FILESYSTEM
    lvgl_fs.c
)

zephyr_library_sources(lvgl.c)

zephyr_library_sources_ifdef( CONFIG_LVGL_MEM_POOL_USER lvgl_mem_user.c)

zephyr_library_sources_ifdef( CONFIG_LVGL_MEM_POOL_KERNEL lvgl_mem_kernel.c)

zephyr_library_link_libraries(lvgl)
