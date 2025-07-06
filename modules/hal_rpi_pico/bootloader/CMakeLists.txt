# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Skip compiler checking
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(second_stage_bootloader)
enable_language(ASM)

set(rp2_common_dir ${ZEPHYR_HAL_RPI_PICO_MODULE_DIR}/src/rp2_common)
set(rp2040_dir ${ZEPHYR_HAL_RPI_PICO_MODULE_DIR}/src/rp2040)
set(common_dir ${ZEPHYR_HAL_RPI_PICO_MODULE_DIR}/src/common)
set(boot_stage_dir ${rp2040_dir}/boot_stage2)

add_executable(boot_stage2)

if(${FLASH_TYPE} STREQUAL W25Q080)
  set(flash_type_file boot2_w25q080.S)
elseif(${FLASH_TYPE} STREQUAL GENERIC_03H)
  set(flash_type_file boot2_generic_03h.S)
elseif(${FLASH_TYPE} STREQUAL IS25LP080)
  set(flash_type_file boot2_is25lp080.S)
elseif(${FLASH_TYPE} STREQUAL W25X10CL)
  set(flash_type_file boot2_w25x10cl.S)
elseif(${FLASH_TYPE} STREQUAL AT25SF128A)
  set(flash_type_file boot2_at25sf128a.S)
else()
  message(FATAL_ERROR "No flash type selected")
endif()

target_sources(boot_stage2 PRIVATE ${boot_stage_dir}/${flash_type_file})

target_include_directories(boot_stage2 PUBLIC
  ..
  ${boot_stage_dir}/asminclude
  ${rp2040_dir}/pico_platform/include
  ${rp2040_dir}/hardware_regs/include
  ${common_dir}/pico_base_headers/include
  ${rp2_common_dir}/pico_platform_compiler/include
  ${rp2_common_dir}/pico_platform_sections/include
  ${rp2_common_dir}/pico_platform_panic/include
  ${ZEPHYR_BASE}/include
  )

target_link_options(boot_stage2 PRIVATE
  "-nostartfiles"
  "--specs=nosys.specs"
  "LINKER:--script=${boot_stage_dir}/boot_stage2.ld"
  )

# The second stage bootloader is compiled without kconfig definitions.
# Therefore, in order to use toolchain.h, it needs to define CONFIG_ARM.
target_compile_definitions(boot_stage2 PRIVATE -DCONFIG_ARM=1)

# Generates a binary file from the compiled bootloader
add_custom_command(TARGET boot_stage2
  POST_BUILD
  BYPRODUCTS boot_stage2.bin
  COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:boot_stage2> boot_stage2.bin
  )

# Checksums the binary, pads it, and generates an assembly file
add_custom_command(TARGET boot_stage2
  POST_BUILD
  BYPRODUCTS ${RP2_BOOTLOADER_BYPRODUCT}
  COMMAND ${PYTHON_EXECUTABLE} ${boot_stage_dir}/pad_checksum
  -s 0xffffffff boot_stage2.bin ${RP2_BOOTLOADER_BYPRODUCT}
  )
