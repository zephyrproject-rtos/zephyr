if(CONFIG_OPENTHREAD)
if(CONFIG_OPENTHREAD_SOURCES)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

macro(kconfig_to_ot_option kconfig_option ot_config description)
    if(${kconfig_option})
      set(${ot_config} ON CACHE BOOL "${description}" FORCE)
    else()
      set(${ot_config} OFF CACHE BOOL "${description}" FORCE)
    endif()
endmacro()

# OpenThread options
set(OT_BUILD_EXECUTABLES OFF CACHE BOOL "Disable OpenThread samples")
set(OT_BUILTIN_MBEDTLS_MANAGEMENT OFF CACHE BOOL "Use Zephyr's mbedTLS heap")
set(OT_PLATFORM "zephyr" CACHE STRING "Zephyr as a target platform")
set(OT_PLATFORM_POWER_CALIBRATION OFF CACHE BOOL "Use Zephyr's power calibration handled by Radio Driver")
set(OT_THREAD_VERSION ${CONFIG_OPENTHREAD_THREAD_VERSION} CACHE STRING "User selected Thread stack version")
set(OT_CLI_TRANSPORT "CONSOLE" CACHE STRING "Set CLI to use console interpreter")

string(REPLACE " " ";" OT_MBEDTLS_LIB_LIST " ${CONFIG_OPENTHREAD_MBEDTLS_LIB_NAME}")

set(
    OT_EXTERNAL_MBEDTLS
    ${OT_MBEDTLS_LIB_LIST}
    CACHE STRING
    "Specify external mbedtls library"
    FORCE
)

if(CONFIG_OPENTHREAD_FTD)
  set(OT_FTD ON CACHE BOOL "Enable FTD" FORCE)
  set(OT_MTD OFF CACHE BOOL "Enable MTD" FORCE)
elseif(CONFIG_OPENTHREAD_MTD)
  set(OT_FTD OFF CACHE BOOL "Enable FTD" FORCE)
  set(OT_MTD ON CACHE BOOL "Enable MTD" FORCE)
endif()

