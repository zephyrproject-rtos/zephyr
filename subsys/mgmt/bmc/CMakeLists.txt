# SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(bmc)

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library_sources(
  auth.c
  bmc_init.c
  callbacks.c
  config.c
  host.c
  net.c
  vpd.c
)

zephyr_library_sources_ifdef(CONFIG_NET_DHCPV4 dhcp.c)
zephyr_library_sources_ifdef(CONFIG_BMC_NTP ntp.c)
zephyr_library_sources_ifdef(CONFIG_BMC_RTC rtc.c)
zephyr_library_sources_ifdef(CONFIG_BMC_HOST_GPIO host_gpio.c)
zephyr_library_sources_ifdef(CONFIG_BMC_RESET_BUTTON button.c)
zephyr_library_sources_ifdef(CONFIG_BMC_SENSORS sensor.c)
zephyr_library_sources_ifdef(CONFIG_BMC_HTTP http.c)
zephyr_library_sources_ifdef(CONFIG_BMC_JTAG jtag.c)
zephyr_library_sources_ifdef(CONFIG_BMC_CONSOLE_LOGGER console_logger.c)
zephyr_library_sources_ifdef(CONFIG_BMC_CONSOLE_BRIDGE console_bridge.c)
zephyr_library_sources_ifdef(CONFIG_BMC_CONSOLE_BRIDGE_WS console_bridge_ws.c)

zephyr_library_sources_ifdef(CONFIG_BMC_REDFISH
  redfish/redfish_account.c
  redfish/redfish_chassis.c
  redfish/redfish_core.c
  redfish/redfish_identity.c
  redfish/redfish_manager.c
  redfish/redfish_root.c
  redfish/redfish_system.c
)

zephyr_library_sources_ifdef(CONFIG_BMC_SENSORS
  redfish/redfish_sensors.c
)

if(CONFIG_BMC_REDFISH)
  generate_inc_file_for_target(
    bmc
    redfish/redfish_metadata.xml
    ${ZEPHYR_BINARY_DIR}/include/generated/redfish_metadata.xml.gz.inc
    --gzip
  )
endif()

if(CONFIG_BMC_DEFAULT_ADMIN_PASSWORD_UNCHANGED)
  message(WARNING
    "The BMC is built with the placeholder administrator password. "
    "Set CONFIG_BMC_DEFAULT_ADMIN_PASSWORD, or register a custom "
    "authentication backend with bmc_auth_ops_register()."
  )
endif()

zephyr_linker_sources(SECTIONS ${CMAKE_CURRENT_SOURCE_DIR}/sections-rom.ld)
zephyr_iterable_section(NAME bmc_component KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_iterable_section(NAME bmc_sensor KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_iterable_section(NAME bmc_redfish_oem KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_iterable_section(NAME bmc_redfish_resource KVMA RAM_REGION GROUP RODATA_REGION)

zephyr_linker_section_ifdef(CONFIG_BMC_HTTP
  NAME http_resource_desc_bmc_http_service
  KVMA RAM_REGION GROUP RODATA_REGION
)

zephyr_linker_section_ifdef(CONFIG_BMC_HTTPS
  NAME http_resource_desc_bmc_https_service
  KVMA RAM_REGION GROUP RODATA_REGION
)
