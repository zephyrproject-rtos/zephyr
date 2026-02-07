# SPDX-License-Identifier: Apache-2.0

# Environment variables:
#   WHISPER_BIN: (Required)
#                Points to the whisper executable.
#   WHISPER_CFG: (Optional)
#                Points to the JSON file describing the platform.
#                This is passed to the whisper executable.
#                If this is not defined, ${BOARD}.whisper.json
#                under ${BOARD_DIR}/support will be used instead.
#
# GDB debugging port is 1024

if(DEFINED ENV{WHISPER_CFG})
  set(WHISPER_JSON_CFG $ENV{WHISPER_CFG})
else()
  set(WHISPER_JSON_CFG ${BOARD_DIR}/support/${board_string}.whisper.json)
endif()

if(NOT EXISTS ${WHISPER_JSON_CFG})
  message(FATAL_ERROR "Whisper config file ${WHISPER_JSON_CFG} does not exist!")
endif()

set(WHISPER_COMMON_FLAGS
  ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  --raw
  --configfile ${WHISPER_JSON_CFG}
  --harts ${CONFIG_MP_MAX_NUM_CPUS}
  )

if(CONFIG_WHISPER_USE_CLINT)
  set(WHISPER_CLINT_FLAG "--clint" "${CONFIG_WHISPER_CLINT_BASE_ADDRESS}")
else()
  set(WHISPER_CLINT_FLAG)
endif()

if(CONFIG_WHISPER_USE_SWINT)
  set(WHISPER_SWINT_FLAG "--softinterrupt" "${CONFIG_WHISPER_SWINT_BASE_ADDRESS}")
else()
  set(WHISPER_SWINT_FLAG)
endif()

if(DEFINED ENV{WHISPER_BIN})
  set(WHISPER_BIN $ENV{WHISPER_BIN})
  if(NOT EXISTS ${WHISPER_BIN})
    message(FATAL_ERROR "Cannot find whisper binary ${WHISPER_BIN}!")
  endif()
else()
  message(FATAL_ERROR "Must define WHISPER_BIN environment variable!")
endif()

message(STATUS "Found whisper: ${WHISPER_BIN}")
message(STATUS "Using whisper config: ${WHISPER_JSON_CFG}")

add_custom_target(run_whisper
  COMMAND
  ${WHISPER_BIN}
  ${WHISPER_COMMON_FLAGS}
  ${WHISPER_CLINT_FLAG}
  ${WHISPER_SWINT_FLAG}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  DEPENDS zephyr.elf
  USES_TERMINAL
  )

add_custom_target(debugserver_whisper
  COMMAND
  ${WHISPER_BIN}
  ${WHISPER_COMMON_FLAGS}
  ${WHISPER_CLINT_FLAG}
  --gdb --gdb-tcp-port 1024
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  DEPENDS zephyr.elf
  USES_TERMINAL
  )
