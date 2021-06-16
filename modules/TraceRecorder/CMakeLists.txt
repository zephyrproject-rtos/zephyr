# Copyright (c) 2021, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PERCEPIO_TRACERECORDER)

  set(TRACERECORDER_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()
  zephyr_library_sources_ifdef(
    CONFIG_PERCEPIO_TRACERECORDER
    ${TRACERECORDER_DIR}/kernelports/Zephyr/trcKernelPort.c
    ${TRACERECORDER_DIR}/trcStreamingRecorder.c
    ${TRACERECORDER_DIR}/extras/SDK/trcSDK.c
    )

  if(CONFIG_PERCEPIO_RECORDER_TRC_RECORDER_STREAM_PORT_RTT)
    zephyr_library_sources(
      ${TRACERECORDER_DIR}/streamports/Jlink_RTT/trcStreamingPort.c
    )

    zephyr_include_directories(
      ${TRACERECORDER_DIR}/streamports/Jlink_RTT/include/
    )
  endif()

  zephyr_include_directories(
    ${TRACERECORDER_DIR}/kernelports/Zephyr/include
    ${TRACERECORDER_DIR}/kernelports/Zephyr/config
    ${TRACERECORDER_DIR}/include
    ${TRACERECORDER_DIR}/extras/SDK/include
    )

endif()
