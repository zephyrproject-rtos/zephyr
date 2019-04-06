# SPDX-License-Identifier: Apache-2.0

include(FetchContent)

set(lv_name lvgl)

set(ep_base ${PROJECT_BINARY_DIR}/ext_proj)
set_property(DIRECTORY PROPERTY "EP_BASE" ${ep_base})

set(lv_SOURCE_DIR ${ep_base}/Source/${lv_name})
set(lv_SUBBUILD_DIR ${ep_base}/Subbuild/${lv_name})

FetchContent_Declare(
    ${lv_name}
    GIT_REPOSITORY https://github.com/littlevgl/lvgl.git
    GIT_TAG        v5.3
    SOURCE_DIR     ${lv_SOURCE_DIR}
    BINARY_DIR     ${lv_SOURCE_DIR}
    SUBBUILD_DIR   ${lv_SUBBUILD_DIR}
)

FetchContent_GetProperties(${lv_name})
if(NOT ${lv_name}_POPULATED)
    FetchContent_Populate(${lv_name})
endif()

zephyr_interface_library_named(lvgl)

set(LVGL_SOURCE_DIR ${${lv_name}_SOURCE_DIR})

target_include_directories(lvgl INTERFACE ${LVGL_SOURCE_DIR})
target_include_directories(lvgl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)

zephyr_library()

zephyr_library_sources(

    ${LVGL_SOURCE_DIR}/lv_core/lv_group.c
    ${LVGL_SOURCE_DIR}/lv_core/lv_indev.c
    ${LVGL_SOURCE_DIR}/lv_core/lv_lang.c
    ${LVGL_SOURCE_DIR}/lv_core/lv_obj.c
    ${LVGL_SOURCE_DIR}/lv_core/lv_refr.c
    ${LVGL_SOURCE_DIR}/lv_core/lv_style.c
    ${LVGL_SOURCE_DIR}/lv_core/lv_vdb.c

    ${LVGL_SOURCE_DIR}/lv_draw/lv_draw.c
    ${LVGL_SOURCE_DIR}/lv_draw/lv_draw_arc.c
    ${LVGL_SOURCE_DIR}/lv_draw/lv_draw_img.c
    ${LVGL_SOURCE_DIR}/lv_draw/lv_draw_label.c
    ${LVGL_SOURCE_DIR}/lv_draw/lv_draw_line.c
    ${LVGL_SOURCE_DIR}/lv_draw/lv_draw_rbasic.c
    ${LVGL_SOURCE_DIR}/lv_draw/lv_draw_rect.c
    ${LVGL_SOURCE_DIR}/lv_draw/lv_draw_triangle.c
    ${LVGL_SOURCE_DIR}/lv_draw/lv_draw_vbasic.c

    ${LVGL_SOURCE_DIR}/lv_hal/lv_hal_disp.c
    ${LVGL_SOURCE_DIR}/lv_hal/lv_hal_indev.c
    ${LVGL_SOURCE_DIR}/lv_hal/lv_hal_tick.c

    ${LVGL_SOURCE_DIR}/lv_misc/lv_anim.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_area.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_circ.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_color.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_font.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_fs.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_gc.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_ll.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_log.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_math.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_mem.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_task.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_templ.c
    ${LVGL_SOURCE_DIR}/lv_misc/lv_txt.c

    ${LVGL_SOURCE_DIR}/lv_objx/lv_arc.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_bar.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_btn.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_btnm.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_calendar.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_canvas.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_cb.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_chart.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_cont.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_ddlist.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_gauge.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_img.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_imgbtn.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_kb.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_label.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_led.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_line.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_list.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_lmeter.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_mbox.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_objx_templ.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_page.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_preload.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_roller.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_slider.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_spinbox.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_sw.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_ta.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_table.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_tabview.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_tileview.c
    ${LVGL_SOURCE_DIR}/lv_objx/lv_win.c

    ${LVGL_SOURCE_DIR}/lv_themes/lv_theme.c
    ${LVGL_SOURCE_DIR}/lv_themes/lv_theme_alien.c
    ${LVGL_SOURCE_DIR}/lv_themes/lv_theme_default.c
    ${LVGL_SOURCE_DIR}/lv_themes/lv_theme_material.c
    ${LVGL_SOURCE_DIR}/lv_themes/lv_theme_mono.c
    ${LVGL_SOURCE_DIR}/lv_themes/lv_theme_nemo.c
    ${LVGL_SOURCE_DIR}/lv_themes/lv_theme_night.c
    ${LVGL_SOURCE_DIR}/lv_themes/lv_theme_templ.c
    ${LVGL_SOURCE_DIR}/lv_themes/lv_theme_zen.c

    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_builtin.c
)

zephyr_library_sources_ifdef( CONFIG_LVGL_BUILD_IN_FONT_10
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_10.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_10_cyrillic.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_10_latin_sup.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_symbol_10.c
)

zephyr_library_sources_ifdef( CONFIG_LVGL_BUILD_IN_FONT_20
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_20.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_20_cyrillic.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_20_latin_sup.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_symbol_20.c
)

zephyr_library_sources_ifdef( CONFIG_LVGL_BUILD_IN_FONT_30
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_30.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_30_cyrillic.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_30_latin_sup.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_symbol_30.c
)

zephyr_library_sources_ifdef( CONFIG_LVGL_BUILD_IN_FONT_40
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_40.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_40_cyrillic.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_dejavu_40_latin_sup.c
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_symbol_40.c
)

zephyr_library_sources_ifdef( CONFIG_LVGL_BUILD_IN_FONT_MONOSPACE
    ${LVGL_SOURCE_DIR}/lv_fonts/lv_font_monospace_8.c
)

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
