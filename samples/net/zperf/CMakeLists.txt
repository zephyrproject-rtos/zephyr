macro(set_conf_file)
  if(PROFILER)
    set(CONF_FILE prj_${BOARD}_prof.conf)
  else()
    set(CONF_FILE prj_${BOARD}.conf)
  endif()
endmacro()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(
  app PRIVATE
  src/shell_utils.c
  src/zperf_session.c
  src/zperf_shell.c
  )
target_sources_ifdef(
  CONFIG_NET_UDP
  app PRIVATE
  src/zperf_udp_receiver.c
  src/zperf_udp_uploader.c
  )
target_sources_ifdef(
  CONFIG_NET_TCP
  app PRIVATE
  src/zperf_tcp_receiver.c
  src/zperf_tcp_uploader.c
  )

target_compile_definitions_ifdef(
  PROFILER
  app PRIVATE
  PROFILER
  )

target_include_directories(app PRIVATE
  $ENV{ZEPHYR_BASE}/subsys/net/ip
  $ENV{ZEPHYR_BASE}/samples/task_profiler/profiler/src
  )

if(PROFILER)
  assert(0 "PROFILER is not supported yet")
  # KBuild did this, but this did not work, not sure why.
  # export PROFILER_NO_SHELL_REGISTER=1
  # obj-y += ../../../task_profiler/profiler/
endif()