kconfig_to_ot_option(CONFIG_OPENTHREAD_ANYCAST_LOCATOR OT_ANYCAST_LOCATOR "Enable anycast locator")
kconfig_to_ot_option(CONFIG_ASSERT OT_ASSERT "Enable assert function OT_ASSERT()")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BACKBONE_ROUTER OT_BACKBONE_ROUTER "Enable backbone router functionality")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BACKBONE_ROUTER_DUA_NDPROXYING OT_BACKBONE_ROUTER_DUA_NDPROXYING "Enable BBR DUA ND Proxy support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BACKBONE_ROUTER_MULTICAST_ROUTING OT_BACKBONE_ROUTER_MULTICAST_ROUTING "Enable BBR MR support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BLE_TCAT OT_BLE_TCAT "Enable BLE TCAT support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BORDER_AGENT OT_BORDER_AGENT "Enable Border Agent")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BORDER_AGENT_EPHEMERAL_KEY_ENABLE OT_BORDER_AGENT_EPSKC "Border agent ephemeral PSKc")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BORDER_AGENT_ID OT_BORDER_AGENT_ID "Create and save border agent ID")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BORDER_ROUTER OT_BORDER_ROUTER "Enable Border Router")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BORDER_ROUTING OT_BORDER_ROUTING "Enable Border routing")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BORDER_ROUTING_COUNTERS OT_BORDER_ROUTING_COUNTERS "Enable Border routing counters")
kconfig_to_ot_option(CONFIG_OPENTHREAD_BORDER_ROUTING_DHCP6_PD OT_BORDER_ROUTING_DHCP6_PD "DHCPv6-PD support in border routing")
kconfig_to_ot_option(CONFIG_OPENTHREAD_CHANNEL_MANAGER OT_CHANNEL_MANAGER "Enable channel manager support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_CHANNEL_MANAGER_CSL OT_CHANNEL_MANAGER_CSL "Channel manager for CSL channel")
kconfig_to_ot_option(CONFIG_OPENTHREAD_CHANNEL_MONITOR OT_CHANNEL_MONITOR "Enable channel monitor support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_COAP OT_COAP "Enable CoAP API")
kconfig_to_ot_option(CONFIG_OPENTHREAD_COAP_BLOCK OT_COAP_BLOCK "Enable CoAP Block-wise option support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_COAP_OBSERVE OT_COAP_OBSERVE "Enable CoAP Observe option support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_COAPS OT_COAPS "Enable secure CoAP API support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_COMMISSIONER OT_COMMISSIONER "Enable Commissioner")
kconfig_to_ot_option(CONFIG_OPENTHREAD_CSL_AUTO_SYNC OT_CSL_AUTO_SYNC "Enable csl autosync")
kconfig_to_ot_option(CONFIG_OPENTHREAD_CSL_DEBUG OT_CSL_DEBUG "Enable CSL debug")
kconfig_to_ot_option(CONFIG_OPENTHREAD_CSL_RECEIVER OT_CSL_RECEIVER "Enable CSL receiver feature for Thread 1.2")
kconfig_to_ot_option(CONFIG_OPENTHREAD_CSL_RECEIVER_LOCAL_TIME_SYNC OT_CSL_RECEIVER_LOCAL_TIME_SYNC "Use local time for CSL sync")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DATASET_UPDATER OT_DATASET_UPDATER "Enable Dataset updater")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DEVICE_PROP_LEADER_WEIGHT OT_DEVICE_PROP_LEADER_WEIGHT "Enable device props for leader weight")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DHCP6_CLIENT OT_DHCP6_CLIENT "Enable DHCPv6 Client")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DHCP6_SERVER OT_DHCP6_SERVER "Enable DHCPv6 Server")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DIAG OT_DIAGNOSTIC "Enable Diagnostics support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DNS_CLIENT OT_DNS_CLIENT "Enable DNS client support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DNS_CLIENT_OVER_TCP OT_DNS_CLIENT_OVER_TCP "Enable dns query over tcp")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DNS_DSO OT_DNS_DSO "Enable DNS Stateful Operations (DSO) support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DNS_UPSTREAM_QUERY OT_DNS_UPSTREAM_QUERY "Enable forwarding DNS queries to upstream")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DNSSD_DISCOVERY_PROXY OT_DNSSD_DISCOVERY_PROXY "Enable DNS-SD discovery proxy support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DNSSD_SERVER OT_DNSSD_SERVER "Enable DNS-SD server support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DUA OT_DUA "Enable Domain Unicast Address feature for Thread 1.2")
kconfig_to_ot_option(CONFIG_OPENTHREAD_DYNAMIC_STORE_FRAME_AHEAD_COUNTER OT_DYNAMIC_STORE_FRAME_AHEAD_COUNTER "Enable dynamic store frame ahead counter")
kconfig_to_ot_option(CONFIG_OPENTHREAD_ECDSA OT_ECDSA "Enable ECDSA support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_ENABLE_SERVICE OT_SERVICE "Enable Service entries in Thread Network Data")
kconfig_to_ot_option(CONFIG_OPENTHREAD_EXTERNAL_HEAP OT_EXTERNAL_HEAP "Enable external heap support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_FIREWALL OT_FIREWALL "Enable firewall")
kconfig_to_ot_option(CONFIG_OPENTHREAD_FULL_LOGS OT_FULL_LOGS "Enable full logs")
kconfig_to_ot_option(CONFIG_OPENTHREAD_HISTORY_TRACKER OT_HISTORY_TRACKER "Enable history tracker support.")
kconfig_to_ot_option(CONFIG_OPENTHREAD_IP6_FRAGM OT_IP6_FRAGM "Enable IPv6 fragmentation support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_JAM_DETECTION OT_JAM_DETECTION "Enable Jam Detection")
kconfig_to_ot_option(CONFIG_OPENTHREAD_JOINER OT_JOINER "Enable Joiner")
kconfig_to_ot_option(CONFIG_OPENTHREAD_LEGACY OT_LEGACY "Enable legacy network support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_LINK_METRICS_INITIATOR OT_LINK_METRICS_INITIATOR "Enable Link Metrics initiator for Thread 1.2")
kconfig_to_ot_option(CONFIG_OPENTHREAD_LINK_METRICS_MANAGER OT_LINK_METRICS_MANAGER "Enable Link Metrics manager for Thread 1.2")
kconfig_to_ot_option(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT OT_LINK_METRICS_SUBJECT "Enable Link Metrics subject for Thread 1.2")
kconfig_to_ot_option(CONFIG_OPENTHREAD_PLATFORM_LOG_CRASH_DUMP OT_PLATFORM_LOG_CRASH_DUMP "Platform log crash dump")
kconfig_to_ot_option(CONFIG_OPENTHREAD_LOG_LEVEL_DYNAMIC OT_LOG_LEVEL_DYNAMIC "Enable dynamic log level control")
kconfig_to_ot_option(CONFIG_OPENTHREAD_MAC_FILTER OT_MAC_FILTER "Enable MAC filter support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_MULTICAST_DNS OT_MDNS "multicast DNS (mDNS)")
kconfig_to_ot_option(CONFIG_OPENTHREAD_MESH_DIAG OT_MESH_DIAG "Enable Mesh Diagnostics")
kconfig_to_ot_option(CONFIG_OPENTHREAD_MESSAGE_USE_HEAP OT_MESSAGE_USE_HEAP "Enable heap allocator for message buffers")
kconfig_to_ot_option(CONFIG_OPENTHREAD_MLE_LONG_ROUTES OT_MLE_LONG_ROUTES "Enable MLE long routes support (Experimental)")
kconfig_to_ot_option(CONFIG_OPENTHREAD_MLR OT_MLR "Enable Multicast Listener Registration feature for Thread 1.2")
kconfig_to_ot_option(CONFIG_OPENTHREAD_MULTIPAN_RCP OT_MULTIPAN_RCP "Enable Multi-PAN RCP")
kconfig_to_ot_option(CONFIG_OPENTHREAD_MULTIPLE_INSTANCE OT_MULTIPLE_INSTANCE "Enable multiple instances")
kconfig_to_ot_option(CONFIG_OPENTHREAD_NAT64_BORDER_ROUTING OT_NAT64_BORDER_ROUTING "Enable border routing NAT64 support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_NAT64_TRANSLATOR OT_NAT64_TRANSLATOR "Enable NAT64 translator")
kconfig_to_ot_option(CONFIG_OPENTHREAD_NEIGHBOR_DISCOVERY_AGENT OT_NEIGHBOR_DISCOVERY_AGENT "Enable neighbor discovery agent support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_NETDIAG_CLIENT OT_NETDIAG_CLIENT "Enable TMF network diagnostics on clients")
kconfig_to_ot_option(CONFIG_OPENTHREAD_NETDIAG_VENDOR_INFO OT_NETDIAG_VENDOR_INFO "Allow setting vendor info at runtime")
kconfig_to_ot_option(CONFIG_OPENTHREAD_NETDATA_PUBLISHER OT_NETDATA_PUBLISHER "Enable Thread Network Data publisher")
kconfig_to_ot_option(CONFIG_OPENTHREAD_OPERATIONAL_DATASET_AUTO_INIT OT_OPERATIONAL_DATASET_AUTO_INIT "Enable operational dataset auto init")
kconfig_to_ot_option(CONFIG_OPENTHREAD_OTNS OT_OTNS "Enable OTNS support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_PING_SENDER OT_PING_SENDER "Enable ping sender support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_PLATFORM_BOOTLOADER_MODE OT_PLATFORM_BOOTLOADER_MODE "Enable platform bootloader mode support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_PLATFORM_KEY_REF OT_PLATFORM_KEY_REF "Enable platform key reference support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_PLATFORM_NETIF OT_PLATFORM_NETIF "Enable platform netif support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_PLATFORM_UDP OT_PLATFORM_UDP "Enable platform UDP support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_RADIO_LINK_IEEE_802_15_4_ENABLE OT_15_4 "Enable 802.15.4 radio")
kconfig_to_ot_option(CONFIG_OPENTHREAD_RAW OT_LINK_RAW "Enable Link Raw")
kconfig_to_ot_option(CONFIG_OPENTHREAD_REFERENCE_DEVICE OT_REFERENCE_DEVICE "Enable Thread Certification Reference Device")
kconfig_to_ot_option(CONFIG_OPENTHREAD_SETTINGS_RAM OT_SETTINGS_RAM "Enable volatile-only storage of settings")
kconfig_to_ot_option(CONFIG_OPENTHREAD_SLAAC OT_SLAAC "Enable SLAAC")
kconfig_to_ot_option(CONFIG_OPENTHREAD_SNTP_CLIENT OT_SNTP_CLIENT "Enable SNTP Client support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_SRP_ADV_PROXY OT_SRP_ADV_PROXY "Enable SRP Server Advertising Proxy support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_SRP_CLIENT OT_SRP_CLIENT "Enable SRP Client support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_SRP_SERVER OT_SRP_SERVER "Enable SRP Server support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_TCP_ENABLE OT_TCP "Enable TCP support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_TIME_SYNC OT_TIME_SYNC "Enable the time synchronization service feature")
kconfig_to_ot_option(CONFIG_OPENTHREAD_TREL OT_TREL "Enable TREL radio link for Thread over Infrastructure feature")
kconfig_to_ot_option(CONFIG_OPENTHREAD_TX_BEACON_PAYLOAD OT_TX_BEACON_PAYLOAD "Enable tx beacon payload support")
kconfig_to_ot_option(CONFIG_OPENTHREAD_TX_QUEUE_STATISTICS OT_TX_QUEUE_STATS "Enable tx queue statistics")
kconfig_to_ot_option(CONFIG_OPENTHREAD_UDP_FORWARD OT_UDP_FORWARD "Enable UDP forward feature")
kconfig_to_ot_option(CONFIG_OPENTHREAD_UPTIME OT_UPTIME "Enable support for tracking OpenThread instance's uptime")
kconfig_to_ot_option(CONFIG_OPENTHREAD_VERHOEFF_CHECKSUM OT_VERHOEFF_CHECKSUM "Verhoeff checksum")
kconfig_to_ot_option(CONFIG_OPENTHREAD_WAKEUP_COORDINATOR OT_WAKEUP_COORDINATOR "Enable Wake-up Coordinator role")
kconfig_to_ot_option(CONFIG_OPENTHREAD_WAKEUP_END_DEVICE OT_WAKEUP_END_DEVICE "Enable Wake-up End Device role")

if(CONFIG_OPENTHREAD_COPROCESSOR_VENDOR_HOOK_SOURCE)
  set(OT_NCP_VENDOR_HOOK_SOURCE ${CONFIG_OPENTHREAD_COPROCESSOR_VENDOR_HOOK_SOURCE} CACHE STRING "NCP vendor hook source file name" FORCE)
endif()

if(CONFIG_OPENTHREAD_POWER_SUPPLY)
  set(OT_POWER_SUPPLY ${CONFIG_OPENTHREAD_POWER_SUPPLY} CACHE STRING "Power supply configuration" FORCE)
endif()

if (CONFIG_OPENTHREAD_CLI_VENDOR_EXTENSION)
  set(OT_CLI_VENDOR_EXTENSION ${CONFIG_OPENTHREAD_CLI_VENDOR_EXTENSION} CACHE STRING "Path to CMake file to define and link Openthread CLI vendor extension" FORCE)
endif()

set(BUILD_TESTING OFF CACHE BOOL "Disable openthread cmake testing targets" FORCE)

# Zephyr logging options

if(CONFIG_LOG_BACKEND_SPINEL)
  add_definitions(
      -DOPENTHREAD_CONFIG_LOG_OUTPUT=OPENTHREAD_CONFIG_LOG_OUTPUT_APP
  )
endif()

# Other options
add_definitions(
    -DOPENTHREAD_CONFIG_LOG_LEVEL=${CONFIG_OPENTHREAD_LOG_LEVEL}
    -DOPENTHREAD_PROJECT_CORE_CONFIG_FILE="openthread-core-zephyr-config.h"
)

# Need to specify build directory as well
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR} build)

zephyr_get_targets(${ZEPHYR_CURRENT_MODULE_DIR} "STATIC_LIBRARY;OBJECT_LIBRARY" ALL_TARGETS)
foreach(target ${ALL_TARGETS})
  # We don't want to build all openthread libraries per default.
  # Setting EXCLUDE_FROM_ALL ensures that only libraries that are linked
  # into Zephyr will be built due to dependencies.
  set_property(TARGET ${target} PROPERTY EXCLUDE_FROM_ALL TRUE)
endforeach()

string(REPLACE " " ";" OT_PARAM_LIST " ${CONFIG_OPENTHREAD_CUSTOM_PARAMETERS}")
target_compile_definitions(ot-config INTERFACE ${OT_PARAM_LIST})

# Since Mbed TLS 3.1.0 MBEDTLS_SSL_EXPORT_KEYS was removed as build symbol and
# it's always assumed to be enabled. Corresponding kconfig was removed from
# Zephyr as well, but OpenThread code still uses it, so we add it here.
target_compile_definitions(ot-config INTERFACE -DMBEDTLS_SSL_EXPORT_KEYS)

# Zephyr compiler options
target_include_directories(ot-config INTERFACE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>
)

target_include_directories(ot-config SYSTEM INTERFACE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
)

target_compile_definitions(ot-config INTERFACE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>
)

# Openthread can use minimal libc, which requires autoconf.h
# (specifically CONFIG_ARM and friends). autoconf.h can't be included
# through openthread-*-config.h because openthread third-party
# libraries do not include this header. So we add the defines to all
# OpenThread files through the gcc flag -imacros instead.
target_compile_options(ot-config INTERFACE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>
    $<TARGET_PROPERTY:compiler,no_builtin>
    -imacros ${AUTOCONF_H}
)

# Openthread depends on errno.h, which includes errno_private.h in minimal libc.
# errno_private.h is generated as part of ${SYSCALL_LIST_H_TARGET} target.
add_dependencies(ot-config ${SYSCALL_LIST_H_TARGET})

# Make sure C library, in case of newlib, is linked after OpenThread libraries
# (to prevent linker errors)
if(CONFIG_NEWLIB_LIBC)
  target_link_libraries(ot-config INTERFACE -lc)
endif()

# Include OpenThread headers
zephyr_system_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)
zephyr_system_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/examples/platforms)

# Determine which libs should be linked in
set(ot_libs "")

if(CONFIG_OPENTHREAD_FTD)
set(cli_lib openthread-cli-ftd)
elseif(CONFIG_OPENTHREAD_MTD)
set(cli_lib openthread-cli-mtd)
endif()

if(CONFIG_OPENTHREAD_SHELL)
list(APPEND ot_libs ${cli_lib})
endif()

if(CONFIG_OPENTHREAD_COPROCESSOR_RCP)
list(APPEND ot_libs openthread-rcp)
endif()

if(CONFIG_OPENTHREAD_COPROCESSOR_NCP)
if(CONFIG_OPENTHREAD_FTD)
list(APPEND ot_libs openthread-ncp-ftd)
elseif(CONFIG_OPENTHREAD_MTD)
list(APPEND ot_libs openthread-ncp-mtd)
endif()
endif()

if(NOT CONFIG_OPENTHREAD_COPROCESSOR_RCP)
if(CONFIG_OPENTHREAD_FTD)
list(APPEND ot_libs openthread-ftd)
elseif(CONFIG_OPENTHREAD_MTD)
list(APPEND ot_libs openthread-mtd)
endif()
endif()

if(CONFIG_HDLC_RCP_IF)
list(APPEND ot_libs
  ot-config
  openthread-platform
  openthread-radio-spinel
  openthread-spinel-ncp
  openthread-url
  openthread-hdlc
)
endif()

if(CONFIG_OPENTHREAD_SETTINGS_RAM)
  target_compile_options(openthread-platform-utils PRIVATE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>
    $<TARGET_PROPERTY:compiler,no_builtin>)
  add_dependencies(openthread-platform-utils syscall_list_h_target)

  list(APPEND ot_libs openthread-platform-utils-static)
endif()

zephyr_link_libraries(${ot_libs})

endif()

add_subdirectory(platform)

endif()
