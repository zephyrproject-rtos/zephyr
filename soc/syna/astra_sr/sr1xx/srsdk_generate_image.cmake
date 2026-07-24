# SPDX-FileCopyrightText: Copyright (c) 2026 Synaptics Incorporated
# SPDX-License-Identifier: Apache-2.0

set(imggen_command
  ${PYTHON_EXECUTABLE}
  srsdk_image_generator.py
  -B0
  -flash_image
  -sdk_secured
  -spk B0_Input_examples/spk_rc3_0_secure_otpk_0605.bin
  -apbl B0_Input_examples/sr100_b0_bootloader_ver_0x0134_ASIC_Release.axf
)

if(DEFINED M4_ELF AND NOT M4_ELF STREQUAL "" AND EXISTS ${M4_ELF})
  list(APPEND imggen_command -m4_image ${M4_ELF})
endif()

list(APPEND imggen_command
  -m55_image ${M55_ELF}
  -flash_type GD25LE128
  -flash_freq 67
)

execute_process(
  COMMAND ${imggen_command}
  WORKING_DIRECTORY ${SRSDK_IMGGEN}
  RESULT_VARIABLE imggen_result
)

if(NOT imggen_result EQUAL 0)
  message(FATAL_ERROR "srsdk_image_generator.py failed with exit code ${imggen_result}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy
    Output/B0_Flash/B0_flash_full_image_GD25LE128_67Mhz_secured.bin
    ${OUTPUT_BIN}
  WORKING_DIRECTORY ${SRSDK_IMGGEN}
  RESULT_VARIABLE copy_result
)

if(NOT copy_result EQUAL 0)
  message(FATAL_ERROR "Failed to copy SR-SDK combined image to ${OUTPUT_BIN}")
endif()
