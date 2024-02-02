# Copyright (c) 2021, Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_PERCEPIO_TRACERECORDER OR CONFIG_PERCEPIO_DFM)
  zephyr_library()
endif()

if(CONFIG_PERCEPIO_TRACERECORDER)

  set(TRACERECORDER_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/TraceRecorder)

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
    ${TRACERECORDER_DIR}/trcDependency.c
    ${TRACERECORDER_DIR}/trcRunnable.c
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

  if (CONFIG_PERCEPIO_TRC_CFG_STREAM_PORT_ZEPHYR_SEMIHOST)
    zephyr_library_sources(
      ${TRACERECORDER_DIR}/kernelports/Zephyr/streamports/Semihost/trcStreamPort.c
    )

    zephyr_include_directories(
      ${TRACERECORDER_DIR}/kernelports/Zephyr/streamports/Semihost/config
      ${TRACERECORDER_DIR}/kernelports/Zephyr/streamports/Semihost/include
    )
  endif()


  zephyr_include_directories(
    ${TRACERECORDER_DIR}/kernelports/Zephyr/include
    ${TRACERECORDER_DIR}/kernelports/Zephyr/config
    ${TRACERECORDER_DIR}/kernelports/Zephyr/config/core
    ${TRACERECORDER_DIR}/include
    )

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND python3 ${TRACERECORDER_DIR}/kernelports/Zephyr/scripts/tz_parse_syscalls.py ${CMAKE_BINARY_DIR} ${ZEPHYR_BASE}
    )

endif()

if(CONFIG_PERCEPIO_DFM)

  set(DFM_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/DFM)

  zephyr_library_sources(
    ${DFM_DIR}/dfm.c
    ${DFM_DIR}/dfmAlert.c
    ${DFM_DIR}/dfmCloud.c
    ${DFM_DIR}/dfmEntry.c
    ${DFM_DIR}/dfmSession.c
    ${DFM_DIR}/dfmStorage.c
    ${DFM_DIR}/dfmRetainedMemory.c
    ${DFM_DIR}/kernelports/Zephyr/dfmKernelPort.c
    )

  if(CONFIG_PERCEPIO_DFM_CFG_STORAGEPORT_NONE)
    zephyr_library_sources(
      ${DFM_DIR}/storageports/Dummy/dfmStoragePort.c
    )

    zephyr_include_directories(
      ${DFM_DIR}/storageports/Dummy/include/
    )
  endif()

  if(CONFIG_PERCEPIO_DFM_CFG_STORAGEPORT_FLASH)
    zephyr_library_sources(
      ${DFM_DIR}/kernelports/Zephyr/storageports/Flash/dfmStoragePort.c
    )

    zephyr_include_directories(
      ${DFM_DIR}/kernelports/Zephyr/storageports/Flash/include/
    )
  endif()

  if(CONFIG_PERCEPIO_DFM_CFG_STORAGEPORT_FILESYSTEM)
    zephyr_library_sources(
      ${DFM_DIR}/kernelports/Zephyr/storageports/Filesystem/dfmStoragePort.c
    )

    zephyr_include_directories(
      ${DFM_DIR}/kernelports/Zephyr/storageports/Filesystem/include/
    )
  endif()

  if(CONFIG_PERCEPIO_DFM_CFG_CLOUDPORT_NONE)
    zephyr_library_sources(
      ${DFM_DIR}/cloudports/Dummy/dfmCloudPort.c
    )

    zephyr_include_directories(
      ${DFM_DIR}/cloudports/Dummy/include/
    )
  endif()

  if(CONFIG_PERCEPIO_DFM_CFG_CLOUDPORT_SERIAL)
    zephyr_library_sources(
      ${DFM_DIR}/kernelports/Zephyr/cloudports/Serial/dfmCloudPort.c
    )

    zephyr_include_directories(
      ${DFM_DIR}/kernelports/Zephyr/cloudports/Serial/config/
      ${DFM_DIR}/kernelports/Zephyr/cloudports/Serial/include/
    )
  endif()

  if(CONFIG_PERCEPIO_DFM_CFG_RETAINED_MEMORY)
    zephyr_library_sources(
      ${DFM_DIR}/kernelports/Zephyr/dfmRetainedMemoryPort.c
    )
  endif()

  zephyr_include_directories(
    ${DFM_DIR}/kernelports/Zephyr/config
    ${DFM_DIR}/kernelports/Zephyr/include
    ${DFM_DIR}/include
    )

endif()
