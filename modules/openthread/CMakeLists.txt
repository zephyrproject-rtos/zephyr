if(CONFIG_OPENTHREAD)
if(CONFIG_OPENTHREAD_SOURCES)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# OpenThread options
set(OT_BUILD_EXECUTABLES OFF CACHE BOOL "Disable OpenThread samples")
set(OT_BUILTIN_MBEDTLS_MANAGEMENT OFF CACHE BOOL "Use Zephyr's mbedTLS heap")
set(OT_PLATFORM "zephyr" CACHE STRING "Zephyr as a target platform")
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

if(CONFIG_ASSERT)
  set(OT_ASSERT ON CACHE BOOL "Enable assert function OT_ASSERT()" FORCE)
else()
  set(OT_ASSERT OFF CACHE BOOL "Enable assert function OT_ASSERT()" FORCE)
endif()

if(CONFIG_OPENTHREAD_BACKBONE_ROUTER)
  set(OT_BACKBONE_ROUTER ON CACHE BOOL "Enable backbone router functionality" FORCE)
else()
  set(OT_BACKBONE_ROUTER OFF CACHE BOOL "Enable backbone router functionality" FORCE)
endif()

if(CONFIG_OPENTHREAD_BORDER_AGENT)
  set(OT_BORDER_AGENT ON CACHE BOOL "Enable Border Agent" FORCE)
else()
  set(OT_BORDER_AGENT OFF CACHE BOOL "Enable Border Agent" FORCE)
endif()

if(CONFIG_OPENTHREAD_BORDER_ROUTER)
  set(OT_BORDER_ROUTER ON CACHE BOOL "Enable Border Router" FORCE)
else()
  set(OT_BORDER_ROUTER OFF CACHE BOOL "Enable Border Router" FORCE)
endif()

if(CONFIG_OPENTHREAD_BORDER_ROUTING_NAT64)
  set(OT_BORDER_ROUTING_NAT64 ON CACHE BOOL "Enable border routing NAT64 support" FORCE)
else()
  set(OT_BORDER_ROUTING_NAT64 OFF CACHE BOOL "Enable border routing NAT64 support" FORCE)
endif()

if(CONFIG_OPENTHREAD_COAP)
  set(OT_COAP ON CACHE BOOL "Enable CoAP API" FORCE)
else()
  set(OT_COAP OFF CACHE BOOL "Enable CoAP API" FORCE)
endif()

if(CONFIG_OPENTHREAD_COAPS)
  set(OT_COAPS ON CACHE BOOL "Enable secure CoAP API support" FORCE)
else()
  set(OT_COAPS OFF CACHE BOOL "Enable secure CoAP API support" FORCE)
endif()

if(CONFIG_OPENTHREAD_COAP_BLOCK)
  set(OT_COAP_BLOCK ON CACHE BOOL "Enable CoAP Block-wise option support" FORCE)
else()
  set(OT_COAP_BLOCK OFF CACHE BOOL "Enable CoAP Block-wise option support" FORCE)
endif()

if(CONFIG_OPENTHREAD_COAP_OBSERVE)
  set(OT_COAP_OBSERVE ON CACHE BOOL "Enable CoAP Observe option support" FORCE)
else()
  set(OT_COAP_OBSERVE OFF CACHE BOOL "Enable CoAP Observe option support" FORCE)
endif()

if(CONFIG_OPENTHREAD_COMMISSIONER)
  set(OT_COMMISSIONER ON CACHE BOOL "Enable Commissioner" FORCE)
else()
  set(OT_COMMISSIONER OFF CACHE BOOL "Enable Commissioner" FORCE)
endif()

if(CONFIG_OPENTHREAD_CHANNEL_MANAGER)
  set(OT_CHANNEL_MANAGER ON CACHE BOOL "Enable channel manager support" FORCE)
else()
  set(OT_CHANNEL_MANAGER OFF CACHE BOOL "Enable channel manager support" FORCE)
endif()

