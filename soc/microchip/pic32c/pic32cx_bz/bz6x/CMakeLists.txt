# Copyright (c) 2025 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(system_config.c)

zephyr_library_sources(soc.c)

zephyr_linker_sources(SECTIONS linker.ld)

zephyr_include_directories(${ZEPHYR_BASE}/drivers)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

set(SIGNED_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/gen_signed_hex.py)
set(INPUT_HEX ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.hex)
set(INPUT_BIN ${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin)
set(OUTPUT_HEX ${CMAKE_BINARY_DIR}/zephyr/zephyr_signed.hex)
set(INPUT_ELF ${CMAKE_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME})
set(IMAGE_BIN ${CMAKE_BINARY_DIR}/zephyr/mchp-image.bin)

set(SEQ_NUM ${CONFIG_MD_SEQ_NUM})
set(AUTH_MTHD ${CONFIG_AUTH_MTHD})
set(FW_IMG_REV ${CONFIG_FW_IMG_REV})

# From zephyr/CMakeLists.txt
# Add --remove-section=.mchp_bcfg*
if(NOT CONFIG_CPP_EXCEPTIONS)
  set(eh_frame_section ".eh_frame")
else()
  set(eh_frame_section "")
endif()
set(remove_sections_argument_list "")

foreach(section .comment .mchp_bcfg* COMMON ${eh_frame_section})
  list(APPEND remove_sections_argument_list
    $<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>${section})
endforeach()


#arm-zephyr-eabi-objcopy.exe --output-target=binary
#--remove-section=.comment --remove-section=COMMON --remove-section=.eh_frame --remove-section=.mchp_bcfg* zephyr.elf ${IMAGE_BIN}
add_custom_command(
    OUTPUT ${IMAGE_BIN}
    COMMAND ${CMAKE_OBJCOPY}
      --output-target=binary
      ${remove_sections_argument_list}
      ${INPUT_ELF} ${IMAGE_BIN}
    DEPENDS ${INPUT_ELF}
    COMMENT "Converting ${KERNEL_ELF_NAME}.hex to mchp-image.bin"
)

add_custom_command(
    OUTPUT ${OUTPUT_HEX}
    COMMAND ${PYTHON_EXECUTABLE} ${SIGNED_SCRIPT}
            ${INPUT_HEX} ${IMAGE_BIN} ${OUTPUT_HEX}
            ${SEQ_NUM} ${AUTH_MTHD} ${FW_IMG_REV}
    DEPENDS ${INPUT_HEX}
    COMMENT "Converting ${CONFIG_KERNEL_BIN_NAME}.hex to zephyr_signed.hex"
)

add_custom_target(
    add_metadata ALL
    DEPENDS ${OUTPUT_HEX} ${IMAGE_BIN}
)
