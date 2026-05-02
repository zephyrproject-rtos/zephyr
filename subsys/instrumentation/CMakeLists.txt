# Copyright 2023 Linaro
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_INSTRUMENTATION
  common/instr_common.c
  timestamp/timestamp.c
  transport/uart.c
)

zephyr_sources_ifdef(CONFIG_INSTRUMENTATION_MODE_CALLGRAPH ringbuffer/ringbuffer.c)

if(CONFIG_INSTRUMENTATION)
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    zephyr_sources(handlers/instr_handlers_gcc.c)
  else()
    message(WARNING "CONFIG_INSTRUMENTATION unsupported by this compiler: ${CMAKE_C_COMPILER_ID}.")
  endif()

  configure_file(${CMAKE_CURRENT_LIST_DIR}/ctf/metadata.template ${CMAKE_BINARY_DIR}/ctf_metadata @ONLY)
endif()

zephyr_compile_definitions_ifdef(CONFIG_INSTRUMENTATION INSTR_TRIGGER_FUNCTION=${CONFIG_INSTRUMENTATION_TRIGGER_FUNCTION})
zephyr_compile_definitions_ifdef(CONFIG_INSTRUMENTATION INSTR_STOPPER_FUNCTION=${CONFIG_INSTRUMENTATION_STOPPER_FUNCTION})
zephyr_include_directories_ifdef(CONFIG_INSTRUMENTATION include)
