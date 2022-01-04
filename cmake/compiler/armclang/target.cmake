# find the compilers for C, CPP, assembly
find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}armclang PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER ${CROSS_COMPILE}armclang PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_ASM_COMPILER ${CROSS_COMPILE}armclang PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

# The CMAKE_REQUIRED_FLAGS variable is used by check_c_compiler_flag()
# (and other commands which end up calling check_c_source_compiles())
# to add additional compiler flags used during checking. These flags
# are unused during "real" builds of Zephyr source files linked into
# the final executable.
#
include(${ZEPHYR_BASE}/cmake/gcc-m-cpu.cmake)
set(CMAKE_SYSTEM_PROCESSOR ${GCC_M_CPU})

list(APPEND TOOLCHAIN_C_FLAGS
  -fshort-enums
  )

if(CONFIG_ARM64)
  list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})

  list(APPEND TOOLCHAIN_C_FLAGS   -mabi=lp64)
  list(APPEND TOOLCHAIN_LD_FLAGS  -mabi=lp64)
else()
  list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})

  if(CONFIG_COMPILER_ISA_THUMB2)
    list(APPEND TOOLCHAIN_C_FLAGS   -mthumb)
  endif()

  list(APPEND TOOLCHAIN_C_FLAGS -mabi=aapcs)

  # Defines a mapping from GCC_M_CPU to FPU

  if(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
    set(PRECISION_TOKEN)
  else()
    set(PRECISION_TOKEN sp-)
  endif()

  set(FPU_FOR_cortex-m4      fpv4-${PRECISION_TOKEN}d16)
  set(FPU_FOR_cortex-m7      fpv5-${PRECISION_TOKEN}d16)
  set(FPU_FOR_cortex-m33     fpv5-${PRECISION_TOKEN}d16)

  if(CONFIG_FPU)
    list(APPEND TOOLCHAIN_C_FLAGS   -mfpu=${FPU_FOR_${GCC_M_CPU}})
    if    (CONFIG_FP_SOFTABI)
      list(APPEND TOOLCHAIN_C_FLAGS   -mfloat-abi=softfp)
    elseif(CONFIG_FP_HARDABI)
      list(APPEND TOOLCHAIN_C_FLAGS   -mfloat-abi=hard)
    endif()
  endif()
endif()

foreach(file_name include/stddef.h)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
    OUTPUT_VARIABLE _OUTPUT
    )
  get_filename_component(_OUTPUT "${_OUTPUT}" DIRECTORY)
  string(REGEX REPLACE "\n" "" _OUTPUT ${_OUTPUT})

  list(APPEND NOSTDINC ${_OUTPUT})
endforeach()

foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem ${isystem_include_dir})
endforeach()

set(CMAKE_REQUIRED_FLAGS ${isystem_include_flags})
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

if(CONFIG_ARMCLANG_STD_LIBC)
  # Zephyr requires AEABI portability to ensure correct functioning of the C
  # library, for example error numbers, errno.h.
  list(APPEND TOOLCHAIN_C_FLAGS -D_AEABI_PORTABILITY_LEVEL=1)
endif()
