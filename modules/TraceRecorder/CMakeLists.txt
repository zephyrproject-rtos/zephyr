# Copyright (c) 2021, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PERCEPIO_TRACERECORDER)

  set(TRACERECORDER_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()
  zephyr_library_sources_ifdef(
    CONFIG_PERCEPIO_TRACERECORDER
    ${TRACERECORDER_DIR}/kernelports/Zephyr/trcKernelPort.c
    ${TRACERECORDER_DIR}/trcAssert.c
    ${TRACERECORDER_DIR}/trcCounter.c
    ${TRACERECORDER_DIR}/trcDiagnostics.c
    ${TRACERECORDER_DIR}/trcEntryTable.c
    ${TRACERECORDER_DIR}/trcError.c
    ${TRACERECORDER_DIR}/trcEvent.c
    ${TRACERECORDER_DIR}/trcEventBuffer.c
    ${TRACERECORDER_DIR}/trcExtension.c
    ${TRACERECORDER_DIR}/trcHardwarePort.c
    ${TRACERECORDER_DIR}/trcHeap.c
    ${TRACERECORDER_DIR}/trcInternalEventBuffer.c
    ${TRACERECORDER_DIR}/trcInterval.c
    ${TRACERECORDER_DIR}/trcISR.c
    ${TRACERECORDER_DIR}/trcMultiCoreEventBuffer.c
    ${TRACERECORDER_DIR}/trcObject.c
    ${TRACERECORDER_DIR}/trcPrint.c
    ${TRACERECORDER_DIR}/trcStackMonitor.c
    ${TRACERECORDER_DIR}/trcStateMachine.c
    ${TRACERECORDER_DIR}/trcStaticBuffer.c
    ${TRACERECORDER_DIR}/trcStreamingRecorder.c
    ${TRACERECORDER_DIR}/trcString.c
    ${TRACERECORDER_DIR}/trcTask.c
    ${TRACERECORDER_DIR}/trcTimestamp.c
    )

  if(CONFIG_PERCEPIO_TRC_CFG_STREAM_PORT_RTT)
    zephyr_library_sources(
      ${TRACERECORDER_DIR}/streamports/Jlink_RTT/trcStreamPort.c
    )

    zephyr_include_directories(
      ${TRACERECORDER_DIR}/kernelports/Zephyr/streamports/Jlink_RTT/config/
      ${TRACERECORDER_DIR}/streamports/Jlink_RTT/include/
    )
  endif()

  if(CONFIG_PERCEPIO_TRC_CFG_STREAM_PORT_ITM)
    zephyr_library_sources(
      ${TRACERECORDER_DIR}/streamports/ARM_ITM/trcStreamPort.c
    )

    zephyr_include_directories(
      ${TRACERECORDER_DIR}/kernelports/Zephyr/streamports/ARM_ITM/config/
      ${TRACERECORDER_DIR}/streamports/ARM_ITM/include/
    )
  endif()

  if(CONFIG_PERCEPIO_TRC_CFG_STREAM_PORT_RINGBUFFER)
    zephyr_library_sources(
      ${TRACERECORDER_DIR}/streamports/RingBuffer/trcStreamPort.c
    )

    zephyr_include_directories(
      ${TRACERECORDER_DIR}/kernelports/Zephyr/streamports/RingBuffer/config/
      ${TRACERECORDER_DIR}/streamports/RingBuffer/include/
    )
  endif()

  zephyr_include_directories(
    ${TRACERECORDER_DIR}/kernelports/Zephyr/include
    ${TRACERECORDER_DIR}/kernelports/Zephyr/config
    ${TRACERECORDER_DIR}/kernelports/Zephyr/config/core
    ${TRACERECORDER_DIR}/include
    )

endif()
