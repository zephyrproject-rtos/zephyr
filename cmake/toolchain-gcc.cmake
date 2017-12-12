# Configures CMake for using GCC, this script is re-used by several
# GCC-based toolchains

set(CMAKE_C_COMPILER   ${CROSS_COMPILE}gcc     CACHE INTERNAL " " FORCE)
set(CMAKE_OBJCOPY      ${CROSS_COMPILE}objcopy CACHE INTERNAL " " FORCE)
set(CMAKE_OBJDUMP      ${CROSS_COMPILE}objdump CACHE INTERNAL " " FORCE)
#set(CMAKE_LINKER      ${CROSS_COMPILE}ld      CACHE INTERNAL " " FORCE) # Not in use yet
set(CMAKE_AR           ${CROSS_COMPILE}ar      CACHE INTERNAL " " FORCE)
set(CMAKE_RANLILB      ${CROSS_COMPILE}ranlib  CACHE INTERNAL " " FORCE)
set(CMAKE_READELF      ${CROSS_COMPILE}readelf CACHE INTERNAL " " FORCE)
set(CMAKE_GDB          ${CROSS_COMPILE}gdb     CACHE INTERNAL " " FORCE)
set(CMAKE_NM           ${CROSS_COMPILE}nm      CACHE INTERNAL " " FORCE)

assert_exists(CMAKE_READELF)

if(CONFIG_CPLUSPLUS)
  set(cplusplus_compiler ${CROSS_COMPILE}g++)
else()
  if(EXISTS ${CROSS_COMPILE}g++)
    set(cplusplus_compiler ${CROSS_COMPILE}g++)
  else()
    # When the toolchain doesn't support C++, and we aren't building
    # with C++ support just set it to something so CMake doesn't
    # crash, it won't actually be called
    set(cplusplus_compiler ${CMAKE_C_COMPILER})
  endif()
endif()
set(CMAKE_CXX_COMPILER ${cplusplus_compiler}     CACHE INTERNAL " " FORCE)

set(NOSTDINC "")

foreach(file_name include include-fixed)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
    OUTPUT_VARIABLE _OUTPUT
    )
  string(REGEX REPLACE "\n" "" _OUTPUT ${_OUTPUT})

  if(MSYS)
    # TODO: Remove this when
    # https://github.com/zephyrproject-rtos/zephyr/issues/4687 is
    # resolved
    execute_process(
      COMMAND cygpath -u ${_OUTPUT}
      OUTPUT_VARIABLE _OUTPUT
      )
    string(REGEX REPLACE "\n" "" _OUTPUT ${_OUTPUT})
  endif()

  list(APPEND NOSTDINC ${_OUTPUT})
endforeach()

include($ENV{ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)

if("${ARCH}" STREQUAL "arm")
  list(APPEND TOOLCHAIN_C_FLAGS
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
  list(APPEND TOOLCHAIN_C_FLAGS
	  -mcpu=${GCC_M_CPU}
    )
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --print-libgcc-file-name
  OUTPUT_VARIABLE LIBGCC_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

execute_process(
  COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --print-multi-directory
  OUTPUT_VARIABLE NEWLIB_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

get_filename_component(LIBGCC_DIR ${LIBGCC_DIR} DIRECTORY)

assert(LIBGCC_DIR "LIBGCC_DIR not found")

LIST(APPEND LIB_INCLUDE_DIR "-L\"${LIBGCC_DIR}\"")
LIST(APPEND TOOLCHAIN_LIBS gcc)

set(LIBC_INCLUDE_DIR ${SYSROOT_DIR}/include)
set(LIBC_LIBRARY_DIR "\"${SYSROOT_DIR}\"/lib/${NEWLIB_DIR}")

# For CMake to be able to test if a compiler flag is supported by the
# toolchain we need to give CMake the necessary flags to compile and
# link a dummy C file.
#
# CMake checks compiler flags with check_c_compiler_flag() (Which we
# wrap with target_cc_option() in extentions.cmake)
foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
endforeach()
set(CMAKE_REQUIRED_FLAGS -nostartfiles -nostdlib ${isystem_include_flags} -Wl,--unresolved-symbols=ignore-in-object-files)
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
