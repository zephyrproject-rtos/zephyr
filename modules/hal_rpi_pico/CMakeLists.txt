# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)

function(pico_wrap_function FUNCNAME)
  target_link_options(app INTERFACE "LINKER:--wrap=${FUNCNAME}")
endfunction()

if(CONFIG_HAS_RPI_PICO)
  zephyr_library()

  set(rp2_common_dir ${ZEPHYR_HAL_RPI_PICO_MODULE_DIR}/src/rp2_common)
  set(rp2040_dir ${ZEPHYR_HAL_RPI_PICO_MODULE_DIR}/src/rp2040)
  set(common_dir ${ZEPHYR_HAL_RPI_PICO_MODULE_DIR}/src/common)
  set(boot_stage_dir ${rp2_common_dir}/boot_stage2)

  # The Second Stage Bootloader is only linked to the app that resides
  # at 0x100. Therefore, only if the app's offset is 0x100, the second
  # stage bootloader should be compiled.
  if(CONFIG_RP2_REQUIRES_SECOND_STAGE_BOOT)
    foreach(flash W25Q080 GENERIC_03H IS25LP080 W25X10CL AT25SF128A)
      if(CONFIG_RP2_FLASH_${flash})
        set(flash_type ${flash})
        break()
      endif()
    endforeach()

    set(rp2_bootloader_prefix ${CMAKE_BINARY_DIR}/bootloader)
    set(rp2_bootloader_asm ${rp2_bootloader_prefix}/boot_stage2.S)

    ExternalProject_Add(
      second_stage_bootloader
      SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/bootloader
      BINARY_DIR ${rp2_bootloader_prefix}
      CMAKE_ARGS
        -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_ASM_COMPILER=${CMAKE_ASM_COMPILER}
        -DZEPHYR_HAL_RPI_PICO_MODULE_DIR=${ZEPHYR_HAL_RPI_PICO_MODULE_DIR}
        -DZEPHYR_BASE=${ZEPHYR_BASE}
        -DFLASH_TYPE=${flash_type}
        -DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}
        -DCONFIG_LEGACY_INCLUDE_PATH=$<BOOL:${CONFIG_LEGACY_INCLUDE_PATH}>
        -DRP2_BOOTLOADER_BYPRODUCT=${rp2_bootloader_asm}
      INSTALL_COMMAND "" # No installation needed
      BUILD_BYPRODUCTS ${rp2_bootloader_asm}
      BUILD_ALWAYS TRUE
      )

      add_dependencies(${ZEPHYR_CURRENT_LIBRARY} second_stage_bootloader)
      zephyr_library_sources(${rp2_bootloader_asm})
  endif()

  # Pico sources and headers necessary for every build.
  # These contain definitions and implementation used mostly for
  # initializing the SoC, and therefore are always required.

  zephyr_library_sources(
    ${rp2_common_dir}/hardware_clocks/clocks.c
    ${rp2_common_dir}/hardware_pll/pll.c
    ${rp2_common_dir}/hardware_xosc/xosc.c
    ${rp2_common_dir}/hardware_watchdog/watchdog.c
    ${rp2_common_dir}/pico_platform/platform.c
    ${rp2_common_dir}/pico_bootrom/bootrom.c
  )

  zephyr_include_directories(
    ${rp2_common_dir}/hardware_base/include
    ${rp2_common_dir}/hardware_clocks/include
    ${rp2_common_dir}/hardware_watchdog/include
    ${rp2_common_dir}/hardware_xosc/include
    ${rp2_common_dir}/hardware_pll/include
    ${rp2_common_dir}/hardware_irq/include
    ${rp2_common_dir}/hardware_sync/include
    ${rp2_common_dir}/hardware_timer/include
    ${rp2_common_dir}/hardware_resets/include
    ${rp2_common_dir}/pico_bootrom/include
    ${rp2040_dir}/hardware_regs/include
    ${rp2040_dir}/hardware_structs/include
    ${common_dir}/pico_base/include
    ${rp2_common_dir}/pico_platform/include
    ${CMAKE_CURRENT_LIST_DIR}
  )

  zephyr_library_sources_ifdef(CONFIG_PICOSDK_USE_GPIO
          ${rp2_common_dir}/hardware_gpio/gpio.c)
  zephyr_include_directories_ifdef(CONFIG_PICOSDK_USE_GPIO
          ${rp2_common_dir}/hardware_gpio/include)

  zephyr_library_sources_ifdef(CONFIG_PICOSDK_USE_UART
          ${rp2_common_dir}/hardware_uart/uart.c)
  zephyr_include_directories_ifdef(CONFIG_PICOSDK_USE_UART
          ${rp2_common_dir}/hardware_uart/include)

  zephyr_library_sources_ifdef(CONFIG_PICOSDK_USE_FLASH
          ${rp2_common_dir}/hardware_flash/flash.c)
  zephyr_include_directories_ifdef(CONFIG_PICOSDK_USE_FLASH
          ${rp2_common_dir}/hardware_flash/include)

  zephyr_include_directories_ifdef(CONFIG_PICOSDK_USE_PWM
          ${rp2_common_dir}/hardware_pwm/include)

  zephyr_library_sources_ifdef(CONFIG_PICOSDK_USE_ADC
          ${rp2_common_dir}/hardware_adc/adc.c)
  zephyr_include_directories_ifdef(CONFIG_PICOSDK_USE_ADC
          ${rp2_common_dir}/hardware_adc/include)

  # Some flash driver functions must be executed from the RAM.
  # Originally pico-sdk places them in the RW data section, so this
  # implementation does the same.
  zephyr_linker_sources_ifdef(CONFIG_PICOSDK_USE_FLASH RWDATA timecritical.ld)

  # A function in flash.c adds 1 to a function pointer, causing a warning
  set_source_files_properties(
    ${rp2_common_dir}/hardware_flash/flash.c
    PROPERTIES
    COMPILE_FLAGS $<TARGET_PROPERTY:compiler,warning_no_pointer_arithmetic>
  )

  if(CONFIG_PICOSDK_USE_ROM_MATH)
    zephyr_library_sources(
      # Hardware divider for improved performance
      ${rp2_common_dir}/hardware_divider/divider.S

      # Float ABI
      ${rp2_common_dir}/pico_float/float_init_rom.c
      ${rp2_common_dir}/pico_float/float_aeabi.S
      ${rp2_common_dir}/pico_float/float_v1_rom_shim.S

      # Double ABI
      ${rp2_common_dir}/pico_double/double_init_rom.c
      ${rp2_common_dir}/pico_double/double_aeabi.S
      ${rp2_common_dir}/pico_double/double_v1_rom_shim.S

      # Int64 ABI
      ${rp2_common_dir}/pico_int64_ops/pico_int64_ops_aeabi.S
    )

    zephyr_include_directories(
      ${rp2_common_dir}/pico_float/include
      ${rp2_common_dir}/pico_double/include
      ${rp2_common_dir}/pico_int64_ops/include
      ${rp2_common_dir}/hardware_divider/include
    )

    # The Pico implementations for the ABI functions have a
    # __wrap_ prefix. To make the compiler call these functions,
    # we need to tell the linker that they are wrapped. For example,
    # calling the linker with "--wrap=__aeabi_fadd" will replace all
    # calls to __aeabi_fadd with calls to __wrap___aeabi_fadd. This
    # will call a wrapper that finds the implementation in ROM, and
    # call it.
    pico_wrap_function(__aeabi_fadd)
    pico_wrap_function(__aeabi_fdiv)
    pico_wrap_function(__aeabi_fmul)
    pico_wrap_function(__aeabi_frsub)
    pico_wrap_function(__aeabi_fsub)
    pico_wrap_function(__aeabi_cfcmpeq)
    pico_wrap_function(__aeabi_cfrcmple)
    pico_wrap_function(__aeabi_cfcmple)
    pico_wrap_function(__aeabi_fcmpeq)
    pico_wrap_function(__aeabi_fcmplt)
    pico_wrap_function(__aeabi_fcmple)
    pico_wrap_function(__aeabi_fcmpge)
    pico_wrap_function(__aeabi_fcmpgt)
    pico_wrap_function(__aeabi_fcmpun)
    pico_wrap_function(__aeabi_i2f)
    pico_wrap_function(__aeabi_l2f)
    pico_wrap_function(__aeabi_ui2f)
    pico_wrap_function(__aeabi_ul2f)
    pico_wrap_function(__aeabi_f2iz)
    pico_wrap_function(__aeabi_f2lz)
    pico_wrap_function(__aeabi_f2uiz)
    pico_wrap_function(__aeabi_f2ulz)
    pico_wrap_function(__aeabi_f2d)

    pico_wrap_function(__aeabi_dadd)
    pico_wrap_function(__aeabi_ddiv)
    pico_wrap_function(__aeabi_dmul)
    pico_wrap_function(__aeabi_drsub)
    pico_wrap_function(__aeabi_dsub)
    pico_wrap_function(__aeabi_cdcmpeq)
    pico_wrap_function(__aeabi_cdrcmple)
    pico_wrap_function(__aeabi_cdcmple)
    pico_wrap_function(__aeabi_dcmpeq)
    pico_wrap_function(__aeabi_dcmplt)
    pico_wrap_function(__aeabi_dcmple)
    pico_wrap_function(__aeabi_dcmpge)
    pico_wrap_function(__aeabi_dcmpgt)
    pico_wrap_function(__aeabi_dcmpun)
    pico_wrap_function(__aeabi_i2d)
    pico_wrap_function(__aeabi_l2d)
    pico_wrap_function(__aeabi_ui2d)
    pico_wrap_function(__aeabi_ul2d)
    pico_wrap_function(__aeabi_d2iz)
    pico_wrap_function(__aeabi_d2lz)
    pico_wrap_function(__aeabi_d2uiz)
    pico_wrap_function(__aeabi_d2ulz)
    pico_wrap_function(__aeabi_d2f)

    pico_wrap_function(__aeabi_lmul)
  endif()
endif()
