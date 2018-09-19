add_library(subsys_net STATIC "")

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_NET_BUF}             buf.c
            IFDEF:${CONFIG_NET_HOSTNAME_ENABLE} hostname.c
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})

if(CONFIG_NETWORKING)
  include_relative(l2/CMakeLists.txt)

  if(CONFIG_NET_RAW_MODE)
    target_sources(subsys_net PRIVATE ${CMAKE_CURRENT_LIST_DIR}/ip/net_pkt.c)
  else()
    include_relative(ip/CMakeLists.txt)
  endif()
endif()

include_relative(lib/CMakeLists.txt)

target_link_libraries(subsys_net PRIVATE zephyr_interface)
target_link_libraries(zephyr_app_linking INTERFACE subsys_net)
zephyr_set_property(TARGET subsys_net MEMORY_SPACE kernel)

if(CONFIG_MBEDTLS)
  target_link_libraries(mbedTLS INTERFACE zephyr_interface)
  target_link_libraries(subsys_net PUBLIC mbedTLS)
endif()

if(CONFIG_NEWLIB_LIBC)
  target_compile_definitions(subsys_net PRIVATE __LINUX_ERRNO_EXTENSIONS__)
endif()
