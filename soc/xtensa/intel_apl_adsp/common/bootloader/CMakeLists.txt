# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

add_library(base_module base_module.c)
target_include_directories(base_module PUBLIC
  $<TARGET_PROPERTY:${ARCH}_${SOC_NAME},SOURCE_DIR>/include
  )

add_library(boot_module boot_module.c)
target_include_directories(boot_module PUBLIC
  $<TARGET_PROPERTY:${ARCH}_${SOC_NAME},SOURCE_DIR>/include
  )

add_executable(bootloader
  boot_entry.S
  ${ARCH_DIR}/${ARCH}/core/startup/memctl_default.S
  ${ARCH_DIR}/${ARCH}/core/startup/memerror-vector.S
  ${ARCH_DIR}/${ARCH}/core/startup/reset-vector.S
  boot_loader.c
  start_address.S
  )

add_dependencies(bootloader ${SYSCALL_LIST_H_TARGET})

set(zephyr_sdk ${ZEPHYR_SDK_INSTALL_DIR})

target_include_directories(bootloader PUBLIC
  ./
  ${ZEPHYR_BASE}/include
  ${TOOLCHAIN_INCLUDES}
  $<TARGET_PROPERTY:${ARCH}_${SOC_NAME},SOURCE_DIR>/
  $<TARGET_PROPERTY:${ARCH}_${SOC_NAME},SOURCE_DIR>/include
  )

set_source_files_properties(boot_entry.S PROPERTIES COMPILE_FLAGS -DASSEMBLY)
set_source_files_properties(${ARCH_DIR}/${ARCH}/core/startup/reset-vector.S PROPERTIES COMPILE_FLAGS -DBOOTLOADER)

target_compile_options(bootloader PUBLIC -fno-inline-functions -mlongcalls -mtext-section-literals -imacros${CMAKE_BINARY_DIR}/zephyr/include/generated/autoconf.h)

target_link_libraries(bootloader PUBLIC -Wl,--no-check-sections -ucall_user_start -Wl,-static -nostdlib)
target_link_libraries(bootloader PRIVATE -lhal -L${zephyr_sdk}/xtensa/intel_apl_adsp/xtensa-zephyr-elf/lib)
target_link_libraries(bootloader PRIVATE -T$<TARGET_PROPERTY:${ARCH}_${SOC_NAME},SOURCE_DIR>/common/bootloader/boot_ldr.x)

if(CONFIG_XTENSA_HAL)
  target_link_libraries(bootloader PRIVATE XTENSA_HAL)
endif()
