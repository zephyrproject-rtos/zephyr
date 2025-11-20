# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
)

zephyr_sources_ifdef(CONFIG_STM32N6_NPU
  npu/npu_stm32n6.c
)

zephyr_include_directories(.)

if(CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS)
  zephyr_linker_sources(SECTIONS mpu_regions.ld)
  zephyr_sources(mpu_regions.c)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

zephyr_linker_sources_ifdef(CONFIG_BOOTLOADER_MCUBOOT SECTIONS ram_check.ld)

if(NOT CONFIG_BOOTLOADER_MCUBOOT)

  set(signing_tool STM32_SigningTool_CLI)

  # find_program will automatically search for .exe extension on Windows
  find_program(signing_tool_find ${signing_tool})
  if(signing_tool_find STREQUAL signing_tool_find-NOTFOUND)
    message(WARNING "
      Signing Image tool (${signing_tool}) is not available.
      Signed image will not be generated.
      You won't be able to run application on the board.
      Refer to board documentation for more information.
    ")
  else()
    message(STATUS "Found STM32 signing tool: ${signing_tool_find}")

    execute_process(
      COMMAND ${signing_tool_find} --version
      OUTPUT_VARIABLE version
      RESULT_VARIABLE result
    )

    set(signing_tool_args
      -in ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
      -nk -t fsbl -hv 2.3 --silent
      -o ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.signed.bin
      -dump ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.signed.bin
    )

    set(signing_tool_version_regex "v([0-9]+[.][0-9]+[.][0-9]+)")
    if(result EQUAL 0 AND version MATCHES ${signing_tool_version_regex})
      set(signing_tool_version ${CMAKE_MATCH_1})
      set(signing_tool_version_min 2.21.0)

      if(signing_tool_version VERSION_GREATER_EQUAL signing_tool_version_min)
        list(APPEND signing_tool_args -align)

        message(STATUS
          "${signing_tool} version is v${signing_tool_version}, "
          "appending \"-align\" flag to post-build signing command "
          "(applicable only with header v2.3 for MCUs, starting from "
          "version v${signing_tool_version_min})"
        )
      endif()
    else()
      message(FATAL_ERROR "
        Unable to determine ${signing_tool} version (expected match to
        ${signing_tool_version_regex} regex)
      ")
    endif()

    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${signing_tool} ${signing_tool_args}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )

    set_property(TARGET runners_yaml_props_target PROPERTY bin_file ${CONFIG_KERNEL_BIN_NAME}.signed.bin)
  endif()

endif()
