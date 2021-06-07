# SPDX-License-Identifier: Apache-2.0

# Enable debug support in mdb
# Dwarf version 2 can be recognized by mdb
# The default dwarf version in gdb is not recognized by mdb
zephyr_cc_option(-g3 -gdwarf-2)

# Without this (poorly named) option, compiler may generate undefined
# references to abort().
# See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63691
zephyr_cc_option(-fno-delete-null-pointer-checks)

zephyr_cc_option_ifdef(CONFIG_ARC_USE_UNALIGNED_MEM_ACCESS -munaligned-access)

if(CONFIG_ISA_ARCV2)
  # Instruct compiler to use register R26 as thread pointer
  # for thread local storage.
  # For ARCv3 the register is fixed to r30, so we don't need to specify it
  zephyr_cc_option_ifdef(CONFIG_THREAD_LOCAL_STORAGE -mtp-regno=26)
endif()

add_subdirectory(core)



if (CONFIG_BUILD_WITH_SECURESHIELD)
  set(SECURESHIELD_BINARY_DIR ${CMAKE_BINARY_DIR}/secureshield)
  add_custom_target(secureshield_cmake
    COMMAND ${CMAKE_COMMAND} -DBOARD=${CONFIG_SECURESHIELD_BOARD} -B${SECURESHIELD_BINARY_DIR} -S${ZEPHYR_BASE}/${CONFIG_SECURESHIELD_IMAGE_PATH} -G${CMAKE_GENERATOR}
    COMMAND_EXPAND_LISTS
  )
  include(ExternalProject)

  ExternalProject_Add(
    secureshield
    SOURCE_DIR ${ZEPHYR_BASE}/${CONFIG_SECURESHIELD_IMAGE_PATH}
    BINARY_DIR ${SECURESHIELD_BINARY_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_COMMAND} --build ${SECURESHIELD_BINARY_DIR}
    INSTALL_COMMAND ""
    BUILD_ALWAYS True
    USES_TERMINAL_BUILD True
    DEPENDS secureshield_cmake
  )
endif()
