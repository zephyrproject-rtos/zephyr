# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_base)

  if(NOT PROPERTY_LINKER_SCRIPT_DEFINES)
    set_property(GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__GCC_LINKER_CMD__)
  endif()

  # TOOLCHAIN_LD_FLAGS comes from compiler/gcc/target.cmake
  # LINKERFLAGPREFIX comes from linker/ld/target.cmake
  zephyr_ld_options(
    ${TOOLCHAIN_LD_FLAGS}
  )

  zephyr_ld_options(
    ${LINKERFLAGPREFIX},--gc-sections
    ${LINKERFLAGPREFIX},--build-id=none
  )

  # Sort the common symbols and each input section by alignment
  # in descending order to minimize padding between these symbols.
  zephyr_ld_option_ifdef(
    CONFIG_LINKER_SORT_BY_ALIGNMENT
    ${LINKERFLAGPREFIX},--sort-common=descending
    ${LINKERFLAGPREFIX},--sort-section=alignment
  )

  if (NOT CONFIG_LINKER_USE_RELAX)
    zephyr_ld_options(
      ${LINKERFLAGPREFIX},--no-relax
    )
  endif()

  if (CONFIG_LLVM_USE_LD)
    if(CONFIG_LIBGCC_RTLIB)
      set(runtime_lib "libgcc")
    elseif(CONFIG_COMPILER_RT_RTLIB)
      set(runtime_lib "compiler_rt")
    endif()

    zephyr_link_libraries(
      --config ${ZEPHYR_BASE}/cmake/toolchain/llvm/clang_${runtime_lib}.cfg
    )
  endif()

  if(CONFIG_CPP AND (CMAKE_C_COMPILER_ID STREQUAL "Clang"))
    # GNU ld complains when used with llvm/clang:
    #   error: section: init_array is not contiguous with other relro sections
    #
    # So do not create RELRO program header.
    zephyr_link_libraries(
      -Wl,-z,norelro
    )
  endif()

endmacro()
