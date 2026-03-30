# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dfota)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_DELTA_UPDATE app PRIVATE src/delta_apply.c)

# flash_patch target: convert a patch binary to Intel HEX at the
# devicetree-resolved patch_partition address and flash it.
dt_nodelabel(patch_path NODELABEL "patch_partition")
if(DEFINED patch_path)
  dt_reg_addr(patch_offset PATH ${patch_path})

  math(EXPR PATCH_FLASH_ADDR
       "${CONFIG_FLASH_BASE_ADDRESS} + ${patch_offset}"
       OUTPUT_FORMAT HEXADECIMAL)

  set(PATCH_FILE ${CMAKE_CURRENT_SOURCE_DIR}/firmwares/patch.bin
      CACHE FILEPATH "Path to the delta patch binary to flash")

  find_program(WEST west REQUIRED)

  add_custom_target(flash_patch
      COMMAND ${CMAKE_OBJCOPY} -I binary -O ihex
              --change-addresses ${PATCH_FLASH_ADDR}
              ${PATCH_FILE}
              ${PROJECT_BINARY_DIR}/patch.hex
      COMMAND ${WEST} flash --hex-file ${PROJECT_BINARY_DIR}/patch.hex
      COMMENT "Flashing delta patch to patch_partition at ${PATCH_FLASH_ADDR}"
      USES_TERMINAL
  )
endif()