if(CONFIG_OPENTHREAD_CHANNEL_MONITOR)
  set(OT_CHANNEL_MONITOR ON CACHE BOOL "Enable channel monitor support" FORCE)
else()
  set(OT_CHANNEL_MONITOR OFF CACHE BOOL "Enable channel monitor support" FORCE)
endif()

if(CONFIG_OPENTHREAD_CHILD_SUPERVISION)
  set(OT_CHILD_SUPERVISION ON CACHE BOOL "Enable child supervision support" FORCE)
else()
  set(OT_CHILD_SUPERVISION OFF CACHE BOOL "Enable child supervision support" FORCE)
endif()

if(CONFIG_OPENTHREAD_CSL_AUTO_SYNC)
  set(OT_CSL_AUTO_SYNC ON CACHE BOOL "Enable csl autosync" FORCE)
else()
  set(OT_CSL_AUTO_SYNC OFF CACHE BOOL "Enable csl autosync" FORCE)
endif()

if(CONFIG_OPENTHREAD_CSL_RECEIVER)
  set(OT_CSL_RECEIVER ON CACHE BOOL "Enable CSL receiver feature for Thread 1.2" FORCE)
else()
  set(OT_CSL_RECEIVER OFF CACHE BOOL "Enable CSL receiver feature for Thread 1.2" FORCE)
endif()

if(CONFIG_OPENTHREAD_CSL_DEBUG)
  set(OT_CSL_DEBUG ON CACHE BOOL "Enable CSL debug" FORCE)
else()
  set(OT_CSL_DEBUG OFF CACHE BOOL "Enable CSL debug" FORCE)
endif()

if(CONFIG_OPENTHREAD_DATSET_UPDATER)
  set(OT_DATASET_UPDATER ON CACHE BOOL "Enable Dataset updater" FORCE)
else()
  set(OT_DATASET_UPDATER OFF CACHE BOOL "Enable Dataset updater" FORCE)
endif()

if(CONFIG_OPENTHREAD_DHCP6_CLIENT)
  set(OT_DHCP6_CLIENT ON CACHE BOOL "Enable DHCPv6 Client" FORCE)
else()
  set(OT_DHCP6_CLIENT OFF CACHE BOOL "Enable DHCPv6 Client" FORCE)
endif()

if(CONFIG_OPENTHREAD_DHCP6_SERVER)
  set(OT_DHCP6_SERVER ON CACHE BOOL "Enable DHCPv6 Server" FORCE)
else()
  set(OT_DHCP6_SERVER OFF CACHE BOOL "Enable DHCPv6 Server" FORCE)
endif()

if(CONFIG_OPENTHREAD_DIAG)
  set(OT_DIAGNOSTIC ON CACHE BOOL "Enable Diagnostics support" FORCE)
else()
  set(OT_DIAGNOSTIC OFF CACHE BOOL "Enable Diagnostics support" FORCE)
endif()

if(CONFIG_OPENTHREAD_DNS_CLIENT)
  set(OT_DNS_CLIENT ON CACHE BOOL "Enable DNS client support" FORCE)
else()
  set(OT_DNS_CLIENT OFF CACHE BOOL "Enable DNS client support" FORCE)
endif()

if(CONFIG_OPENTHREAD_DNS_DSO)
  set(OT_DNS_DSO ON CACHE BOOL "Enable DNS Stateful Operations (DSO) support" FORCE)
else()
  set(OT_DNS_DSO OFF CACHE BOOL "Enable DNS Stateful Operations (DSO) support" FORCE)
endif()

if(CONFIG_OPENTHREAD_DNSSD_SERVER)
  set(OT_DNSSD_SERVER ON CACHE BOOL "Enable DNS-SD server support" FORCE)
else()
  set(OT_DNSSD_SERVER OFF CACHE BOOL "Enable DNS-SD server support" FORCE)
endif()

if(CONFIG_OPENTHREAD_DUA)
  set(OT_DUA ON CACHE BOOL "Enable Domain Unicast Address feature for Thread 1.2" FORCE)
