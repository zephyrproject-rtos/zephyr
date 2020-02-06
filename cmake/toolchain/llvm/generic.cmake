# SPDX-License-Identifier: Apache-2.0

find_appropriate_cache_directory(USER_CACHE_DIR)

if((NOT "${USER_CACHE_DIR}" STREQUAL "") AND (EXISTS "${USER_CACHE_DIR}"))
  message(STATUS "Invalidating toolchain capability cache in ${USER_CACHE_DIR}")
  execute_process(COMMAND
    ${CMAKE_COMMAND} -E remove_directory "${USER_CACHE_DIR}")
endif()

if(DEFINED $ENV{CLANG_ROOT_DIR})
  set(TOOLCHAIN_HOME ${CLANG_ROOT}/bin/)
endif()

set(COMPILER clang)
set(LINKER ld) # TODO: Use lld eventually rather than GNU ld
set(BINTOOLS llvm)

if("${ARCH}" STREQUAL "arm")
  set(triple arm-zephyr-eabi)
  set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")
  set_ifndef(GNUARMEMB_TOOLCHAIN_PATH "$ENV{GNUARMEMB_TOOLCHAIN_PATH}")
  find_program(ARMGCC ${triple}-gcc PATHS ${ZEPHYR_SDK_INSTALL_DIR}/${triple}/bin NO_DEFAULT_PATH)
  assert_exists(ARMGCC)
  set(CMAKE_C_LINK_EXECUTABLE "${ARMGCC} <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> -lc")
  set(CMAKE_CXX_LINK_EXECUTABLE "${ARMGCC} <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> -lc")
elseif("${ARCH}" STREQUAL "x86")
  set(triple i686-pc-none-elf)
endif()

set(CMAKE_C_COMPILER_TARGET   ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

if("${ARCH}" STREQUAL "posix")
  set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")
endif()
