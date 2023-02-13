# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_base)

  if(NOT PROPERTY_LINKER_SCRIPT_DEFINES)
    set_property(GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__GCC_LINKER_CMD__)
  endif()

  # TOOLCHAIN_LD_FLAGS comes from compiler/clang/target.cmake
  # LINKERFLAGPREFIX comes from linker/lld/target.cmake
  zephyr_ld_options(
    -no-pie
    ${TOOLCHAIN_LD_FLAGS}
  )

  zephyr_ld_options(
    ${LINKERFLAGPREFIX},--gc-sections
    ${LINKERFLAGPREFIX},--build-id=none
  )

  # Sort each input section by alignment.
  zephyr_ld_option_ifdef(
    CONFIG_LINKER_SORT_BY_ALIGNMENT
    ${LINKERFLAGPREFIX},--sort-section=alignment
  )

  if (NOT CONFIG_LINKER_USE_RELAX)
    zephyr_ld_options(
      ${LINKERFLAGPREFIX},--no-relax
    )
  endif()

endmacro()
