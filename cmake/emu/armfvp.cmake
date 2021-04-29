# Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
# SPDX-License-Identifier: Apache-2.0

set(armfvp_bin_path $ENV{ARMFVP_BIN_PATH})

find_program(
  ARMFVP
  PATHS ${armfvp_bin_path}
  NO_DEFAULT_PATH
  NAMES ${ARMFVP_BIN_NAME}
  )

add_custom_target(run
  COMMAND
  ${ARMFVP}
  ${ARMFVP_FLAGS}
  -a ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  DEPENDS ${ARMFVP} ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT "FVP: ${ARMFVP}"
  USES_TERMINAL
  )
