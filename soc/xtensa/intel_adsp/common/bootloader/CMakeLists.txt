# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# TODO: Need to unbind SOF module.

add_library(base_module base_module.c)
target_include_directories(base_module PUBLIC
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/include
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/../common/include
  ${ZEPHYR_BASE}/../modules/hal/xtensa/include
  ${PROJECT_BINARY_DIR}/include/generated
  ${ZEPHYR_BASE}/../modules/audio/sof/zephyr/include
  )

add_library(boot_module boot_module.c)
target_include_directories(boot_module PUBLIC
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/include
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/../common/include
  ${ZEPHYR_BASE}/../modules/hal/xtensa/include
  ${PROJECT_BINARY_DIR}/include/generated
  ${ZEPHYR_BASE}/../modules/audio/sof/zephyr/include
  )

add_executable(bootloader
  boot_entry.S
  ${ARCH_DIR}/${ARCH}/core/startup/memctl_default.S
  ${ARCH_DIR}/${ARCH}/core/startup/memerror-vector.S
  boot_loader.c
  start_address.S
  )

target_sources_ifdef(CONFIG_RESET_VECTOR_IN_BOOTLOADER bootloader PRIVATE
  ${ARCH_DIR}/${ARCH}/core/startup/reset-vector.S
  )

add_dependencies(bootloader ${SYSCALL_LIST_H_TARGET})

set(zephyr_sdk $ENV{ZEPHYR_SDK_INSTALL_DIR})

target_include_directories(bootloader PUBLIC
  ./
  ${PROJECT_BINARY_DIR}/include
  ${TOOLCHAIN_INCLUDES}
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/include
  ${PROJECT_BINARY_DIR}/include/generated
  ${ZEPHYR_BASE}/../modules/audio/sof/zephyr/include
  )

# TODO: pre-process linker script. How do we use toplevel infrastructure ??
set(bootloader_linker_script "boot_ldr")
add_custom_command(TARGET bootloader
    PRE_LINK
    DEPENDS ${bootloader_linker_script}.x
    COMMENT "Generating Bootloader!!!!!!!"
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    ${NOSYSDEF_CFLAG}
    -MD
    -D_LINKER
    -D_ASMLANGUAGE
    -I ${SOC_DIR}/${ARCH}/${SOC_PATH}/include
    -I ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/common/include
    ${current_defines}
    ${linker_pass_define}
    -E ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/common/bootloader/${bootloader_linker_script}.x
    -P # Prevent generation of debug `#line' directives.
    -o ${bootloader_linker_script}.ld
    VERBATIM
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
 )

set_source_files_properties(boot_entry.S PROPERTIES COMPILE_FLAGS -DASSEMBLY)
if(CONFIG_RESET_VECTOR_IN_BOOTLOADER)
  set_source_files_properties(${ARCH_DIR}/${ARCH}/core/startup/reset-vector.S PROPERTIES COMPILE_FLAGS -DBOOTLOADER)
endif()

target_compile_options(bootloader PUBLIC -fno-inline-functions -mlongcalls -mtext-section-literals -imacros${CMAKE_BINARY_DIR}/zephyr/include/generated/autoconf.h)

target_link_libraries(bootloader PUBLIC -Wl,--no-check-sections -ucall_user_start -Wl,-static -nostdlib)
target_link_libraries(bootloader PRIVATE -T${CMAKE_BINARY_DIR}/zephyr/${bootloader_linker_script}.ld)

if(CONFIG_XTENSA_HAL)
  target_link_libraries(bootloader PRIVATE XTENSA_HAL)
  target_link_libraries(bootloader PRIVATE modules_xtensa_hal)
endif()
