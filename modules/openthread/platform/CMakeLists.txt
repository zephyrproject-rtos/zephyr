# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(openthread_platform)
zephyr_library_sources(
  alarm.c
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

zephyr_include_directories(.)

if(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
  zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/l2/openthread)
endif()
