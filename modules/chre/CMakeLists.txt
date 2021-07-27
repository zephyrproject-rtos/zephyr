# Copyright (c) 2021 Google LLC
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CHRE)
  set(CHRE_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()

  set(CHRE_INCLUDE_DIRS
      "${CHRE_DIR}/chre_api/include"
      "${CHRE_DIR}/chre_api/include/chre_api"
      "${CHRE_DIR}/core/include"
      "${CHRE_DIR}/platform/include"
      "${CHRE_DIR}/util/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/include")
  zephyr_include_directories("${CHRE_INCLUDE_DIRS}")
  zephyr_library_include_directories("${CHRE_INCLUDE_DIRS}")

  zephyr_library_sources(
      "src/log_module.c"
      "${CHRE_DIR}/core/debug_dump_manager.cc"
      "${CHRE_DIR}/core/event.cc"
      "${CHRE_DIR}/core/event_loop.cc"
      "${CHRE_DIR}/core/event_loop_manager.cc"
      "${CHRE_DIR}/core/event_ref_queue.cc"
      "${CHRE_DIR}/core/host_comms_manager.cc"
      "${CHRE_DIR}/core/init.cc"
      "${CHRE_DIR}/core/log.cc"
      "${CHRE_DIR}/core/nanoapp.cc"
      "${CHRE_DIR}/core/settings.cc"
      "${CHRE_DIR}/core/static_nanoapps.cc"
      "${CHRE_DIR}/core/timer_pool.cc"
  )

  # Optional audio support
  if(DEFINED CHRE_AUDIO_SUPPORT_ENABLED)
    add_compile_definitions(CHRE_AUDIO_SUPPORT_ENABLED)
    zephyr_library_sources("${CHRE_DIR}/core/audio_request_manager.cc")
  endif()

  # Optional GNSS support
  if(DEFINED CHRE_GNSS_SUPPORT_ENABLED)
    add_compile_definitions(CHRE_GNSS_SUPPORT_ENABLED)
    zephyr_library_sources("${CHRE_DIR}/core/gnss_manager.cc")
  endif()

  # Optional sensor support
  if(DEFINED CHRE_SENSORS_SUPPORT_ENABLED)
    add_compile_definitions(CHRE_SENSORS_SUPPORT_ENABLED)
    zephyr_library_sources(
        "${CHRE_DIR}/core/sensor.cc"
        "${CHRE_DIR}/core/sensor_request.cc"
        "${CHRE_DIR}/core/sensor_request_manager.cc"
        "${CHRE_DIR}/core/sensor_request_multiplexer.cc"
        "${CHRE_DIR}/core/sensor_type.cc"
        "${CHRE_DIR}/core/sensor_type_helpers.cc"
    )
  endif()

  # Optional WiFi support
  if(DEFINED CHRE_WIFI_SUPPORT_ENABLED)
    add_compile_definitions(CHRE_WIFI_SUPPORT_ENABLED)
    zephyr_library_sources(
        "${CHRE_DIR}/core/wifi_request_manager.cc"
        "${CHRE_DIR}/core/wifi_scan_request.cc"
    )
  endif()

  # Optional WWAN support
  if(DEFINED CHRE_WWAN_SUPPORT_ENABLED)
    add_compile_definitions(CHRE_WWAN_SUPPORT_ENABLED)
    zephyr_library_sources("${CHRE_DIR}/core/wwan_request_manager.cc")
  endif()

  add_compile_definitions(
      CHRE_MESSAGE_TO_HOST_MAX_SIZE=${CONFIG_CHRE_MESSAGE_TO_HOST_MAX_SIZE})

  add_compile_definitions(CHRE_FILENAME=__FILE__)

  # Add logging definitions
  if((NOT DEFINED CONFIG_CHRE_LOG_LEVEL) OR
     "${CONFIG_CHRE_LOG_LEVEL}" EQUAL "0")
    add_compile_definitions(CHRE_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_MUTE)
  elseif("${CONFIG_CHRE_LOG_LEVEL}" EQUAL "1")
    add_compile_definitions(CHRE_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_ERROR)
  elseif("${CONFIG_CHRE_LOG_LEVEL}" EQUAL "2")
    add_compile_definitions(CHRE_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_WARN)
  elseif("${CONFIG_CHRE_LOG_LEVEL}" EQUAL "3")
    add_compile_definitions(CHRE_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_INFO)
  elseif(("${CONFIG_CHRE_LOG_LEVEL}" EQUAL "4") OR
         ("${CONFIG_CHRE_LOG_LEVEL}" EQUAL "5"))
    # Debug and verbose are collapsed into one since Zephyr doesn't
    # differentiate the two levels
    add_compile_definitions(CHRE_MINIMUM_LOG_LEVEL=CHRE_LOG_LEVEL_VERBOSE)
  endif()

  if(DEFINED CONFIG_CHRE_ASSERTIONS)
    add_compile_definitions(CHRE_ASSERTIONS_ENABLED)
  else()
    add_compile_definitions(CHRE_ASSERTIONS_DISABLED)
  endif()
endif()