else()
  set(OT_DUA OFF CACHE BOOL "Enable Domain Unicast Address feature for Thread 1.2" FORCE)
endif()

if(CONFIG_OPENTHREAD_ECDSA)
  set(OT_ECDSA ON CACHE BOOL "Enable ECDSA support" FORCE)
else()
  set(OT_ECDSA OFF CACHE BOOL "Enable ECDSA support" FORCE)
endif()

if(CONFIG_OPENTHREAD_EXCLUDE_TCPLP_LIB)
  set(OT_EXCLUDE_TCPLP_LIB ON CACHE BOOL "Exclude TCPlp library from build" FORCE)
else()
  set(OT_EXCLUDE_TCPLP_LIB OFF CACHE BOOL "Exclude TCPlp library from build" FORCE)
endif()

if(CONFIG_OPENTHREAD_ENABLE_SERVICE)
  set(OT_SERVICE ON CACHE BOOL "Enable Service entries in Thread Network Data" FORCE)
else()
  set(OT_SERVICE OFF CACHE BOOL "Enable Service entries in Thread Network Data" FORCE)
endif()

if(CONFIG_OPENTHREAD_EXTERNAL_HEAP)
  set(OT_EXTERNAL_HEAP ON CACHE BOOL "Enable external heap support" FORCE)
else()
  set(OT_EXTERNAL_HEAP OFF CACHE BOOL "Enable external heap support" FORCE)
endif()

if(CONFIG_OPENTHREAD_HISTORY_TRACKER)
  set(OT_HISTORY_TRACKER ON CACHE BOOL "Enable history tracker support." FORCE)
else()
  set(OT_HISTORY_TRACKER OFF CACHE BOOL "Enable history tracker support." FORCE)
endif()

if(CONFIG_OPENTHREAD_IP6_FRAGM)
  set(OT_IP6_FRAGM ON CACHE BOOL "Enable IPv6 fragmentation support" FORCE)
else()
  set(OT_IP6_FRAGM OFF CACHE BOOL "Enable IPv6 fragmentation support" FORCE)
endif()

if(CONFIG_OPENTHREAD_JAM_DETECTION)
  set(OT_JAM_DETECTION ON CACHE BOOL "Enable Jam Detection" FORCE)
else()
  set(OT_JAM_DETECTION OFF CACHE BOOL "Enable Jam Detection" FORCE)
endif()

if(CONFIG_OPENTHREAD_JOINER)
  set(OT_JOINER ON CACHE BOOL "Enable Joiner" FORCE)
else()
  set(OT_JOINER OFF CACHE BOOL "Enable Joiner" FORCE)
endif()

if(CONFIG_OPENTHREAD_LEGACY)
  set(OT_LEGACY ON CACHE BOOL "Enable legacy network support" FORCE)
else()
  set(OT_LEGACY OFF CACHE BOOL "Enable legacy network support" FORCE)
endif()

if(CONFIG_OPENTHREAD_LINK_METRICS_INITIATOR)
  set(OT_LINK_METRICS_INITIATOR ON CACHE BOOL "Enable Link Metrics initiator for Thread 1.2" FORCE)
else()
  set(OT_LINK_METRICS_INITIATOR OFF CACHE BOOL "Enable Link Metrics initiator for Thread 1.2" FORCE)
endif()

if(CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT)
  set(OT_LINK_METRICS_SUBJECT ON CACHE BOOL "Enable Link Metrics subject for Thread 1.2" FORCE)
else()
  set(OT_LINK_METRICS_SUBJECT OFF CACHE BOOL "Enable Link Metrics subject for Thread 1.2" FORCE)
endif()

if(CONFIG_OPENTHREAD_LOG_LEVEL_DYNAMIC)
  set(OT_LOG_LEVEL_DYNAMIC ON CACHE BOOL "Enable dynamic log level control" FORCE)
else()
  set(OT_LOG_LEVEL_DYNAMIC OFF CACHE BOOL "Enable dynamic log level control" FORCE)
endif()

