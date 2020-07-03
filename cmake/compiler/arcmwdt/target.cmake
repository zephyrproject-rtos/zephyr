# find the compilers for C, CPP, assembly
find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}ccac PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER ${CROSS_COMPILE}ccac PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_ASM_COMPILER ${CROSS_COMPILE}ccac PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
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

# For CMake to be able to test if a compiler flag is supported by the
# toolchain we need to give CMake the necessary flags to compile and
# link a dummy C file.
#
# CMake checks compiler flags with check_c_compiler_flag() (Which we
# wrap with target_cc_option() in extentions.cmake)
foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
endforeach()

# common compile options, no copyright msg, little-endian, no small data
list(APPEND TOOLCHAIN_C_FLAGS -Hnocopyr -HL -Hnosdata)

# based flags for assembly
macro(toolchain_cc_asm_base_flags dest_var_name)
  # Specify assembly as the source language for the preprocessor to expect
  set_ifndef(${dest_var_name} "-Hasmcpp")
endmacro()

# options to not include default library
macro(toolchain_cc_nostdinc)
  if (NOT CONFIG_NEWLIB_LIBC AND
    NOT CONFIG_NATIVE_APPLICATION)
    zephyr_compile_options( -Hno_default_include -Hnoarcexlib)
    zephyr_system_include_directories(${NOSTDINC})
  endif()
endmacro()

# do not link in supplied run-time startup files
macro(toolchain_cc_freestanding)
  zephyr_compile_options(-Hnocrt)
endmacro()

# options to generate debug information
macro(toolchain_cc_produce_debug_info)
  zephyr_compile_options(-g)
endmacro()

# compile common globals like normal definitions
macro(toolchain_cc_nocommon)
  zephyr_compile_options(-fno-common)
endmacro()

# c std options
macro(toolchain_cc_cstd_flag dest_var_name c_std)
# mwdt supports: c89, c99, gnu89, gnu99, c++11. c++98
  if (${c_std} STREQUAL "c99")
    set_ifndef(${dest_var_name} "-std=gnu99")
  elseif(${c_std} STREQUAL "c89")
    set_ifndef(${dest_var_name} "-std=gnu89")
  endif()
endmacro()

# coverage options
macro(toolchain_cc_coverage)
# mwdt's coverage mechanism is different with gnu
# at present, zephyr only support gnu coverage
endmacro()

# base options for cpp
macro(toolchain_cc_cpp_base_flags dest_list_name)
endmacro()

# C++ std options
# The "register" keyword was deprecated since C++11, but not for C++98
macro(toolchain_cc_cpp_dialect_std_98_flags dest_list_name)
  list(APPEND ${dest_list_name} "-std=c++98")
endmacro()
macro(toolchain_cc_cpp_dialect_std_11_flags dest_list_name)
  list(APPEND ${dest_list_name} "-std=c++11")
endmacro()
macro(toolchain_cc_cpp_dialect_std_14_flags dest_list_name)
#no support of C++14
endmacro()
macro(toolchain_cc_cpp_dialect_std_17_flags dest_list_name)
#no support of C++17"
endmacro()
macro(toolchain_cc_cpp_dialect_std_2a_flags dest_list_name)
#no support of C++2a"
endmacro()

# no exceptions for C++
macro(toolchain_cc_cpp_no_exceptions_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-fno-exceptions")
endmacro()

# no rtti for c++
macro(toolchain_cc_cpp_no_rtti_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-fno-rtti")
endmacro()

#  include only macros defined in a file
macro(toolchain_cc_imacros header_file)
  zephyr_compile_options(-imacros${header_file})
endmacro()

# optimizaiton options
macro(toolchain_cc_optimize_for_no_optimizations_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-O0")
endmacro()

# optimizaiton for debug
macro(toolchain_cc_optimize_for_debug_flag dest_var_name)
    set_ifndef(${dest_var_name}  "-O0")
endmacro()

macro(toolchain_cc_optimize_for_speed_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-O2")
endmacro()

macro(toolchain_cc_optimize_for_size_flag dest_var_name)
  set_ifndef(${dest_var_name}  "-Os")
endmacro()

macro(toolchain_cc_asan)
#no support of -fsanitize=address and -lasan
endmacro()

macro(toolchain_cc_ubsan)
#no support of -fsanitize=undefined"
endmacro()

macro(toolchain_cc_security_canaries)
  zephyr_compile_options(-fstack-protector-all)
#no support of -mstack-protector-guard=global"
endmacro()

macro(toolchain_cc_security_fortify)
#no support of _FORTIFY_SOURCE"
endmacro()

# options for warnings
# base options
macro(toolchain_cc_warning_base)
  zephyr_compile_options(
      -Wformat
      -Wformat-security
      -Wno-format-zero-length
      -Wno-main-return-type
      -Wno-unaligned-pointer-conversion
      -Wno-incompatible-pointer-types-discards-qualifiers
  )
  zephyr_cc_option(-Wno-pointer-sign)

  # Prohibit void pointer arithmetic. Illegal in C99
  zephyr_cc_option(-Wpointer-arith)

endmacro()

# level 1 warning options
macro(toolchain_cc_warning_dw_1)
  zephyr_compile_options(
    -Wextra
    -Wunused
    -Wno-unused-parameter
    -Wmissing-declarations
    -Wmissing-format-attribute
    )
  zephyr_cc_option(
    -Wold-style-definition
    -Wmissing-prototypes
    -Wmissing-include-dirs
    -Wunused-but-set-variable
    -Wno-missing-field-initializers
    )
endmacro()

# level 2 warning options
macro(toolchain_cc_warning_dw_2)
 zephyr_compile_options(
    -Waggregate-return
    -Wcast-align
    -Wdisabled-optimization
    -Wnested-externs
    -Wshadow
    )
  zephyr_cc_option(
    -Wlogical-op
    -Wmissing-field-initializers
    )
endmacro()

# level 3 warning options
macro(toolchain_cc_warning_dw_3)
 zephyr_compile_options(
    -Wbad-function-cast
    -Wcast-qual
    -Wconversion
    -Wpacked
    -Wpadded
    -Wpointer-arith
    -Wredundant-decls
    -Wswitch-default
    )
  zephyr_cc_option(
    -Wpacked-bitfield-compat
    -Wvla
    )
endmacro()

# extended warning options
macro(toolchain_cc_warning_extended)
  zephyr_cc_option(
    -Wno-sometimes-uninitialized
    -Wno-shift-overflow
    -Wno-missing-braces
    -Wno-self-assign
    -Wno-address-of-packed-member
    -Wno-unused-function
    -Wno-initializer-overrides
    -Wno-section
    -Wno-unknown-warning-option
    -Wno-unused-variable
    -Wno-format-invalid-specifier
    -Wno-gnu
    # comparison of unsigned expression < 0 is always false
    -Wno-tautological-compare
    )

endmacro()

macro(toolchain_cc_warning_error_implicit_int)
  zephyr_cc_option(-Werror=implicit-int)
endmacro()

#
# The following macros leaves it up to the root CMakeLists.txt to choose
# the variables in which to put the requested flags, and whether or not
# to call the macros
#

macro(toolchain_cc_warning_error_misra_sane dest_var_name)
  set_ifndef(${dest_var_name} "-Werror=vla")
endmacro()

macro(toolchain_cc_cpp_warning_error_misra_sane dest_var_name)
  set_ifndef(${dest_var_name} "-Werror=vla")
endmacro()

# List the warnings that are not supported for C++ compilations

list(APPEND CXX_EXCLUDED_OPTIONS
  -Werror=implicit-int
  -Wold-style-definition
  )
