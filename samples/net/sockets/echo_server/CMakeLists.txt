# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sockets_echo_server)

if(CONFIG_NET_SOCKETS_SOCKOPT_TLS AND
   CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED AND
   (CONFIG_NET_SAMPLE_PSK_HEADER_FILE STREQUAL "dummy_psk.h"))
  add_custom_target(development_psk
    COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------------------------------"
    COMMAND ${CMAKE_COMMAND} -E echo "--- WARNING: Using dummy PSK! Only suitable for        ---"
    COMMAND ${CMAKE_COMMAND} -E echo "--- development. Set NET_SAMPLE_PSK_HEADER_FILE to use ---"
    COMMAND ${CMAKE_COMMAND} -E echo "--- own pre-shared key.                                ---"
    COMMAND ${CMAKE_COMMAND} -E echo "----------------------------------------------------------"
  )
  add_dependencies(app development_psk)
endif()

target_sources(                     app PRIVATE src/echo-server.c)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE src/udp.c)
target_sources_ifdef(CONFIG_NET_TCP app PRIVATE src/tcp.c)

if (CONFIG_USB_DEVICE_STACK)
  target_sources(app PRIVATE src/usb.c)
endif()

if (CONFIG_USB_DEVICE_STACK_NEXT)
  target_sources(app PRIVATE src/usb.c)
  include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)
endif()

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(inc_file
	ca.der
	server.der
	server_privkey.der
	echo-apps-cert.der
	echo-apps-key.der
    )
  generate_inc_file_for_target(
    app
    src/${inc_file}
    ${gen_dir}/${inc_file}.inc
    )
endforeach()

if(CONFIG_NET_SAMPLE_WEBSOCKET_CONSOLE)
  zephyr_linker_sources(SECTIONS src/ws_console/sections-rom.ld)
  target_sources(app PRIVATE src/ws_console/ws.c)
  generate_inc_file_for_target(app src/ws_console/index.html ${gen_dir}/index.html.gz.inc --gzip)
  generate_inc_file_for_target(app src/ws_console/style.css ${gen_dir}/style.css.gz.inc --gzip)
  generate_inc_file_for_target(app src/ws_console/favicon-16x16.png ${gen_dir}/favicon-16x16.png.gz.inc --gzip)
endif()
if(CONFIG_NET_SAMPLE_HTTPS_SERVICE)
  zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_WEBSOCKET_CONSOLE NAME
			      http_resource_desc_wss_console_service
			      KVMA RAM_REGION GROUP RODATA_REGION
			      SUBALIGN Z_LINK_ITERABLE_SUBALIGN)
endif()
zephyr_linker_section_ifdef(CONFIG_NET_SAMPLE_WEBSOCKET_CONSOLE NAME
			    http_resource_desc_ws_console_service
			    KVMA RAM_REGION GROUP RODATA_REGION
			    SUBALIGN Z_LINK_ITERABLE_SUBALIGN)
