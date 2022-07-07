# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_LOG_MODE_MINIMAL)
  zephyr_sources_ifdef(
    CONFIG_LOG
    log_core.c
    log_mgmt.c
    log_msg.c
    log_output.c
  )

  # Determine if __auto_type is supported. If not then runtime approach must always
  # be used.
  # Supported by:
  # - C++ (auto)
  # - GCC 4.9.0 https://gcc.gnu.org/gcc-4.9/changes.html
  # - Clang 3.8
  if (NOT CONFIG_LOG_ALWAYS_RUNTIME)
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
      if(CMAKE_C_COMPILER_VERSION VERSION_LESS "3.8.0")
        message(WARNING "Compiler version requires CONFIG_LOG_ALWAYS_RUNTIME to be set")
      endif()
    endif()
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
      if(CMAKE_C_COMPILER_VERSION VERSION_LESS "4.9.0")
        message(WARNING "Compiler version requires CONFIG_LOG_ALWAYS_RUNTIME to be set")
      endif()
    endif()
  endif()

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_UART
    log_backend_uart.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_FS
    log_backend_fs.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_CMDS
    log_cmds.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_NATIVE_POSIX
    log_backend_native_posix.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_XTENSA_SIM
    log_backend_xtensa_sim.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_NET
    log_backend_net.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_RTT
    log_backend_rtt.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_SWO
    log_backend_swo.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_MIPI_SYST_ENABLE
    log_output_syst.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_ADSP
    log_backend_adsp.c
    )

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_CAVS_HDA
    log_backend_cavs_hda.c
  )

  if(CONFIG_LOG_BACKEND_SPINEL)
    zephyr_library_include_directories(
	    ${ZEPHYR_BASE}/subsys/net/lib/openthread/platform/
    )
  endif()

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_SPINEL
    log_backend_spinel.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_FRONTEND_DICT_UART
    log_frontend_dict_uart.c
  )

  if(CONFIG_LOG_DICTIONARY_SUPPORT)
    zephyr_sources(log_output_dict.c)
  endif()

else()
  zephyr_sources(log_minimal.c)
endif()
