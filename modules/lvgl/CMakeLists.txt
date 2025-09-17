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
zephyr_library_compile_definitions(_POSIX_C_SOURCE=200809L)
zephyr_compile_definitions(LV_CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h")

zephyr_library_sources(

    ${LVGL_DIR}/src/core/lv_group.c
    ${LVGL_DIR}/src/core/lv_obj.c
    ${LVGL_DIR}/src/core/lv_obj_class.c
    ${LVGL_DIR}/src/core/lv_obj_draw.c
    ${LVGL_DIR}/src/core/lv_obj_event.c
    ${LVGL_DIR}/src/core/lv_obj_id_builtin.c
    ${LVGL_DIR}/src/core/lv_obj_pos.c
    ${LVGL_DIR}/src/core/lv_obj_property.c
    ${LVGL_DIR}/src/core/lv_obj_scroll.c
    ${LVGL_DIR}/src/core/lv_obj_style.c
    ${LVGL_DIR}/src/core/lv_obj_style_gen.c
    ${LVGL_DIR}/src/core/lv_obj_tree.c
    ${LVGL_DIR}/src/core/lv_refr.c

    ${LVGL_DIR}/src/display/lv_display.c

    ${LVGL_DIR}/src/draw/dma2d/lv_draw_dma2d.c
    ${LVGL_DIR}/src/draw/dma2d/lv_draw_dma2d_fill.c
    ${LVGL_DIR}/src/draw/dma2d/lv_draw_dma2d_img.c

    ${LVGL_DIR}/src/draw/lv_draw_3d.c
    ${LVGL_DIR}/src/draw/lv_draw_arc.c
    ${LVGL_DIR}/src/draw/lv_draw_buf.c
    ${LVGL_DIR}/src/draw/lv_draw.c
    ${LVGL_DIR}/src/draw/lv_draw_image.c
    ${LVGL_DIR}/src/draw/lv_draw_label.c
    ${LVGL_DIR}/src/draw/lv_draw_line.c
    ${LVGL_DIR}/src/draw/lv_draw_mask.c
    ${LVGL_DIR}/src/draw/lv_draw_rect.c
    ${LVGL_DIR}/src/draw/lv_draw_triangle.c
    ${LVGL_DIR}/src/draw/lv_draw_vector.c
    ${LVGL_DIR}/src/draw/lv_image_decoder.c

    ${LVGL_DIR}/src/draw/nxp/pxp/lv_draw_buf_pxp.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_draw_pxp.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_draw_pxp_fill.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_draw_pxp_img.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_draw_pxp_layer.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_pxp_cfg.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_pxp_osa.c
    ${LVGL_DIR}/src/draw/nxp/pxp/lv_pxp_utils.c
    ${LVGL_DIR}/src/draw/nxp/g2d/lv_draw_g2d_img.c
    ${LVGL_DIR}/src/draw/nxp/g2d/lv_g2d_utils.c
    ${LVGL_DIR}/src/draw/nxp/g2d/lv_draw_buf_g2d.c
    ${LVGL_DIR}/src/draw/nxp/g2d/lv_g2d_buf_map.c
    ${LVGL_DIR}/src/draw/nxp/g2d/lv_draw_g2d_fill.c
    ${LVGL_DIR}/src/draw/nxp/g2d/lv_draw_g2d.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_buf_vglite.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_arc.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_border.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_fill.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_img.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_label.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_layer.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_line.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_draw_vglite_triangle.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_vglite_buf.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_vglite_matrix.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_vglite_path.c
    ${LVGL_DIR}/src/draw/nxp/vglite/lv_vglite_utils.c

    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d_arc.c
    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d_border.c
    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d.c
    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d_fill.c
    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d_image.c
    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d_label.c
    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d_line.c
    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d_mask_rectangle.c
    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d_triangle.c
    ${LVGL_DIR}/src/draw/renesas/dave2d/lv_draw_dave2d_utils.c

    ${LVGL_DIR}/src/draw/sw/blend/lv_draw_sw_blend.c
    ${LVGL_DIR}/src/draw/sw/blend/lv_draw_sw_blend_to_al88.c
    ${LVGL_DIR}/src/draw/sw/blend/lv_draw_sw_blend_to_argb8888.c
    ${LVGL_DIR}/src/draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.c
    ${LVGL_DIR}/src/draw/sw/blend/lv_draw_sw_blend_to_i1.c
    ${LVGL_DIR}/src/draw/sw/blend/lv_draw_sw_blend_to_l8.c
    ${LVGL_DIR}/src/draw/sw/blend/lv_draw_sw_blend_to_rgb565.c
    ${LVGL_DIR}/src/draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.c
    ${LVGL_DIR}/src/draw/sw/blend/lv_draw_sw_blend_to_rgb888.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_arc.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_border.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_box_shadow.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_fill.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_grad.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_img.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_letter.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_line.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_mask.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_mask_rect.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_transform.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_triangle.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_utils.c
    ${LVGL_DIR}/src/draw/sw/lv_draw_sw_vector.c

    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_buf_vg_lite.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_arc.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_border.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_box_shadow.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_fill.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_img.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_label.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_layer.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_line.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_mask_rect.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_triangle.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_draw_vg_lite_vector.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_vg_lite_decoder.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_vg_lite_grad.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_vg_lite_math.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_vg_lite_path.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_vg_lite_pending.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_vg_lite_stroke.c
    ${LVGL_DIR}/src/draw/vg_lite/lv_vg_lite_utils.c

    ${LVGL_DIR}/src/font/lv_binfont_loader.c
    ${LVGL_DIR}/src/font/lv_font.c
    ${LVGL_DIR}/src/font/lv_font_dejavu_16_persian_hebrew.c
    ${LVGL_DIR}/src/font/lv_font_fmt_txt.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_10.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_12.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_14.c
    ${LVGL_DIR}/src/font/lv_font_montserrat_14_aligned.c
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
    ${LVGL_DIR}/src/font/lv_font_simsun_14_cjk.c
    ${LVGL_DIR}/src/font/lv_font_simsun_16_cjk.c
    ${LVGL_DIR}/src/font/lv_font_source_han_sans_sc_14_cjk.c
    ${LVGL_DIR}/src/font/lv_font_source_han_sans_sc_16_cjk.c
    ${LVGL_DIR}/src/font/lv_font_unscii_16.c
    ${LVGL_DIR}/src/font/lv_font_unscii_8.c

    ${LVGL_DIR}/src/indev/lv_indev.c
    ${LVGL_DIR}/src/indev/lv_indev_gesture.c
    ${LVGL_DIR}/src/indev/lv_indev_scroll.c

    ${LVGL_DIR}/src/layouts/flex/lv_flex.c
    ${LVGL_DIR}/src/layouts/grid/lv_grid.c
    ${LVGL_DIR}/src/layouts/lv_layout.c

    ${LVGL_DIR}/src/libs/barcode/code128.c
    ${LVGL_DIR}/src/libs/barcode/lv_barcode.c
    ${LVGL_DIR}/src/libs/bin_decoder/lv_bin_decoder.c
    ${LVGL_DIR}/src/libs/bmp/lv_bmp.c
    ${LVGL_DIR}/src/libs/expat/xmlparse.c
    ${LVGL_DIR}/src/libs/expat/xmlrole.c
    ${LVGL_DIR}/src/libs/expat/xmltok.c
    ${LVGL_DIR}/src/libs/expat/xmltok_impl.c
    ${LVGL_DIR}/src/libs/expat/xmltok_ns.c
    ${LVGL_DIR}/src/libs/ffmpeg/lv_ffmpeg.c
    ${LVGL_DIR}/src/libs/freetype/lv_freetype.c
    ${LVGL_DIR}/src/libs/freetype/lv_freetype_glyph.c
    ${LVGL_DIR}/src/libs/freetype/lv_freetype_image.c
    ${LVGL_DIR}/src/libs/freetype/lv_freetype_outline.c
    ${LVGL_DIR}/src/libs/freetype/lv_ftsystem.c
    ${LVGL_DIR}/src/libs/fsdrv/lv_fs_cbfs.c
    ${LVGL_DIR}/src/libs/fsdrv/lv_fs_fatfs.c
    ${LVGL_DIR}/src/libs/fsdrv/lv_fs_littlefs.c
    ${LVGL_DIR}/src/libs/fsdrv/lv_fs_memfs.c
    ${LVGL_DIR}/src/libs/fsdrv/lv_fs_posix.c
    ${LVGL_DIR}/src/libs/fsdrv/lv_fs_stdio.c
    ${LVGL_DIR}/src/libs/fsdrv/lv_fs_uefi.c
    ${LVGL_DIR}/src/libs/fsdrv/lv_fs_win32.c
    ${LVGL_DIR}/src/libs/gif/gifdec.c
    ${LVGL_DIR}/src/libs/gif/lv_gif.c
    ${LVGL_DIR}/src/libs/libjpeg_turbo/lv_libjpeg_turbo.c
    ${LVGL_DIR}/src/libs/libpng/lv_libpng.c
    ${LVGL_DIR}/src/libs/lodepng/lodepng.c
    ${LVGL_DIR}/src/libs/lodepng/lv_lodepng.c
    ${LVGL_DIR}/src/libs/lz4/lz4.c
    ${LVGL_DIR}/src/libs/qrcode/lv_qrcode.c
    ${LVGL_DIR}/src/libs/qrcode/qrcodegen.c
    ${LVGL_DIR}/src/libs/rle/lv_rle.c
    ${LVGL_DIR}/src/libs/rlottie/lv_rlottie.c
    ${LVGL_DIR}/src/libs/svg/lv_svg.c
    ${LVGL_DIR}/src/libs/svg/lv_svg_decoder.c
    ${LVGL_DIR}/src/libs/svg/lv_svg_parser.c
    ${LVGL_DIR}/src/libs/svg/lv_svg_render.c
    ${LVGL_DIR}/src/libs/svg/lv_svg_token.c
    ${LVGL_DIR}/src/libs/tiny_ttf/lv_tiny_ttf.c
    ${LVGL_DIR}/src/libs/tjpgd/lv_tjpgd.c
    ${LVGL_DIR}/src/libs/tjpgd/tjpgd.c

    ${LVGL_DIR}/src/lv_init.c

    ${LVGL_DIR}/src/misc/cache/lv_cache.c
    ${LVGL_DIR}/src/misc/cache/lv_cache_entry.c
    ${LVGL_DIR}/src/misc/cache/class/lv_cache_lru_rb.c
    ${LVGL_DIR}/src/misc/cache/class/lv_cache_lru_ll.c
    ${LVGL_DIR}/src/misc/cache/instance/lv_image_cache.c
    ${LVGL_DIR}/src/misc/cache/instance/lv_image_header_cache.c
    ${LVGL_DIR}/src/misc/lv_anim.c
    ${LVGL_DIR}/src/misc/lv_anim_timeline.c
    ${LVGL_DIR}/src/misc/lv_area.c
    ${LVGL_DIR}/src/misc/lv_array.c
    ${LVGL_DIR}/src/misc/lv_async.c
    ${LVGL_DIR}/src/misc/lv_bidi.c
    ${LVGL_DIR}/src/misc/lv_color.c
    ${LVGL_DIR}/src/misc/lv_color_op.c
    ${LVGL_DIR}/src/misc/lv_circle_buf.c
    ${LVGL_DIR}/src/misc/lv_event.c
    ${LVGL_DIR}/src/misc/lv_fs.c
    ${LVGL_DIR}/src/misc/lv_grad.c
    ${LVGL_DIR}/src/misc/lv_iter.c
    ${LVGL_DIR}/src/misc/lv_ll.c
    ${LVGL_DIR}/src/misc/lv_log.c
    ${LVGL_DIR}/src/misc/lv_lru.c
    ${LVGL_DIR}/src/misc/lv_math.c
    ${LVGL_DIR}/src/misc/lv_matrix.c
    ${LVGL_DIR}/src/misc/lv_palette.c
    ${LVGL_DIR}/src/misc/lv_profiler_builtin.c
    ${LVGL_DIR}/src/misc/lv_rb.c
    ${LVGL_DIR}/src/misc/lv_style.c
    ${LVGL_DIR}/src/misc/lv_style_gen.c
    ${LVGL_DIR}/src/misc/lv_templ.c
    ${LVGL_DIR}/src/misc/lv_text_ap.c
    ${LVGL_DIR}/src/misc/lv_text.c
    ${LVGL_DIR}/src/misc/lv_timer.c
    ${LVGL_DIR}/src/misc/lv_tree.c
    ${LVGL_DIR}/src/misc/lv_utils.c
    ${LVGL_DIR}/src/osal/lv_os.c

    ${LVGL_DIR}/src/others/file_explorer/lv_file_explorer.c
    ${LVGL_DIR}/src/others/font_manager/lv_font_manager.c
    ${LVGL_DIR}/src/others/font_manager/lv_font_manager_recycle.c
    ${LVGL_DIR}/src/others/fragment/lv_fragment.c
    ${LVGL_DIR}/src/others/fragment/lv_fragment_manager.c
    ${LVGL_DIR}/src/others/gridnav/lv_gridnav.c
    ${LVGL_DIR}/src/others/ime/lv_ime_pinyin.c
    ${LVGL_DIR}/src/others/imgfont/lv_imgfont.c
    ${LVGL_DIR}/src/others/monkey/lv_monkey.c
    ${LVGL_DIR}/src/others/observer/lv_observer.c
    ${LVGL_DIR}/src/others/snapshot/lv_snapshot.c
    ${LVGL_DIR}/src/others/sysmon/lv_sysmon.c
    ${LVGL_DIR}/src/others/vg_lite_tvg/vg_lite_matrix.c
    ${LVGL_DIR}/src/others/xml/lv_xml_base_types.c
    ${LVGL_DIR}/src/others/xml/lv_xml.c
    ${LVGL_DIR}/src/others/xml/lv_xml_component.c
    ${LVGL_DIR}/src/others/xml/lv_xml_parser.c
    ${LVGL_DIR}/src/others/xml/lv_xml_style.c
    ${LVGL_DIR}/src/others/xml/lv_xml_update.c
    ${LVGL_DIR}/src/others/xml/lv_xml_utils.c
    ${LVGL_DIR}/src/others/xml/lv_xml_widget.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_arc_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_bar_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_buttonmatrix_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_button_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_calendar_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_canvas_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_chart_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_checkbox_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_dropdown_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_event_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_image_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_keyboard_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_label_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_obj_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_roller_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_scale_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_slider_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_spangroup_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_table_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_tabview_parser.c
    ${LVGL_DIR}/src/others/xml/parsers/lv_xml_textarea_parser.c

    ${LVGL_DIR}/src/stdlib/builtin/lv_tlsf.c
    ${LVGL_DIR}/src/stdlib/clib/lv_string_clib.c
    ${LVGL_DIR}/src/stdlib/clib/lv_sprintf_clib.c

    ${LVGL_DIR}/src/stdlib/lv_mem.c

    ${LVGL_DIR}/src/themes/default/lv_theme_default.c

    ${LVGL_DIR}/src/themes/lv_theme.c
    ${LVGL_DIR}/src/themes/mono/lv_theme_mono.c
    ${LVGL_DIR}/src/themes/simple/lv_theme_simple.c

    ${LVGL_DIR}/src/tick/lv_tick.c

    ${LVGL_DIR}/src/widgets/3dtexture/lv_3dtexture.c
    ${LVGL_DIR}/src/widgets/animimage/lv_animimage.c
    ${LVGL_DIR}/src/widgets/arc/lv_arc.c
    ${LVGL_DIR}/src/widgets/bar/lv_bar.c
    ${LVGL_DIR}/src/widgets/button/lv_button.c
    ${LVGL_DIR}/src/widgets/buttonmatrix/lv_buttonmatrix.c
    ${LVGL_DIR}/src/widgets/calendar/lv_calendar.c
    ${LVGL_DIR}/src/widgets/calendar/lv_calendar_chinese.c
    ${LVGL_DIR}/src/widgets/calendar/lv_calendar_header_arrow.c
    ${LVGL_DIR}/src/widgets/calendar/lv_calendar_header_dropdown.c
    ${LVGL_DIR}/src/widgets/canvas/lv_canvas.c
    ${LVGL_DIR}/src/widgets/chart/lv_chart.c
    ${LVGL_DIR}/src/widgets/checkbox/lv_checkbox.c
    ${LVGL_DIR}/src/widgets/dropdown/lv_dropdown.c
    ${LVGL_DIR}/src/widgets/imagebutton/lv_imagebutton.c
    ${LVGL_DIR}/src/widgets/image/lv_image.c
    ${LVGL_DIR}/src/widgets/keyboard/lv_keyboard.c
    ${LVGL_DIR}/src/widgets/label/lv_label.c
    ${LVGL_DIR}/src/widgets/led/lv_led.c
    ${LVGL_DIR}/src/widgets/line/lv_line.c
    ${LVGL_DIR}/src/widgets/list/lv_list.c
    ${LVGL_DIR}/src/widgets/lottie/lv_lottie.c
    ${LVGL_DIR}/src/widgets/menu/lv_menu.c
    ${LVGL_DIR}/src/widgets/msgbox/lv_msgbox.c
    ${LVGL_DIR}/src/widgets/objx_templ/lv_objx_templ.c
    ${LVGL_DIR}/src/widgets/property/lv_animimage_properties.c
    ${LVGL_DIR}/src/widgets/property/lv_dropdown_properties.c
    ${LVGL_DIR}/src/widgets/property/lv_image_properties.c
    ${LVGL_DIR}/src/widgets/property/lv_keyboard_properties.c
    ${LVGL_DIR}/src/widgets/property/lv_label_properties.c
    ${LVGL_DIR}/src/widgets/property/lv_obj_properties.c
    ${LVGL_DIR}/src/widgets/property/lv_roller_properties.c
    ${LVGL_DIR}/src/widgets/property/lv_slider_properties.c
    ${LVGL_DIR}/src/widgets/property/lv_style_properties.c
    ${LVGL_DIR}/src/widgets/property/lv_textarea_properties.c
    ${LVGL_DIR}/src/widgets/roller/lv_roller.c
    ${LVGL_DIR}/src/widgets/scale/lv_scale.c
    ${LVGL_DIR}/src/widgets/slider/lv_slider.c
    ${LVGL_DIR}/src/widgets/span/lv_span.c
    ${LVGL_DIR}/src/widgets/spinbox/lv_spinbox.c
    ${LVGL_DIR}/src/widgets/spinner/lv_spinner.c
    ${LVGL_DIR}/src/widgets/switch/lv_switch.c
    ${LVGL_DIR}/src/widgets/table/lv_table.c
    ${LVGL_DIR}/src/widgets/tabview/lv_tabview.c
    ${LVGL_DIR}/src/widgets/textarea/lv_textarea.c
    ${LVGL_DIR}/src/widgets/tileview/lv_tileview.c
    ${LVGL_DIR}/src/widgets/win/lv_win.c

    lvgl.c
    lvgl_display.c
    lvgl_display_mono.c
    lvgl_display_8bit.c
    lvgl_display_16bit.c
    lvgl_display_24bit.c
    lvgl_display_32bit.c
    lvgl_display_mono.c
    lvgl_zephyr_osal.c
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
