# Copyright 2023 Linaro
# Copyright (c) 2026 Antmicro <www.antmicro.com>
# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_INSTRUMENTATION)

  zephyr_library()

  zephyr_library_sources(common/instr_common.c)
  zephyr_library_sources(timestamp/timestamp.c)

  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    zephyr_library_sources(handlers/instr_handlers_gcc.c)
  else()
    message(WARNING "CONFIG_INSTRUMENTATION unsupported by this compiler: ${CMAKE_C_COMPILER_ID}.")
  endif()

  if(CONFIG_INSTRUMENTATION_BACKEND_UART)
    zephyr_library_sources(transport/uart.c)

    if(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH)
      zephyr_library_sources(ringbuffer/ringbuffer.c)
    endif()

    configure_file(${CMAKE_CURRENT_LIST_DIR}/ctf/metadata.template ${CMAKE_BINARY_DIR}/ctf_metadata @ONLY)
  elseif(CONFIG_INSTRUMENTATION_BACKEND_TRACING_CORE)
    zephyr_library_sources(transport/tracing.c)
  endif()

  zephyr_compile_definitions_ifdef(CONFIG_INSTRUMENTATION INSTR_TRIGGER_FUNCTION=${CONFIG_INSTRUMENTATION_TRIGGER_FUNCTION})
  zephyr_compile_definitions_ifdef(CONFIG_INSTRUMENTATION INSTR_STOPPER_FUNCTION=${CONFIG_INSTRUMENTATION_STOPPER_FUNCTION})
  zephyr_include_directories_ifdef(CONFIG_INSTRUMENTATION include)
  zephyr_include_directories_ifdef(CONFIG_INSTRUMENTATION ${ZEPHYR_BASE}/kernel/include)

endif()
