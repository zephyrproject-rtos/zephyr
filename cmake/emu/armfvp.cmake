# Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
# SPDX-License-Identifier: Apache-2.0

set(armfvp_bin_path $ENV{ARMFVP_BIN_PATH})

find_program(
  ARMFVP
  PATHS ${armfvp_bin_path}
  NO_DEFAULT_PATH
  NAMES ${ARMFVP_BIN_NAME}
  )

if(CONFIG_ARMV8_A_NS)
  foreach(filetype BL1 FIP)
    if ((NOT DEFINED ARMFVP_${filetype}_FILE) AND (EXISTS "$ENV{ARMFVP_${filetype}_FILE}"))
      set(ARMFVP_${filetype}_FILE "$ENV{ARMFVP_${filetype}_FILE}" CACHE FILEPATH
        "ARM FVP ${filetype} File specified in environment"
	)
    endif()

    if(NOT EXISTS "${ARMFVP_${filetype}_FILE}")
      string(TOLOWER ${filetype} filename)
      message(FATAL_ERROR "Please specify ARMFVP_${filetype}_FILE in environment "
        "or with -DARMFVP_${filetype}_FILE=</path/to/${filename}.bin>")
    endif()
  endforeach()

  set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
    -C bp.secureflashloader.fname=${ARMFVP_BL1_FILE}
    -C bp.flashloader0.fname=${ARMFVP_FIP_FILE}
    --data cluster0.cpu0="${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_BIN_NAME}"@0x88000000
    )
else()
  set(ARMFVP_FLAGS ${ARMFVP_FLAGS}
    -a ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
    )
endif()

add_custom_target(run_armfvp
  COMMAND
  ${ARMFVP}
  ${ARMFVP_FLAGS}
  DEPENDS ${ARMFVP} ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  COMMENT "FVP: ${ARMFVP}"
  USES_TERMINAL
  )
