cmake_minimum_required(VERSION 3.13.1)

macro(set_conf_file)
  if(EXISTS ${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf)
    set(CONF_FILE "${APPLICATION_SOURCE_DIR}/prj_${BOARD}.conf")
  elseif(EXISTS ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    set(CONF_FILE
      "prj.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf")
  else()
    set(CONF_FILE "prj.conf")
  endif()
endmacro()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(zperf)

target_sources(app PRIVATE
  src/shell_utils.c
  src/zperf_session.c
  src/zperf_shell.c
  src/zperf_udp_receiver.c
  src/zperf_udp_uploader.c
  src/zperf_tcp_receiver.c
  src/zperf_tcp_uploader.c
  )

target_include_directories(app PRIVATE
  $ENV{ZEPHYR_BASE}/subsys/net/ip
  )
