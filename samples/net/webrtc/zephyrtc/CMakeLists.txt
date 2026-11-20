# SPDX-FileCopyrightText: Copyright (c) 2026 Meta Platforms, Inc.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyrtc LANGUAGES C)

zephyr_include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/config"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_sources(app PRIVATE
  src/main.c
  src/platform_zephyr.c
  src/fault_trace_zephyr.c
  src/libpeer_transport_probe.c
  src/libpeer_endpoint.c
)

if(CONFIG_ZEPHYRTC_AUDIO_RT1170)
  set(MCUX_TEMPSENSOR_DIR
      "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk-ng/drivers/tempsensor")
  set(MCUX_RT1170_POWER_DIR
      "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk-ng/devices/RT/RT1170/MIMXRT1176/drivers/power")
  target_include_directories(app PRIVATE
    "${MCUX_TEMPSENSOR_DIR}"
    "${MCUX_RT1170_POWER_DIR}"
  )
  target_sources(app PRIVATE
    src/media_rt1170_zephyr.c
    "${MCUX_TEMPSENSOR_DIR}/fsl_tempsensor.c"
    "${MCUX_RT1170_POWER_DIR}/fsl_soc_src.c"
  )
  # The SDK drivers do not build cleanly with the sample's warning flags.
  set_source_files_properties(
    "${MCUX_TEMPSENSOR_DIR}/fsl_tempsensor.c"
    "${MCUX_RT1170_POWER_DIR}/fsl_soc_src.c"
    PROPERTIES COMPILE_OPTIONS "-Wno-error"
  )
else()
  target_sources(app PRIVATE src/media_synthetic_zephyr.c)
endif()

target_sources_ifdef(CONFIG_ZEPHYRTC_MODE_LOOPBACK app PRIVATE
  src/libpeer_loopback.c
)
target_sources_ifdef(CONFIG_ZEPHYRTC_SIGNALING_CLIENT app PRIVATE
  src/signaling_client_zephyr.c
  src/libpeer_offerer.c
)
target_sources_ifdef(CONFIG_ZEPHYRTC_MODE_HTTP_SIGNALING app PRIVATE
  src/signaling_test_server.c
  src/signaling_tests.c
)
target_sources_ifdef(CONFIG_ZEPHYRTC_MODE_EXTERNAL app PRIVATE
  src/libpeer_external.c
)

if(CONFIG_ZEPHYRTC_VIDEO_H264)
  target_sources(app PRIVATE
    src/video_codec_zephyr.c
    src/video_color_zephyr.c
  )
  set_source_files_properties(src/video_codec_zephyr.c PROPERTIES
    COMPILE_OPTIONS "-Wno-unused-function;-Wno-sign-compare;-Wno-strict-prototypes"
  )
endif()

target_compile_options(app PRIVATE
  -Wall
  -Wextra
  -Wformat=2
  -Wmissing-prototypes
  -Wstrict-prototypes
  -Werror=implicit-function-declaration
  -Werror=incompatible-pointer-types
  -Werror=return-type
)
