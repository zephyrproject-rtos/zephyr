# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_LOG_MODE_MINIMAL)
  zephyr_sources_ifdef(
    CONFIG_LOG
    log_core.c
    log_mgmt.c
    log_msg.c
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

  zephyr_sources_ifdef(
    CONFIG_LOG_MIPI_SYST_ENABLE
    log_output_syst.c
  )

  add_subdirectory(backends)

  if(CONFIG_LOG_CUSTOM_FORMAT_SUPPORT)
    zephyr_sources(log_output_custom.c)
  endif()

else()
  zephyr_sources(log_minimal.c)
endif()
