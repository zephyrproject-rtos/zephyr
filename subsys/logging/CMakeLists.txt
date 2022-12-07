# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_LOG_MODE_MINIMAL)
  zephyr_sources_ifdef(
    CONFIG_LOG
    log_core.c
    log_mgmt.c
    log_cache.c
    log_msg.c
    log_cache.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_OUTPUT
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
    CONFIG_LOG_CMDS
    log_cmds.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_FRONTEND_DICT_UART
    log_frontend_dict_uart.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_DICTIONARY_SUPPORT
    log_output_dict.c
  )

  add_subdirectory(backends)

  # For some reason, running sys-t with catalog message on
  # Cortex-M0 would result in hard fault in mipi_catalog_formatter()
  # if this is complied before the backends (only with SIZE
  # optimization). Workaround the issue by placing it here instead
  # as this is causing CI failures. Actual root-cause is TBD.
  zephyr_sources_ifdef(
    CONFIG_LOG_MIPI_SYST_ENABLE
    log_output_syst.c
  )

  if(CONFIG_LOG_CUSTOM_FORMAT_SUPPORT)
    zephyr_sources(log_output_custom.c)
  endif()

  zephyr_sources_ifdef(
    CONFIG_LOG_MULTIDOMAIN_LINK
    log_multidomain_link.c
  )

  zephyr_sources_ifdef(
    CONFIG_LOG_LINK_IPC_SERVICE
    log_link_ipc_service.c
  )


else()
  zephyr_sources(log_minimal.c)
endif()
