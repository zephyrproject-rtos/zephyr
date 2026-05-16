cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(lvgl_anim)

# ───────────────────────── Video → LVGL frame generation ─────────────────────────
#
# Reads display width/height and pixel-format from the DTS chosen node so the
# generated frames always match the board's display configuration.
#
# Kconfig variables consumed here:
#   CONFIG_LVGL_ANIMATION_VIDEO_PATH       Path to the source video file
#   CONFIG_LVGL_ANIMATION_FRAME_COUNT      Number of frames to extract
#   CONFIG_LVGL_ANIMATION_FIT              Fit mode: crop | letterbox | stretch
#   CONFIG_LVGL_ANIMATION_PREFIX           C variable name prefix (default: frame)
#
# DTS properties read via dt_chosen / dt_prop:
#   /chosen zephyr,display  → width, height, pixel-format
#
# ────────────────────────────────────────────────────────────────────────────────

dt_chosen(DISPLAY_NODE PROPERTY "zephyr,display")
dt_prop(DISPLAY_WIDTH  PATH "${DISPLAY_NODE}" PROPERTY "width")
dt_prop(DISPLAY_HEIGHT PATH "${DISPLAY_NODE}" PROPERTY "height")
dt_prop(DISPLAY_PIXEL_FORMAT PATH "${DISPLAY_NODE}" PROPERTY "pixel-format")

if(DISPLAY_PIXEL_FORMAT STREQUAL "1")
    set(LVGL_COLOR_FORMAT "rgb888")
elseif(DISPLAY_PIXEL_FORMAT STREQUAL "8")
    set(LVGL_COLOR_FORMAT "argb8888")
elseif(DISPLAY_PIXEL_FORMAT STREQUAL "16")
    set(LVGL_COLOR_FORMAT "rgb565")
else()
    message(WARNING "Unknown pixel-format '${DISPLAY_PIXEL_FORMAT}', default rgb565")
    set(LVGL_COLOR_FORMAT "rgb565")
endif()

message(STATUS "Display: ${DISPLAY_WIDTH}x${DISPLAY_HEIGHT}  format=${LVGL_COLOR_FORMAT}")

if(NOT EXISTS "${CONFIG_LVGL_ANIMATION_VIDEO_PATH}")
    message(FATAL_ERROR "Video file not found: '${CONFIG_LVGL_ANIMATION_VIDEO_PATH}'")
endif()

set(LVGL_IMG_DIR "${ZEPHYR_BINARY_DIR}/include/generated/img")
set(LVGL_STAMP "${ZEPHYR_BINARY_DIR}/include/generated/lvgl_frames.stamp")
set(LVGL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/script/video_to_lvgl_frames.py")

set(LVGL_SCRIPT_ARGS
    "${CONFIG_LVGL_ANIMATION_VIDEO_PATH}"
    --width   "${DISPLAY_WIDTH}"
    --height  "${DISPLAY_HEIGHT}"
    --frames  "${CONFIG_LVGL_ANIMATION_FRAME_COUNT}"
    --format  "${LVGL_COLOR_FORMAT}"
    --fit     "${CONFIG_LVGL_ANIMATION_FIT}"
    --out     "${LVGL_IMG_DIR}"
    --prefix  "${CONFIG_LVGL_ANIMATION_PREFIX}"
)

add_custom_command(
    OUTPUT  "${LVGL_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E echo
            "-- Generating LVGL frames: ${DISPLAY_WIDTH}x${DISPLAY_HEIGHT} ${LVGL_COLOR_FORMAT}"
    COMMAND echo "y" | ${Python3_EXECUTABLE} "${LVGL_SCRIPT}" ${LVGL_SCRIPT_ARGS}
    COMMAND ${CMAKE_COMMAND} -E touch "${LVGL_STAMP}"
    DEPENDS
        "${CONFIG_LVGL_ANIMATION_VIDEO_PATH}"
        "${LVGL_SCRIPT}"
        "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt"
    COMMENT "Generating LVGL frame headers from ${CONFIG_LVGL_ANIMATION_VIDEO_PATH}"
    VERBATIM
)

add_custom_target(lvgl_frames_gen ALL
    DEPENDS "${LVGL_STAMP}"
)

add_dependencies(app lvgl_frames_gen)

target_sources(app PRIVATE src/main.c)
