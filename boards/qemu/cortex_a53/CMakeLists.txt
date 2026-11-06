# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 KylinSoft Corporation

if(CONFIG_ARM64_EFI)
  set(ZEFI_DIR ${ZEPHYR_BASE}/arch/arm64/zefi)
  set(ZEFI_SCRIPT ${ZEFI_DIR}/zefi.py)
  set(ZEFI_SOURCE ${ZEFI_DIR}/zefi.c)
  set(ZEFI_LINKER_SCRIPT ${ZEFI_DIR}/efi.ld)
  set(ZEFI_HEADERS
    ${ZEFI_DIR}/efi.h
    ${ZEFI_DIR}/printf.h
  )
  set(ZEFI_EFI ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.efi)
  set(ZEFI_STUB_ELF ${PROJECT_BINARY_DIR}/zefi.elf)
  set(ZEFI_DATA_FILE ${PROJECT_BINARY_DIR}/data.dat)
  set(ZEFI_SEGMENTS_HEADER ${PROJECT_BINARY_DIR}/zefi-segments.h)
  set(ZEFI_INPUTS
    ${ZEFI_SCRIPT}
    ${ZEFI_SOURCE}
    ${ZEFI_LINKER_SCRIPT}
    ${ZEFI_HEADERS}
  )
  set_property(GLOBAL PROPERTY zefi_arm64_link_depends ${ZEFI_INPUTS})

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEFI_SCRIPT}
    -c ${CMAKE_C_COMPILER}
    -i ${ZEPHYR_BASE}/include ${PROJECT_BINARY_DIR}/include/generated
    -o ${CMAKE_OBJCOPY}
    -f ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.elf
    --source-file ${ZEFI_SOURCE}
    --linker-script ${ZEFI_LINKER_SCRIPT}
    --output ${ZEFI_EFI}
    --stub-elf ${ZEFI_STUB_ELF}
    --data-file ${ZEFI_DATA_FILE}
    --segments-header ${ZEFI_SEGMENTS_HEADER}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )

  set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
    ${ZEFI_EFI}
    ${ZEFI_STUB_ELF}
    ${ZEFI_DATA_FILE}
    ${ZEFI_SEGMENTS_HEADER}
  )

  # Board CMakeLists runs before the ELF target exists. Defer attaching
  # LINK_DEPENDS until the end of the Zephyr CMakeLists so that edits to the
  # freestanding stub sources re-link and re-run the post-build zefi step.
  function(zefi_arm64_attach_link_depends)
    if(NOT TARGET "${logical_target_for_zephyr_elf}")
      return()
    endif()
    get_property(zefi_inputs GLOBAL PROPERTY zefi_arm64_link_depends)
    set_property(TARGET ${logical_target_for_zephyr_elf} APPEND PROPERTY
      LINK_DEPENDS
      ${zefi_inputs}
    )
  endfunction()

  cmake_language(DEFER DIRECTORY ${ZEPHYR_BASE} CALL zefi_arm64_attach_link_depends)
endif()
