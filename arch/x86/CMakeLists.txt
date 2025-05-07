# Copyright (c) 2019 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

# Convert the .bin file argument to a .o file, create a wrapper
# library for the .o file, and register the library as a generated
# file that is to be linked in after the first link.
function(add_bin_file_to_the_next_link target_dependency bin)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${bin}.o
    COMMAND
    ${CMAKE_OBJCOPY}
    -I binary
    -B ${OUTPUT_ARCH}
    -O ${OUTPUT_FORMAT}
    --rename-section .data=${bin},CONTENTS,ALLOC,LOAD,READONLY,DATA
    ${bin}.bin
    ${bin}.o
    DEPENDS ${target_dependency} ${bin}.bin
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(${bin}_o DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${bin}.o)
  add_library(${bin} STATIC IMPORTED GLOBAL)
  set_property(TARGET ${bin} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${bin}.o)
  add_dependencies(${bin} ${bin}_o)
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_OBJECT_FILES ${bin})
endfunction()

if(CONFIG_X86_64)
  include(intel64.cmake)
else()
  include(ia32.cmake)
endif()

# Always set for 64-bit (long mode requires page tables), optional for 32-bit
if (CONFIG_MMU)
  set(GEN_MMU ${ZEPHYR_BASE}/arch/x86/gen_mmu.py)

  if(DEFINED X86_EXTRA_GEN_MMU_ARGUMENTS)
    # Make the string into a list, or else it will be passed to ${GEN_MMU}
    # as a quoted string, which is then parsed as one item by Python's
    # argparse.
    string(REPLACE " " ";"
           X86_EXTRA_GEN_MMU_ARGUMENTS
           "${X86_EXTRA_GEN_MMU_ARGUMENTS}")
  else()
    set(X86_EXTRA_GEN_MMU_ARGUMENTS "")
  endif()

  add_custom_target(
    pagetables_bin_target
    DEPENDS
    pagetables.bin
  )
  add_custom_command(
    OUTPUT pagetables.bin
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${GEN_MMU}
    --kernel $<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
    --output pagetables.bin
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    ${X86_EXTRA_GEN_MMU_ARGUMENTS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${ZEPHYR_PREBUILT_EXECUTABLE} ${GEN_MMU}
  )

  add_bin_file_to_the_next_link(pagetables_bin_target pagetables)
endif()

if(CONFIG_ARCH_HAS_TIMING_FUNCTIONS AND
    NOT CONFIG_SOC_HAS_TIMING_FUNCTIONS AND
    NOT CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
zephyr_library_sources_ifdef(CONFIG_TIMING_FUNCTIONS timing.c)
endif()

if(CONFIG_X86_CET)
  zephyr_compile_options(-fcf-protection=full)

  if(CONFIG_HW_SHADOW_STACK)
    set(GEN_SHSTK_ARRAY ${ZEPHYR_BASE}/arch/x86/gen_static_shstk_array.py)

    if(CONFIG_X86_64)
      set(gen_shstk_array_arch_param --arch x86_64)

      execute_process(
        COMMAND
        ${PYTHON_EXECUTABLE}
        ${GEN_SHSTK_ARRAY}
        --header-output ${PROJECT_BINARY_DIR}/include/generated/zephyr/x86_shstk.h
        --config ${DOTCONFIG}
        ${gen_shstk_array_arch_param}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
    else()
      set(gen_shstk_array_arch_param --arch x86)
    endif()

    set_property(GLOBAL APPEND PROPERTY post_build_patch_elf_commands
      COMMAND
      ${PYTHON_EXECUTABLE}
      ${GEN_SHSTK_ARRAY}
      --kernel $<TARGET_FILE:${ZEPHYR_FINAL_EXECUTABLE}>
      ${gen_shstk_array_arch_param}
    )
  endif()
endif()
