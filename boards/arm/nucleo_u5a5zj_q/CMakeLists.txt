# SPDX-License-Identifier: Apache-2.0

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "zephyr")
	set(COMPILER_FULL_PATH ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc)
elseif(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "gnuarmemb")
	set(COMPILER_FULL_PATH ${GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc)
endif()

if(CONFIG_BUILD_WITH_TFM)
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    #Execute post build script postbuild.sh
    COMMAND $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/postbuild.sh ${COMPILER_FULL_PATH}
  )
endif()
