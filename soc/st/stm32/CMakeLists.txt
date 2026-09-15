# Copyright (c) 2024 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFM)
  # Run the postbuild.sh script in a custom target so that the regression.sh script
  # installed by the TF-M build always gets updated with the proper values.
  add_custom_target(stm32_tfm_postbuild ALL
    COMMAND $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/postbuild.sh ${CROSS_COMPILE}${CC}
    COMMENT "Patching TF-M helper scripts with flash layout addresses"
    VERBATIM
  )
  add_dependencies(stm32_tfm_postbuild tfm)
endif()

add_subdirectory(common)
add_subdirectory(${CONFIG_SOC_SERIES})
