# SPDX-License-Identifier: Apache-2.0

include(ExternalProject)

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
    ExternalProject_Add(
      second_stage_bootloader
      SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/bootloader
      BINARY_DIR ${rp2_bootloader_prefix}
      CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_ASM_COMPILER=${CMAKE_ASM_COMPILER}
        -DZEPHYR_HAL_RPI_PICO_MODULE_DIR=${ZEPHYR_HAL_RPI_PICO_MODULE_DIR}
        -DZEPHYR_BASE=${ZEPHYR_BASE}
        -DFLASH_TYPE=${flash_type}
        -DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}
      INSTALL_COMMAND "" # No installation needed
      BUILD_BYPRODUCTS ${rp2_bootloader_prefix}/boot_stage2.S
      BUILD_ALWAYS TRUE
      )
    zephyr_library_sources(${rp2_bootloader_prefix}/boot_stage2.S)
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
endif()
