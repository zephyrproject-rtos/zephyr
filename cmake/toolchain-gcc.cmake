# Configures CMake for using GCC, this script is re-used by several
# GCC-based toolchains

set(CMAKE_C_COMPILER   ${CROSS_COMPILE}gcc     CACHE INTERNAL " " FORCE)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++     CACHE INTERNAL " " FORCE)
set(CMAKE_OBJCOPY      ${CROSS_COMPILE}objcopy CACHE INTERNAL " " FORCE)
set(CMAKE_OBJDUMP      ${CROSS_COMPILE}objdump CACHE INTERNAL " " FORCE)
#set(CMAKE_LINKER      ${CROSS_COMPILE}ld      CACHE INTERNAL " " FORCE) # Not in use yet
set(CMAKE_AR           ${CROSS_COMPILE}ar      CACHE INTERNAL " " FORCE)
set(CMAKE_RANLILB      ${CROSS_COMPILE}ranlib  CACHE INTERNAL " " FORCE)

include($ENV{ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)

if("${ARCH}" STREQUAL "arm")
  set(TOOLCHAIN_C_FLAGS
    -mthumb
    -mcpu=${GCC_M_CPU}
    )

  include($ENV{ZEPHYR_BASE}/cmake/fpu-for-gcc-m-cpu.cmake)

  if(CONFIG_FLOAT)
    list(APPEND TOOLCHAIN_C_FLAGS -mfpu=${FPU_FOR_${GCC_M_CPU}})
    if    (CONFIG_FP_SOFTABI)
      list(APPEND TOOLCHAIN_C_FLAGS -mfloat-abi=soft)
    elseif(CONFIG_FP_HARDABI)
      list(APPEND TOOLCHAIN_C_FLAGS -mfloat-abi=hard)
    endif()
  endif()
elseif("${ARCH}" STREQUAL "arc")
  set(TOOLCHAIN_C_FLAGS
	  -mcpu=${GCC_M_CPU}
    )
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --sysroot ${SYSROOT_DIR} --print-libgcc-file-name
  OUTPUT_VARIABLE LIBGCC_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

execute_process(
  COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --sysroot ${SYSROOT_DIR} --print-multi-directory
  OUTPUT_VARIABLE NEWLIB_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

get_filename_component(LIBGCC_DIR ${LIBGCC_DIR} DIRECTORY)

LIST(APPEND LIB_INCLUDE_DIR -L${LIBGCC_DIR})
LIST(APPEND TOOLCHAIN_LIBS gcc)

set(LIBC_INCLUDE_DIR ${SYSROOT_DIR}/usr/include)
set(LIBC_LIBRARY_DIR ${SYSROOT_DIR}/usr/lib/${NEWLIB_DIR})

# For CMake to be able to test if a compiler flag is supported by the
# toolchain we need to give CMake the necessary flags to compile and
# link a dummy C file.
#
# CMake checks compiler flags with check_c_compiler_flag() (Which we
# wrap with target_cc_option() in extentions.cmake)
set(CMAKE_REQUIRED_FLAGS "-nostartfiles -nostdlib --sysroot=${SYSROOT_DIR} -Wl,--unresolved-symbols=ignore-in-object-files")


