# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(jpeg_pipeline)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Generate MJPEG file to test using GStreamer CLI
find_program(GST_LAUNCH gst-launch-1.0 REQUIRED)

set(TEST_MJPEG_FILE ${CMAKE_CURRENT_BINARY_DIR}/test.mjpeg)

add_custom_command(
    OUTPUT ${TEST_MJPEG_FILE}
    COMMAND ${GST_LAUNCH}
        videotestsrc num-buffers=${CONFIG_MJPEG_NUM_FRAMES}
        ! "video/x-raw,width=${CONFIG_JPEG_IMAGE_WIDTH},height=${CONFIG_JPEG_IMAGE_HEIGHT}"
        ! jpegenc
        ! filesink location=${TEST_MJPEG_FILE}
    COMMENT "Generating test.mjpeg (${CONFIG_JPEG_IMAGE_WIDTH}x${CONFIG_JPEG_IMAGE_HEIGHT}, ${CONFIG_MJPEG_NUM_FRAMES} frames) using GStreamer"
)

add_custom_target(gen_test_mjpeg DEPENDS ${TEST_MJPEG_FILE})

generate_inc_file_for_target(app
  ${TEST_MJPEG_FILE}
  ${CMAKE_CURRENT_BINARY_DIR}/mjpeg.inc
)

add_dependencies(app gen_test_mjpeg)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
