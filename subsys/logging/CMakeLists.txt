# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_LOG_MINIMAL)
  zephyr_sources_ifdef(
    CONFIG_LOG
    log_list.c
    log_core.c
    log_msg.c
    log_output.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG2
    log_msg2.c
  )


  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_UART
    log_backend_uart.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_FS
    log_backend_fs.c
  )

  zephyr_link_libraries_ifdef(CONFIG_LOG_BACKEND_FS LITTLEFS)

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

  if(CONFIG_LOG_BACKEND_SPINEL)
    zephyr_library_include_directories(
	    ${ZEPHYR_BASE}/subsys/net/lib/openthread/platform/
    )
  endif()

  zephyr_sources_ifdef(
    CONFIG_LOG_BACKEND_SPINEL
    log_backend_spinel.c
  )

  if(CONFIG_LOG_DICTIONARY_SUPPORT)
    zephyr_sources(log_output_dict.c)
  endif()

else()
  zephyr_sources(log_minimal.c)
endif()