if(CONFIG_OPENTHREAD_MAC_FILTER)
  set(OT_MAC_FILTER ON CACHE BOOL "Enable MAC filter support" FORCE)
else()
  set(OT_MAC_FILTER OFF CACHE BOOL "Enable MAC filter support" FORCE)
endif()

if(CONFIG_OPENTHREAD_MLE_LONG_ROUTES)
  set(OT_MLE_LONG_ROUTES ON CACHE BOOL "Enable MLE long routes support (Experimental)" FORCE)
else()
  set(OT_MLE_LONG_ROUTES OFF CACHE BOOL "Enable MLE long routes support (Experimental)" FORCE)
endif()

if(CONFIG_OPENTHREAD_MLR)
  set(OT_MLR ON CACHE BOOL "Enable Multicast Listener Registration feature for Thread 1.2" FORCE)
else()
  set(OT_MLR OFF CACHE BOOL "Enable Multicast Listener Registration feature for Thread 1.2" FORCE)
endif()

if(CONFIG_OPENTHREAD_MTD_NETDIAG)
  set(OT_MTD_NETDIAG ON CACHE BOOL "Enable TMF network diagnostics on MTDs" FORCE)
else()
  set(OT_MTD_NETDIAG OFF CACHE BOOL "Enable TMF network diagnostics on MTDs" FORCE)
endif()

if(CONFIG_OPENTHREAD_MULTIPLE_INSTANCE)
  set(OT_MULTIPLE_INSTANCE ON CACHE BOOL "Enable multiple instances" FORCE)
else()
  set(OT_MULTIPLE_INSTANCE OFF CACHE BOOL "Enable multiple instances" FORCE)
endif()

if(CONFIG_OPENTHREAD_NEIGHBOR_DISCOVERY_AGENT)
  set(OT_NEIGHBOR_DISCOVERY_AGENT ON CACHE BOOL "Enable neighbor discovery agent support" FORCE)
else()
  set(OT_NEIGHBOR_DISCOVERY_AGENT OFF CACHE BOOL "Enable neighbor discovery agent support" FORCE)
endif()

if(CONFIG_OPENTHREAD_NETDATA_PUBLISHER)
  set(OT_NETDATA_PUBLISHER ON CACHE BOOL "Enable Thread Network Data publisher" FORCE)
else()
  set(OT_NETDATA_PUBLISHER OFF CACHE BOOL "Enable Thread Network Data publisher" FORCE)
endif()

if(CONFIG_OPENTHREAD_PING_SENDER)
  set(OT_PING_SENDER ON CACHE BOOL "Enable ping sender support" FORCE)
else()
  set(OT_PING_SENDER OFF CACHE BOOL "Enable ping sender support" FORCE)
endif()

if(CONFIG_OPENTHREAD_PLATFORM_NETIF)
  set(OT_PLATFORM_NETIF ON CACHE BOOL "Enable platform netif support" FORCE)
else()
  set(OT_PLATFORM_NETIF OFF CACHE BOOL "Enable platform netif support" FORCE)
endif()

if(CONFIG_OPENTHREAD_PLATFORM_UDP)
  set(OT_PLATFORM_UDP ON CACHE BOOL "Enable platform UDP support" FORCE)
else()
  set(OT_PLATFORM_UDP OFF CACHE BOOL "Enable platform UDP support" FORCE)
endif()

if(CONFIG_OPENTHREAD_RAW)
  set(OT_LINK_RAW ON CACHE BOOL "Enable Link Raw" FORCE)
else()
  set(OT_LINK_RAW OFF CACHE BOOL "Enable Link Raw" FORCE)
endif()

if(CONFIG_OPENTHREAD_REFERENCE_DEVICE)
  set(OT_REFERENCE_DEVICE ON CACHE BOOL "Enable Thread Certification Reference Device" FORCE)
else()
  set(OT_REFERENCE_DEVICE OFF CACHE BOOL "Enable Thread Certification Reference Device" FORCE)
endif()

if(CONFIG_OPENTHREAD_SETTINGS_RAM)
  set(OT_SETTINGS_RAM ON CACHE BOOL "Enable volatile-only storage of settings" FORCE)
