# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(silabs_commander "--device=SiWG917M111GTBA" "--file-type=bin"
    "--file=${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}.rps")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)

# It is not possible to load/flash a firmware using JLink, but it is possible to
# debug a firmware with:
#    west attach -r jlink
# Once started, it should be possible to reset the device with "monitor reset"
board_runner_args(jlink "--device=Si917" "--speed=10000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

# Custom signing with Silicon Labs Commander
if(CONFIG_SIGNING_ENABLED)
  # Debug: Confirm this block is entered
  message(STATUS "Entering CONFIG_SIGN_WITH_COMMANDER block")

  # Define input and output files
  set(ZEPHYR ${CMAKE_BINARY_DIR}/zephyr/zephyr.bin)
  set(ZEPHYR_BINARY ${CMAKE_BINARY_DIR}/zephyr/zephyr.bin.rps)
  set(SIGNED_RPS ${CMAKE_BINARY_DIR}/zephyr/zephyr.bin.rps)  # Align with runner

  # Define paths and keys from Kconfig
  set(WORKSPACE_ROOT $ENV{PWD})
  set(SL_COMMANDER_PATH $ENV{PWD}/SimplicityCommander-Linux/commander/commander)
  set(M4_OTA_KEY ${CONFIG_M4_OTA_KEY})
  set(M4_PRIVATE_KEY $ENV{PWD}/${CONFIG_M4_PRIVATE_KEY})

  # Check if commander exists
  if(NOT EXISTS ${SL_COMMANDER_PATH})
    message(FATAL_ERROR "SimplicityCommander not found at '${SL_COMMANDER_PATH}'. Ensure it exists and is executable.")
  else()
    message(STATUS "SimplicityCommander found at '${SL_COMMANDER_PATH}'")
  endif()

  # Check if private key exists
  if(NOT EXISTS ${M4_PRIVATE_KEY})
    message(FATAL_ERROR "Private key not found at '${M4_PRIVATE_KEY}'. Update CONFIG_M4_PRIVATE_KEY in prj.conf.")
  else()
    message(STATUS "Private key found at '${M4_PRIVATE_KEY}'")
  endif()

  # Define the commander signing command
  set(commander_cmd
    ${SL_COMMANDER_PATH}
    rps
    convert
    ${SIGNED_RPS}
    --app ${ZEPHYR_BINARY}
    --mic ${M4_OTA_KEY}
    --encrypt ${M4_OTA_KEY}
    --sign ${M4_PRIVATE_KEY}
  )

  # Debug: Print the signing command
  message(STATUS "Commander command: ${commander_cmd}")

  # Add a custom command to generate zephyr.rps, tied to zephyr.bin
  add_custom_command(
    OUTPUT ${SIGNED_RPS}
    COMMAND ${commander_cmd}
    DEPENDS ${ZEPHYR}
    COMMENT "Signing binary with Silicon Labs Commander to generate zephyr.rps"
    VERBATIM
  )

  # Add a custom target to trigger signing
  add_custom_target(commander_sign_target
    ALL  # Include in default build
    DEPENDS ${SIGNED_RPS}
    COMMENT "Ensuring signing is complete"
  )

  # Tie to zephyr_final indirectly via zephyr.bin dependency
  # Note: zephyr_final dependency is handled by the build system via zephyr.bin
else()
  # Debug: Confirm when signing is disabled
  message(STATUS "CONFIG_SIGN_WITH_COMMANDER is not enabled")
endif()