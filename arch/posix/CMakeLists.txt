# SPDX-License-Identifier: Apache-2.0

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${CMAKE_HOST_SYSTEM_NAME}.${CMAKE_HOST_SYSTEM_PROCESSOR}.cmake)
  # @Intent: Set necessary compiler & linker options for this specific host architecture & OS
  include(${CMAKE_HOST_SYSTEM_NAME}.${CMAKE_HOST_SYSTEM_PROCESSOR}.cmake)
else()
  if (CONFIG_64BIT)
    # some gcc versions fail to build without -fPIC
    zephyr_compile_options(-m64 -fPIC)
    zephyr_link_libraries(-m64)
  else ()
    zephyr_compile_options(-m32)
    zephyr_link_libraries(-m32)
  endif ()
endif()

zephyr_compile_options(
  ${ARCH_FLAG}
  -include ${ZEPHYR_BASE}/arch/posix/include/posix_cheats.h
  )

# @Intent: Obtain compiler specific flags for no freestanding compilation
zephyr_compile_options($<TARGET_PROPERTY:compiler,hosted>)

zephyr_include_directories(${BOARD_DIR})

if (CONFIG_GPROF)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,gprof>)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,gprof>)
endif()

zephyr_compile_definitions(_POSIX_C_SOURCE=200809 _XOPEN_SOURCE=600 _XOPEN_SOURCE_EXTENDED)

zephyr_ld_options(
  -ldl
  -pthread
)

# About the -include directive: The reason to do it this way, is because in this
# manner it is transparent to the application. Otherwise posix_cheats.h needs to
# be included in all the applications' files which define main( ), and in any
# app file which uses the pthreads like API provided by Zephyr
# ( include/posix/pthread.h / kernel/pthread.c ) [And any future API added to
# Zephyr which will clash with the native POSIX API] . It would also need to
# be included in a few zephyr kernel files.

#
# Support for the LLVM Sanitizer toolchain instrumentation frameworks
# (supported by current gcc's as well)
#

if(CONFIG_ASAN)
  list(APPEND LLVM_SANITIZERS "address")
endif()

if(CONFIG_MSAN)
  list(APPEND LLVM_SANITIZERS "memory")
endif()

if(CONFIG_UBSAN)
  list(APPEND LLVM_SANITIZERS "undefined")
endif()

if(CONFIG_ASAN_RECOVER)
  zephyr_compile_options(-fsanitize-recover=all)
endif()

if(CONFIG_ARCH_POSIX_LIBFUZZER)
  list(APPEND LLVM_SANITIZERS "fuzzer")
  if(NOT CONFIG_64BIT)
    # On i386, libfuzzer seems to dynamically relocate the binary, so
    # we need to emit PIC code.  This limitation is undocumented and
    # poorly understood...
    zephyr_compile_options(-fPIC)
  endif()
endif()

list(JOIN LLVM_SANITIZERS "," LLVM_SANITIZERS_ARG)
if(NOT ${LLVM_SANITIZERS_ARG} STREQUAL "")
  set(LLVM_SANITIZERS_ARG "-fsanitize=${LLVM_SANITIZERS_ARG}")
  zephyr_compile_options("${LLVM_SANITIZERS_ARG}")
  zephyr_link_libraries("${LLVM_SANITIZERS_ARG}")
endif()


add_subdirectory(core)
