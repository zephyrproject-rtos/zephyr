# SPDX-License-Identifier: Apache-2.0

if (NOT CMAKE_HOST_UNIX OR CMAKE_HOST_APPLE)
  message(FATAL_ERROR "The POSIX architecture only works on Linux. If on Windows or macOS "
          "consider using a virtual machine to run a Linux guest.")
endif()

# This native_simulator library is used to pass options to the
# native_simulator runner build. Currently the following are used:
#  INTERFACE_COMPILE_OPTIONS:
#      Extra compile options to be used during the build of the runner files
#      For ex. target_compile_options(native_simulator INTERFACE "-m64")
#  INTERFACE_LINK_OPTIONS:
#      Extra link options to be passed during the *final* link of the runner
#      with the embedded SW.
#      For ex. target_link_options(native_simulator INTERFACE "-lstdc++")
#  INTERFACE_SOURCES:
#      Extra sources to be built in the native simulator runner context
#      For ex. target_sources(native_simulator INTERFACE silly.c)
#      Note that these are built with the host libC and the include directories
#      the runner is built with.
#  RUNNER_LINK_LIBRARIES:
#      Extra libraries to link with the runner
#      For ex. set_property(TARGET native_simulator APPEND PROPERTY RUNNER_LINK_LIBRARIES "mylib.a")
#  Note: target_link_libraries() cannot be used on this library at this point.
#        target_link_libraries() updates INTERFACE_LINK_LIBRARIES but wrapping it with extra
#        information. This means we cannot directly pass it to the native_simulator runner build.
#        Check https://cmake.org/cmake/help/latest/prop_tgt/INTERFACE_LINK_LIBRARIES.html for more
#        info.
#        We use target_link_options() instead
add_library(native_simulator INTERFACE)
set_property(TARGET native_simulator PROPERTY RUNNER_LINK_LIBRARIES "")

set(NSI_DIR ${ZEPHYR_BASE}/scripts/native_simulator CACHE PATH "Path to the native simulator")

if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/${CMAKE_HOST_SYSTEM_NAME}.${CMAKE_HOST_SYSTEM_PROCESSOR}.cmake)
  # @Intent: Set necessary compiler & linker options for this specific host architecture & OS
  include(${CMAKE_HOST_SYSTEM_NAME}.${CMAKE_HOST_SYSTEM_PROCESSOR}.cmake)
else() # Linux.x86_64
  if (CONFIG_64BIT)
    # some gcc versions fail to build without -fPIC
    zephyr_compile_options(-m64 -fPIC)
    zephyr_link_libraries(-m64)

    target_link_options(native_simulator INTERFACE "-m64")
    target_compile_options(native_simulator INTERFACE "-m64")
  else ()
    zephyr_compile_options(-m32)
    zephyr_link_libraries(-m32)

    target_link_options(native_simulator INTERFACE "-m32")
    target_compile_options(native_simulator INTERFACE "-m32")

    # When building for 32bits x86, gcc defaults to using the old 8087 float arithmetic
    # which causes some issues with literal float comparisons. So we set it
    # to use the SSE2 float path instead
    # (clang defaults to use SSE, but, setting this option for it is safe)
    check_set_compiler_property(APPEND PROPERTY fpsse2 "SHELL:-msse2 -mfpmath=sse")
    zephyr_compile_options($<TARGET_PROPERTY:compiler,fpsse2>)
    target_compile_options(native_simulator INTERFACE "$<TARGET_PROPERTY:compiler,fpsse2>")
  endif ()
endif()

zephyr_compile_options(
  ${ARCH_FLAG}
  )

if (CONFIG_NATIVE_APPLICATION)
  zephyr_compile_options(
    -include ${ZEPHYR_BASE}/arch/posix/include/posix_cheats.h
  )
elseif (CONFIG_NATIVE_LIBRARY)
  zephyr_compile_options(
    -fvisibility=hidden
  )

  # While doing the partial linking of the native library, some symbols will be missing
  # which are provided by the native simulator runner
  zephyr_ld_options(
    -Wl,--unresolved-symbols=ignore-all
  )

  if (NOT CONFIG_EXTERNAL_LIBC)
    # Get the *compiler* include path, that is where the *compiler* provided headers are (not the
    # default libC ones). This includes basic headers like stdint.h, stddef.h or float.h
    # We expect something like
    #  /usr/lib/gcc/x86_64-linux-gnu/12/include or /usr/lib/llvm-14/lib/clang/14.0.0/include
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} --print-file-name=include/stddef.h
      OUTPUT_VARIABLE _OUTPUT
      COMMAND_ERROR_IS_FATAL ANY
    )
    get_filename_component(COMPILER_OWN_INCLUDE_PATH "${_OUTPUT}" DIRECTORY)

    # Do not use the C library from this compiler/host,
    # but still use the basic compiler headers
    # -fno-builtin to avoid the compiler using builtin replacements for std library functions
    zephyr_compile_options(
      -nostdinc
      -isystem ${COMPILER_OWN_INCLUDE_PATH}
      $<TARGET_PROPERTY:compiler,freestanding>
      -fno-builtin
    )
  endif()
endif()

if(CONFIG_EXTERNAL_LIBC)
  # @Intent: Obtain compiler specific flags for no freestanding compilation
  zephyr_compile_options($<TARGET_PROPERTY:compiler,hosted>)
endif()

if(CONFIG_EXTERNAL_LIBCPP)
  target_link_options(native_simulator INTERFACE "-lstdc++")
endif()

zephyr_include_directories(${BOARD_DIR})

if(CONFIG_COVERAGE)
  target_compile_options(native_simulator INTERFACE $<TARGET_PROPERTY:compiler,coverage>)
  target_link_options(native_simulator INTERFACE $<TARGET_PROPERTY:linker,coverage>)
endif()

if (CONFIG_GPROF)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,gprof>)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,gprof>)

  target_link_options(native_simulator INTERFACE "-pg")
endif()

zephyr_compile_definitions(_POSIX_C_SOURCE=200809 _XOPEN_SOURCE=600 _XOPEN_SOURCE_EXTENDED)

if (CONFIG_NATIVE_APPLICATION)
  zephyr_ld_options(
    -ldl
    -pthread
  )
endif()

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

  target_link_options(native_simulator INTERFACE ${LLVM_SANITIZERS_ARG})
  target_compile_options(native_simulator INTERFACE ${LLVM_SANITIZERS_ARG})
endif()

# Override the C standard used for compilation to C 2011
# This is due to some tests using _Static_assert which is a 2011 feature, but
# otherwise relying on compilers supporting it also when set to C99.
# This was in general ok, but with some host compilers and C library versions
# it led to problems. So we override it to 2011 for native_posix.
set_property(GLOBAL PROPERTY CSTD c11)

add_subdirectory(core)
