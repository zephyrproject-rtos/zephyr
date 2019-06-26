# SPDX-License-Identifier: Apache-2.0

# The intention with this file is, to have a common placeholder for macros
# which does not fit into any of the categories defined by the existing
# target_xxx.cmake files and which have a fairly high commonality between
# toolchains.
#
# See root CMakeLists.txt for description and expectations of this macro

macro(toolchain_cc_nocommon)

  zephyr_compile_options(-fno-common)

endmacro()

macro(toolchain_cc_cstd_flag dest_var_name c_std)

  set_ifndef(${dest_var_name} "-std=${c_std}")

endmacro()

#
# The two macros are dumping grounds for toolchain-specific options
# that were previously in the top-level CMakeLists.txt. This is merely
# a stopgap until the toolchain abstraction work underway (by others)
# is complete. (There are two of these to preserve the original order.)
#

macro(toolchain_cc_others_1)
  zephyr_cc_option(-fno-asynchronous-unwind-tables)
  zephyr_cc_option(-fno-pie)
  zephyr_cc_option(-fno-pic)
  zephyr_cc_option(-fno-strict-overflow)

  if(CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT)
    if(CONFIG_OMIT_FRAME_POINTER)
      zephyr_cc_option(-fomit-frame-pointer)
    else()
      zephyr_cc_option(-fno-omit-frame-pointer)
    endif()
  endif()
endmacro()

macro(toolchain_cc_others_2)
  if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    zephyr_cc_option(-fno-reorder-functions)

    if(NOT ${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "xcc")
      zephyr_cc_option(-fno-defer-pop)
    endif()
  endif()

  zephyr_cc_option_ifdef(CONFIG_STACK_USAGE -fstack-usage)

  # Strip the ${ZEPHYR_BASE} prefix from the __FILE__ macro used in __ASSERT*,
  # in the .noinit."/home/joe/zephyr/fu/bar.c" section names and in any
  # application code. This saves some memory, stops leaking user locations
  # in binaries, makes failure logs more deterministic and most importantly
  # makes builds more deterministic.

  # If both match then the last one wins. This matters for tests/ and
  # samples/ inside *both* CMAKE_SOURCE_DIR and ZEPHYR_BASE: for them
  # let's strip the shortest prefix.

  zephyr_cc_option(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=CMAKE_SOURCE_DIR)
  zephyr_cc_option(-fmacro-prefix-map=${ZEPHYR_BASE}=ZEPHYR_BASE)

  # TODO: -fmacro-prefix-map=modules/etc. "build/zephyr_modules.txt" might help.
endmacro()
