# find the compilers for C, CPP, assembly
find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}ccac PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER ${CROSS_COMPILE}ccac PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_ASM_COMPILER ${CROSS_COMPILE}ccac PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
# The CMAKE_REQUIRED_FLAGS variable is used by check_c_compiler_flag()
# (and other commands which end up calling check_c_source_compiles())
# to add additional compiler flags used during checking. These flags
# are unused during "real" builds of Zephyr source files linked into
# the final executable.
#
# Appending onto any existing values lets users specify
# toolchain-specific flags at generation time.
list(APPEND CMAKE_REQUIRED_FLAGS
  -c
  -HL
  -Hnosdata
  -Hnolib
  -Hnocrt
  -Hnoentry
  -Hldopt=-Bbase=0x0 # Set an entry point to avoid a warning
  -Werror
  )
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

set(NOSTDINC "")

list(APPEND NOSTDINC ${TOOLCHAIN_HOME}/arc/inc)

if(CONFIG_ARCMWDT_LIBC AND CONFIG_LIB_CPLUSPLUS)
  list(APPEND NOSTDINC ${TOOLCHAIN_HOME}/arc/lib/src/c++/inc)
endif()

# For CMake to be able to test if a compiler flag is supported by the
# toolchain we need to give CMake the necessary flags to compile and
# link a dummy C file.
#
# CMake checks compiler flags with check_c_compiler_flag() (Which we
# wrap with target_cc_option() in extensions.cmake)
foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
endforeach()

# common compile options, no copyright msg, little-endian, no small data,
# no MWDT stack checking
list(APPEND TOOLCHAIN_C_FLAGS -Hnocopyr -HL -Hnosdata -Hoff=Stackcheck_alloca)

# The MWDT compiler can replace some code with call to builtin functions.
# We can't rely on these functions presence if we don't use MWDT libc.
# NOTE: the option name '-fno-builtin' is misleading a bit - we still can
# manually call __builtin_** functions even if we specify it.
if(NOT CONFIG_ARCMWDT_LIBC)
  list(APPEND TOOLCHAIN_C_FLAGS -fno-builtin)
endif()
