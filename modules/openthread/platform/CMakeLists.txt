# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(openthread_platform)
if(CONFIG_OPENTHREAD_ALARM_COUNTER)
  zephyr_library_sources(alarm_counter.c)
else()
  zephyr_library_sources(alarm_micro.c)
endif()
zephyr_library_sources(
  alarm_milli.c
  entropy.c
  misc.c
  platform.c
  )

if(NOT CONFIG_HDLC_RCP_IF)
  # Radio platform implementation dedicated for L2 Platform adaptation layer
  zephyr_library_sources_ifdef(CONFIG_NET_L2_OPENTHREAD radio.c)
  zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_DIAG diag.c)
  zephyr_library_sources(spi.c)
endif()

zephyr_library_sources_ifdef(CONFIG_HDLC_RCP_IF
  radio_spinel.cpp
  hdlc_interface.cpp
  )

zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_BLE_TCAT ble.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_COPROCESSOR uart.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_CRYPTO_PSA crypto_psa.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_EXTERNAL_HEAP memory.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_PLATFORM_MESSAGE_MANAGEMENT messagepool.c)
zephyr_library_sources_ifdef(CONFIG_SETTINGS settings.c)
zephyr_library_sources_ifndef(CONFIG_LOG_BACKEND_SPINEL logging.c)

zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER infra_if.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER udp.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER mdns_socket.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER border_agent.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER trel.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER dhcp6_pd.c)
zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER dns_upstream_resolver.c)

zephyr_include_directories(.)

if(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)
  zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/l2/openthread)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/lib/dns)
endif()

# include vendor-specific resources
if(CONFIG_OPENTHREAD_RADIO_VENDOR_IMPLEMENTATION)
  if("${CONFIG_OPENTHREAD_RADIO_VENDOR_IMPLEMENTATION_FILE}" STREQUAL "")
    message(FATAL_ERROR
      "CONFIG_OPENTHREAD_RADIO_VENDOR_IMPLEMENTATION is enabled but "
      "CONFIG_OPENTHREAD_RADIO_VENDOR_IMPLEMENTATION_FILE is not set.")
  endif()

  set(_vendor_radio_file ${CONFIG_OPENTHREAD_RADIO_VENDOR_IMPLEMENTATION_FILE})

  # If not absolute, resolve: first relative to application dir, then to in-tree vendor dir
  if(NOT IS_ABSOLUTE ${_vendor_radio_file})
    if(EXISTS ${APPLICATION_SOURCE_DIR}/${_vendor_radio_file})
      set(_vendor_radio_file ${APPLICATION_SOURCE_DIR}/${_vendor_radio_file})
    elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/vendor/${_vendor_radio_file})
      set(_vendor_radio_file ${CMAKE_CURRENT_LIST_DIR}/vendor/${_vendor_radio_file})
    endif()
  endif()

  if(NOT EXISTS ${_vendor_radio_file})
    message(FATAL_ERROR
      "CONFIG_OPENTHREAD_RADIO_VENDOR_IMPLEMENTATION_FILE is set to "
      "'${CONFIG_OPENTHREAD_RADIO_VENDOR_IMPLEMENTATION_FILE}' but the file does not exist.")
  endif()

  get_filename_component(_vendor_radio_dir ${_vendor_radio_file} DIRECTORY)
  zephyr_library_include_directories(${_vendor_radio_dir})
  zephyr_library_sources(${_vendor_radio_file})
endif()

zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS_BUILTIN mbedtls_iface)
