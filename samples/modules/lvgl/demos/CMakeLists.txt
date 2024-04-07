# Copyright (c) 2023 Fabian Blatz <fabianblatz@gmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lvgl_sample)

set(LVGL_DIR ${ZEPHYR_LVGL_MODULE_DIR})

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE
    ${LVGL_DIR}/demos/
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_MUSIC app PRIVATE
    ${LVGL_DIR}/demos/music/lv_demo_music_list.c
    ${LVGL_DIR}/demos/music/lv_demo_music.c
    ${LVGL_DIR}/demos/music/lv_demo_music_main.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_next.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_wave_top_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_4_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_4.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_corner_right.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_1.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_slider_knob_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_3.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_pause.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_pause_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_wave_bottom_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_2_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_list_play_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_wave_top.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_play_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_1_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_wave_bottom.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_corner_left_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_play.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_list_border.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_2.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_next_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_list_play.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_list_border_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_rnd.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_3.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_prev_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_loop.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_1.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_slider_knob.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_corner_right_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_corner_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_corner_left.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_2_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_3_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_icon_1_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_2.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_prev.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_rnd_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_list_pause_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_cover_3_large.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_list_pause.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_logo.c
    ${LVGL_DIR}/demos/music/assets/img_lv_demo_music_btn_loop_large.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_BENCHMARK app PRIVATE
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_avatar.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_lvgl_logo_argb.c
    ${LVGL_DIR}/demos/benchmark/assets/img_benchmark_lvgl_logo_rgb.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_benchmark_montserrat_12_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_benchmark_montserrat_16_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/assets/lv_font_benchmark_montserrat_28_compr_az.c.c
    ${LVGL_DIR}/demos/benchmark/lv_demo_benchmark.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_STRESS app PRIVATE
    ${LVGL_DIR}/demos/stress/lv_demo_stress.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_WIDGETS app PRIVATE
    ${LVGL_DIR}/demos/widgets/assets/img_clothes.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_avatar.c
    ${LVGL_DIR}/demos/widgets/assets/img_demo_widgets_needle.c
    ${LVGL_DIR}/demos/widgets/assets/img_lvgl_logo.c
    ${LVGL_DIR}/demos/widgets/lv_demo_widgets.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_FLEX_LAYOUT app PRIVATE
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_main.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_view.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_flex_loader.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_view_child_node.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_view_ctrl_pad.c
    ${LVGL_DIR}/demos/flex_layout/lv_demo_flex_layout_ctrl_pad.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_KEYPAD_AND_ENCODER app PRIVATE
    ${LVGL_DIR}/demos/keypad_encoder/lv_demo_keypad_encoder.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_RENDER app PRIVATE
    ${LVGL_DIR}/demos/render/assets/img_render_arc_bg.c
    ${LVGL_DIR}/demos/render/assets/img_render_lvgl_logo_argb8888.c
    ${LVGL_DIR}/demos/render/assets/img_render_lvgl_logo_i1.c
    ${LVGL_DIR}/demos/render/assets/img_render_lvgl_logo_l8.c
    ${LVGL_DIR}/demos/render/assets/img_render_lvgl_logo_rgb565.c
    ${LVGL_DIR}/demos/render/assets/img_render_lvgl_logo_rgb565a8.c
    ${LVGL_DIR}/demos/render/assets/img_render_lvgl_logo_rgb888.c
    ${LVGL_DIR}/demos/render/assets/img_render_lvgl_logo_xrgb8888.c
    ${LVGL_DIR}/demos/render/lv_demo_render.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_SCROLL app PRIVATE
    ${LVGL_DIR}/demos/scroll/lv_demo_scroll.c
)

target_sources_ifdef(CONFIG_LV_USE_DEMO_MULTILANG app PRIVATE
    ${LVGL_DIR}/demos/multilang/assets/img_multilang_like.c
    ${LVGL_DIR}/demos/multilang/assets/fonts/font_multilang_large.c
    ${LVGL_DIR}/demos/multilang/assets/fonts/font_multilang_small.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_movie_camera.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_flexed_biceps.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_rocket.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_artist_palette.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_deciduous_tree.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_cat_face.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_red_heart.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_camera_with_flash.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_dog_face.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_books.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_earth_globe_europe_africa.c
    ${LVGL_DIR}/demos/multilang/assets/emojis/img_emoji_soccer_ball.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_19.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_4.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_8.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_5.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_16.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_22.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_9.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_6.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_18.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_17.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_13.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_2.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_3.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_25.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_14.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_1.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_11.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_7.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_15.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_12.c
    ${LVGL_DIR}/demos/multilang/assets/avatars/img_multilang_avatar_10.c
    ${LVGL_DIR}/demos/multilang/lv_demo_multilang.c
)
