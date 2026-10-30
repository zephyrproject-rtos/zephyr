# SPDX-FileCopyrightText: Copyright (c) 2026 Synaptics Incorporated
# SPDX-License-Identifier: Apache-2.0

find_path(SRSDK_IMGGEN
  srsdk_image_generator.py
  OPTIONAL
)

if(SRSDK_IMGGEN AND NOT DEFINED TC_RUNID)
  message("-- Found SRSDK Image generator directory: ${SRSDK_IMGGEN}")
  message("executing srsdk_image_generator.py
      -B0
      -flash_image
      -sdk_secured
      -spk B0_Input_examples/spk_rc4_1_0_secure_otpk.bin
      ${SR100_IMAGEGEN_M4_PARAM}
      -apbl ${SRSDK_IMGGEN}/B0_Input_examples/sr100_b0_bootloader_ver_0x0134_ASIC_Release.axf
      -m55_image ${SR100_IMAGEGEN_M55_IMG}
      -model ''
      -flash_type GD25LE128
      -flash_freq 67")

  add_custom_target(srsdk_generate_image ALL
    COMMAND ${PYTHON_EXECUTABLE} srsdk_image_generator.py
      -B0
      -flash_image
      -sdk_secured
      -spk B0_Input_examples/spk_rc4_1_0_secure_otpk.bin
      ${SR100_IMAGEGEN_M4_PARAM}
      -apbl ${SRSDK_IMGGEN}/B0_Input_examples/sr100_b0_bootloader_ver_0x0134_ASIC_Release.axf
      -m55_image ${SR100_IMAGEGEN_M55_IMG}
      -model ''
      -flash_type GD25LE128
      -flash_freq 67
    COMMAND ${CMAKE_COMMAND} -E copy
      Output/B0_Flash/B0_flash_full_image_GD25LE128_67Mhz_secured.bin
      ${CMAKE_BINARY_DIR}/zephyr/zephyr_flash.bin
    WORKING_DIRECTORY ${SRSDK_IMGGEN}
    DEPENDS ${SR100_IMAGEGEN_M55_IMG}
  )

  if(TARGET runners_yaml_props_target)
    set_property(TARGET runners_yaml_props_target PROPERTY bin_file ${CMAKE_BINARY_DIR}/zephyr/zephyr_flash.bin)
  endif()
elseif(SRSDK_IMGGEN AND DEFINED TC_RUNID)
  message("-- SR100 imagegen: skipping post-link image generation for Twister build")
endif()
