# Copyright (c) 2019 jan.van_winkel@dxplore.eu
# Copyright (c) 2020 Teslabs Engineering S.L.
# Copyright (c) 2023 Fabian Blatz <fabianblatz@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LVGL)

set(ZEPHYR_CURRENT_LIBRARY lvgl)
set(LVGL_DIR ${ZEPHYR_LVGL_MODULE_DIR})

zephyr_interface_library_named(LVGL)
zephyr_library()

zephyr_include_directories(${LVGL_DIR}/src/)
zephyr_include_directories(include)

zephyr_compile_definitions(LV_CONF_INCLUDE_SIMPLE=1)
zephyr_compile_definitions(LV_CONF_PATH=${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h)

zephyr_library_sources(
    ${LVGL_DIR}/src/core/lv_disp.c
    ${LVGL_DIR}/src/core/lv_event.c
    ${LVGL_DIR}/src/core/lv_group.c
    ${LVGL_DIR}/src/core/lv_indev.c
    ${LVGL_DIR}/src/core/lv_indev_scroll.c
    ${LVGL_DIR}/src/core/lv_obj.c
    ${LVGL_DIR}/src/core/lv_obj_class.c
    ${LVGL_DIR}/src/core/lv_obj_draw.c
    ${LVGL_DIR}/src/core/lv_obj_pos.c
    ${LVGL_DIR}/src/core/lv_obj_scroll.c
    ${LVGL_DIR}/src/core/lv_obj_style.c
    ${LVGL_DIR}/src/core/lv_obj_style_gen.c
    ${LVGL_DIR}/src/core/lv_obj_tree.c
    ${LVGL_DIR}/src/core/lv_refr.c
    ${LVGL_DIR}/src/core/lv_theme.c

    ${LVGL_DIR}/src/draw/arm2d/lv_gpu_arm2d.c
    ${LVGL_DIR}/src/draw/lv_draw_arc.c
    ${LVGL_DIR}/src/draw/lv_draw.c
    ${LVGL_DIR}/src/draw/lv_draw_img.c
    ${LVGL_DIR}/src/draw/lv_draw_label.c
    ${LVGL_DIR}/src/draw/lv_draw_layer.c
    ${LVGL_DIR}/src/draw/lv_draw_line.c
    ${LVGL_DIR}/src/draw/lv_draw_mask.c
    ${LVGL_DIR}/src/draw/lv_draw_rect.c
    ${LVGL_DIR}/src/draw/lv_draw_transform.c
    ${LVGL_DIR}/src/draw/lv_draw_triangle.c
    ${LVGL_DIR}/src/draw/lv_img_buf.c
    ${LVGL_DIR}/src/draw/lv_img_cache.c
    ${LVGL_DIR}/src/draw/lv_img_decoder.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_draw_pxp_blend.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_draw_pxp.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_gpu_nxp_pxp.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_gpu_nxp_pxp_osa.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_arc.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_blend.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_line.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_rect.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_vglite_buf.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_vglite_utils.c
    ${LVGL_DIR}/src/draw/renesas/lv_gpu_d2_draw_label.c
    ${LVGL_DIR}/src/draw/renesas/lv_gpu_d2_ra6m3.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_arc.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_bg.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_composite.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_img.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_label.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_layer.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_line.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_mask.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_polygon.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_rect.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_stack_blur.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_texture_cache.c
    ${LVGL_DIR}/src/draw/sdl/lv_draw_sdl_utils.c
    ${LVGL_DIR}/src/draw/stm32_dma2d/lv_gpu_stm32_dma2d.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_arc.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_blend.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_dither.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_gradient.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_img.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_layer.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_letter.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_line.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_polygon.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_rect.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_transform.c
    ${LVGL_DIR}/src/draw/swm341_dma2d/lv_gpu_swm341_dma2d.c

    ${LVGL_DIR}/src/extra/layouts/flex/lv_flex.c
    ${LVGL_DIR}/src/extra/layouts/grid/lv_grid.c
    ${LVGL_DIR}/src/extra/libs/bmp/lv_bmp.c
    ${LVGL_DIR}/src/extra/libs/ffmpeg/lv_ffmpeg.c
    ${LVGL_DIR}/src/extra/libs/freetype/lv_freetype.c
    ${LVGL_DIR}/src/extra/libs/fsdrv/lv_fs_fatfs.c
    ${LVGL_DIR}/src/extra/libs/fsdrv/lv_fs_littlefs.c
    ${LVGL_DIR}/src/extra/libs/fsdrv/lv_fs_posix.c
    ${LVGL_DIR}/src/extra/libs/fsdrv/lv_fs_stdio.c
    ${LVGL_DIR}/src/extra/libs/fsdrv/lv_fs_win32.c
    ${LVGL_DIR}/src/extra/libs/gif/gifdec.c
    ${LVGL_DIR}/src/extra/libs/gif/lv_gif.c
    ${LVGL_DIR}/src/extra/libs/png/lodepng.c
    ${LVGL_DIR}/src/extra/libs/png/lv_png.c
    ${LVGL_DIR}/src/extra/libs/qrcode/lv_qrcode.c
    ${LVGL_DIR}/src/extra/libs/qrcode/qrcodegen.c
    ${LVGL_DIR}/src/extra/libs/rlottie/lv_rlottie.c
    ${LVGL_DIR}/src/extra/libs/sjpg/lv_sjpg.c
    ${LVGL_DIR}/src/extra/libs/sjpg/tjpgd.c
    ${LVGL_DIR}/src/extra/libs/tiny_ttf/lv_tiny_ttf.c
    ${LVGL_DIR}/src/extra/lv_extra.c
    ${LVGL_DIR}/src/extra/others/fragment/lv_fragment.c
    ${LVGL_DIR}/src/extra/others/fragment/lv_fragment_manager.c
    ${LVGL_DIR}/src/extra/others/gridnav/lv_gridnav.c
    ${LVGL_DIR}/src/extra/others/ime/lv_ime_pinyin.c
    ${LVGL_DIR}/src/extra/others/imgfont/lv_imgfont.c
    ${LVGL_DIR}/src/extra/others/monkey/lv_monkey.c
    ${LVGL_DIR}/src/extra/others/msg/lv_msg.c
    ${LVGL_DIR}/src/extra/others/snapshot/lv_snapshot.c
    ${LVGL_DIR}/src/extra/themes/basic/lv_theme_basic.c
    ${LVGL_DIR}/src/extra/themes/default/lv_theme_default.c
    ${LVGL_DIR}/src/extra/themes/mono/lv_theme_mono.c
    ${LVGL_DIR}/src/extra/widgets/animimg/lv_animimg.c
    ${LVGL_DIR}/src/extra/widgets/calendar/lv_calendar.c
    ${LVGL_DIR}/src/extra/widgets/calendar/lv_calendar_header_arrow.c
    ${LVGL_DIR}/src/extra/widgets/calendar/lv_calendar_header_dropdown.c
    ${LVGL_DIR}/src/extra/widgets/chart/lv_chart.c
    ${LVGL_DIR}/src/extra/widgets/colorwheel/lv_colorwheel.c
    ${LVGL_DIR}/src/extra/widgets/imgbtn/lv_imgbtn.c
    ${LVGL_DIR}/src/extra/widgets/keyboard/lv_keyboard.c
    ${LVGL_DIR}/src/extra/widgets/led/lv_led.c
    ${LVGL_DIR}/src/extra/widgets/list/lv_list.c
    ${LVGL_DIR}/src/extra/widgets/menu/lv_menu.c
    ${LVGL_DIR}/src/extra/widgets/meter/lv_meter.c
    ${LVGL_DIR}/src/extra/widgets/msgbox/lv_msgbox.c
    ${LVGL_DIR}/src/extra/widgets/span/lv_span.c
    ${LVGL_DIR}/src/extra/widgets/spinbox/lv_spinbox.c
    ${LVGL_DIR}/src/extra/widgets/spinner/lv_spinner.c
    ${LVGL_DIR}/src/extra/widgets/tabview/lv_tabview.c
    ${LVGL_DIR}/src/extra/widgets/tileview/lv_tileview.c
    ${LVGL_DIR}/src/extra/widgets/win/lv_win.c

    ${LVGL_DIR}/src/font/lv_font.c
    ${LVGL_DIR}/src/font/lv_font_dejavu_16_persian_hebrew.c
    ${LVGL_DIR}/src/font/lv_font_fmt_txt.c
    ${LVGL_DIR}/src/font/lv_font_loader.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_10.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_12.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_12_subpx.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_14.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_16.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_18.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_20.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_22.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_24.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_26.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_28.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_28_compressed.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_30.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_32.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_34.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_36.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_38.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_40.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_42.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_44.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_46.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_48.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_8.c
    ${LVGL_DIR}/src/font/lv_font_simsun_16_cjk.c
    ${LVGL_DIR}/src/font/lv_font_unscii_16.c
    ${LVGL_DIR}/src/font/lv_font_unscii_8.c

    ${LVGL_DIR}/src/hal/lv_hal_disp.c
    ${LVGL_DIR}/src/hal/lv_hal_indev.c
    ${LVGL_DIR}/src/hal/lv_hal_tick.c

    ${LVGL_DIR}/src/misc/lv_anim.c
    ${LVGL_DIR}/src/misc/lv_anim_timeline.c
    ${LVGL_DIR}/src/misc/lv_area.c
    ${LVGL_DIR}/src/misc/lv_async.c
    ${LVGL_DIR}/src/misc/lv_bidi.c
    ${LVGL_DIR}/src/misc/lv_color.c
    ${LVGL_DIR}/src/misc/lv_fs.c
    ${LVGL_DIR}/src/misc/lv_gc.c
    ${LVGL_DIR}/src/misc/lv_ll.c
    ${LVGL_DIR}/src/misc/lv_log.c
    ${LVGL_DIR}/src/misc/lv_lru.c
    ${LVGL_DIR}/src/misc/lv_math.c
    ${LVGL_DIR}/src/misc/lv_mem.c
    ${LVGL_DIR}/src/misc/lv_printf.c
    ${LVGL_DIR}/src/misc/lv_style.c
    ${LVGL_DIR}/src/misc/lv_style_gen.c
    ${LVGL_DIR}/src/misc/lv_templ.c
    ${LVGL_DIR}/src/misc/lv_timer.c
    ${LVGL_DIR}/src/misc/lv_tlsf.c
    ${LVGL_DIR}/src/misc/lv_txt_ap.c
    ${LVGL_DIR}/src/misc/lv_txt.c
    ${LVGL_DIR}/src/misc/lv_utils.c

    ${LVGL_DIR}/src/widgets/lv_arc.c
    ${LVGL_DIR}/src/widgets/lv_bar.c
    ${LVGL_DIR}/src/widgets/lv_btn.c
    ${LVGL_DIR}/src/widgets/lv_btnmatrix.c
    ${LVGL_DIR}/src/widgets/lv_canvas.c
    ${LVGL_DIR}/src/widgets/lv_checkbox.c
    ${LVGL_DIR}/src/widgets/lv_dropdown.c
    ${LVGL_DIR}/src/widgets/lv_img.c
    ${LVGL_DIR}/src/widgets/lv_label.c
    ${LVGL_DIR}/src/widgets/lv_line.c
    ${LVGL_DIR}/src/widgets/lv_objx_templ.c
    ${LVGL_DIR}/src/widgets/lv_roller.c
    ${LVGL_DIR}/src/widgets/lv_slider.c
    ${LVGL_DIR}/src/widgets/lv_switch.c
    ${LVGL_DIR}/src/widgets/lv_table.c
    ${LVGL_DIR}/src/widgets/lv_textarea.c

    lvgl.c
    lvgl_display.c
    lvgl_display_mono.c
    lvgl_display_16bit.c
    lvgl_display_24bit.c
    lvgl_display_32bit.c
    lvgl_display_mono.c
)

zephyr_library_sources_ifdef(CONFIG_LV_Z_USE_FILESYSTEM lvgl_fs.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_MEM_POOL_SYS_HEAP lvgl_mem.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_SHELL      lvgl_shell.c)

zephyr_library_sources(input/lvgl_common_input.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_POINTER_INPUT input/lvgl_pointer_input.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_BUTTON_INPUT input/lvgl_button_input.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_ENCODER_INPUT input/lvgl_encoder_input.c)
zephyr_library_sources_ifdef(CONFIG_LV_Z_KEYPAD_INPUT input/lvgl_keypad_input.c)

zephyr_library_link_libraries(LVGL)
target_link_libraries(LVGL INTERFACE zephyr_interface)

endif()
