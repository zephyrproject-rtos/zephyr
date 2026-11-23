# Copyright (c) 2021-2022, 2025 Arm Limited (or its affiliates). All rights reserved.
# SPDX-License-Identifier: Apache-2.0

zephyr_get(ARMFVP_BIN_PATH)

find_program(
  ARMFVP
  HINTS ${ARMFVP_BIN_PATH}
  NAMES ${ARMFVP_BIN_NAME}
  )

if(ARMFVP AND (DEFINED ARMFVP_MIN_VERSION))
  execute_process(
    COMMAND ${ARMFVP} --version
    OUTPUT_VARIABLE out
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE "\n" "" out ${out})
  string(REGEX MATCH "[0-9]+\.[0-9]+\.[0-9]+" armfvp_version ${out})
  if(${armfvp_version} VERSION_LESS ${ARMFVP_MIN_VERSION})
    set(armfvp_warning_message "Found FVP version is \"${armfvp_version}\", "
      "the minimum required by the current board is \"${ARMFVP_MIN_VERSION}\".")
    message(WARNING "${armfvp_warning_message}")
    set(ARMFVP
      COMMAND ${CMAKE_COMMAND} -E echo ${armfvp_warning_message}
      COMMAND ${ARMFVP}
    )
  endif()
endif()

# Boards that don't assemble their own firmware args in ARMFVP_FLAGS fall back
# to the generic "-a <elf>" application loading.  Boards that do assemble their
# own firmware args (e.g. via CONFIG_BUILD_WITH_TFA or CONFIG_ARMV8_A_NS) must
# populate ARMFVP_FLAGS before this file is processed.
if(NOT "-a" IN_LIST ARMFVP_FLAGS
   AND NOT CONFIG_BUILD_WITH_TFA
   AND NOT CONFIG_ARMV8_A_NS)
  set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
    -a ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
    )
endif()

if(CONFIG_ETH_SMSC91X)
  set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
    -C bp.smsc_91c111.enabled=1
    -C bp.hostbridge.userNetworking=1
    )
endif()

# Use flags passed in from the environment
set(env_fvp $ENV{ARMFVP_EXTRA_FLAGS})
separate_arguments(env_fvp)
list(APPEND ARMFVP_EXTRA_FLAGS ${env_fvp})

add_custom_target(run_armfvp
  COMMAND
  ${ARMFVP}
  ${ARMFVP_FLAGS}
  ${ARMFVP_EXTRA_FLAGS}
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT "${ARMFVP_BIN_NAME}: ${armfvp_version}"
  USES_TERMINAL
  )
