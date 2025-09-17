# SPDX-License-Identifier: Apache-2.0

# Note that the GDB debugging port is 1024.

# Find whisper executable via available cmake mechanisms.
# Search path can be extended by setting environment variable WHISPER_BIN_PATH.
find_program(WHISPER_BIN
  NAMES whisper
  PATHS ENV WHISPER_BIN_PATH
)

if(WHISPER_BIN STREQUAL WHISPER_BIN-NOTFOUND)
  message(WARNING "Unable to find whisper binary.")
else()

  # Points to the JSON file describing the platform.
  # This is passed to the whisper executable.
  # ${BOARD_DIR}/support/${BOARD}.whisper.json is the default.
  # This can be overridden via mechanism in zephyr_get().
  set(WHISPER_CFG ${BOARD_DIR}/support/${board_string}.whisper.json)
  zephyr_get(WHISPER_CFG)

  if(NOT EXISTS ${WHISPER_CFG})
    message(FATAL_ERROR "Whisper config file ${WHISPER_CFG} does not exist!")
  endif()

  set(WHISPER_COMMON_FLAGS
    ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
    --raw
    --configfile ${WHISPER_CFG}
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

  message(STATUS "Found whisper: ${WHISPER_BIN}")
  message(STATUS "Using whisper config: ${WHISPER_CFG}")

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

endif()