else()
  set(OT_SETTINGS_RAM OFF CACHE BOOL "Enable volatile-only storage of settings" FORCE)
endif()

if(CONFIG_OPENTHREAD_SLAAC)
  set(OT_SLAAC ON CACHE BOOL "Enable SLAAC" FORCE)
else()
  set(OT_SLAAC OFF CACHE BOOL "Enable SLAAC" FORCE)
endif()

if(CONFIG_OPENTHREAD_SNTP_CLIENT)
  set(OT_SNTP_CLIENT ON CACHE BOOL "Enable SNTP Client support" FORCE)
else()
  set(OT_SNTP_CLIENT OFF CACHE BOOL "Enable SNTP Client support" FORCE)
endif()

if(CONFIG_OPENTHREAD_SRP_CLIENT)
  set(OT_SRP_CLIENT ON CACHE BOOL "Enable SRP Client support" FORCE)
else()
  set(OT_SRP_CLIENT OFF CACHE BOOL "Enable SRP Client support" FORCE)
endif()

if(CONFIG_OPENTHREAD_SRP_SERVER)
  set(OT_SRP_SERVER ON CACHE BOOL "Enable SRP Server support" FORCE)
else()
  set(OT_SRP_SERVER OFF CACHE BOOL "Enable SRP Server support" FORCE)
endif()

if(CONFIG_OPENTHREAD_TREL)
  set(OT_TREL ON CACHE BOOL "Enable TREL radio link for Thread over Infrastructure feature" FORCE)
else()
  set(OT_TREL OFF CACHE BOOL "Enable TREL radio link for Thread over Infrastructure feature" FORCE)
endif()

if(CONFIG_OPENTHREAD_TIME_SYNC)
  set(OT_TIME_SYNC ON CACHE BOOL "Enable the time synchronization service feature" FORCE)
else()
  set(OT_TIME_SYNC OFF CACHE BOOL "Enable the time synchronization service feature" FORCE)
endif()

if(CONFIG_OPENTHREAD_UDP_FORWARD)
  set(OT_UDP_FORWARD ON CACHE BOOL "Enable UDP forward feature" FORCE)
else()
  set(OT_UDP_FORWARD OFF CACHE BOOL "Enable UDP forward feature" FORCE)
endif()

if(CONFIG_OPENTHREAD_UPTIME)
  set(OT_UPTIME ON CACHE BOOL "Enable support for tracking OpenThread instance's uptime" FORCE)
else()
  set(OT_UPTIME OFF CACHE BOOL "Enable support for tracking OpenThread instance's uptime" FORCE)
endif()

if(CONFIG_OPENTHREAD_OTNS)
  set(OT_OTNS ON CACHE BOOL "Enable OTNS support" FORCE)
else()
  set(OT_OTNS OFF CACHE BOOL "Enable OTNS support" FORCE)
endif()

if(CONFIG_OPENTHREAD_FULL_LOGS)
  set(OT_FULL_LOGS ON CACHE BOOL "Enable full logs" FORCE)
else()
  set(OT_FULL_LOGS OFF CACHE BOOL "Enable full logs" FORCE)
endif()

if(CONFIG_OPENTHREAD_COPROCESSOR_VENDOR_HOOK_SOURCE)
  set(OT_NCP_VENDOR_HOOK_SOURCE ${CONFIG_OPENTHREAD_COPROCESSOR_VENDOR_HOOK_SOURCE} CACHE STRING "NCP vendor hook source file name" FORCE)
endif()

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
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS> -fno-builtin
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

if(CONFIG_OPENTHREAD_SETTINGS_RAM)
  target_compile_options(openthread-platform-utils PRIVATE
    $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS> -fno-builtin)
  add_dependencies(openthread-platform-utils syscall_list_h_target)

  list(APPEND ot_libs openthread-platform-utils-static)
endif()

zephyr_link_libraries(${ot_libs})

endif()

add_subdirectory(platform)

endif()
