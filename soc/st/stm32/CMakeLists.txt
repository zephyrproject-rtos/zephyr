# Copyright (c) 2024 STMicroelectronics
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_WITH_TFM)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    # Execute post build script postbuild.sh
    COMMAND $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/postbuild.sh ${CROSS_COMPILE}${CC}
  )
  # Ensure postbuild.sh script is run to update raw regression.sh installed from TF-M source tree
  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
    ${PROJECT_BINARY_DIR}/tfm/api_ns/regression.sh
  )
endif()

add_subdirectory(common)
add_subdirectory(${SOC_SERIES})
