# SPDX-License-Identifier: Apache-2.0

# *DOCUMENTATION*
#
# Note that this is *NOT* the top-level CMakeLists.txt. That's in the
# application. See the Application Development Primer documentation
# for details.
#
# To see a list of typical targets execute "make usage"
# More info can be located in ./README.rst
# Comments in this file are targeted only to the developer, do not
# expect to learn how to build the kernel reading this file.

if(NOT DEFINED ZEPHYR_BINARY_DIR)
  message(FATAL_ERROR "A user error has occurred.
cmake was invoked with '${CMAKE_CURRENT_LIST_DIR}' specified as the source directory,
but it must be invoked with an application source directory,
such as '${CMAKE_CURRENT_LIST_DIR}/samples/hello_world'.
Debug variables:
CMAKE_CACHEFILE_DIR: ${CMAKE_CACHEFILE_DIR}
")
endif()


# See https://gitlab.kitware.com/cmake/cmake/issues/16228
# and https://cmake.org/pipermail/cmake/2019-May/thread.html#69496
if(NOT ZEPHYR_BASE STREQUAL CMAKE_CURRENT_SOURCE_DIR)
message(WARNING "ZEPHYR_BASE doesn't match CMAKE_CURRENT_SOURCE_DIR
  ZEPHYR_BASE              = ${ZEPHYR_BASE}
  PWD                      = $ENV{PWD}
  CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}
You may be using a mix of symbolic links and real paths which causes \
subtle and hard to debug CMake issues.")
endif()
# For Zephyr more specifically this breaks (at least)
#     -fmacro-prefix-map=${ZEPHYR_BASE}=


# In some cases the "final" things are not used at all and "_prebuilt"
# is the last station. See "logical_target_for_zephyr_elf" below for
# details.
set(CMAKE_EXECUTABLE_SUFFIX .elf)

# Zephyr build system will use a dynamic number of linking stages based on build
# configuration.
#
# Currently up to three linking stages may be executed:
# zephyr_pre0:  First linking stage
# zephyr_pre1:  Second linking stage
# zephyr_final: Final linking stage
#
# There will at minimum be a single linking stage.
# When only a single linking stage is required, the `zephyr_pre0` will be mapped
# into the `zephyr_final` target.
#
# Multiple linking stages are required in the following cases:
# - device dependencies structs must be generated (CONFIG_DEVICE_DEPS=y)
# - ISR tables must be generated (CONFIG_GEN_ISR_TABLES=y)
# - Kernel objects hash tables (CONFIG_USERSPACE=y)
# - Application memory partitions (CONFIG_USERSPACE=y)
#
# Some generators require that memory locations has been fixed, thus those are
# placed at the second linking stage.
#
# When all three linking stages are active, then the following properties applies:
# - zephyr_pre0:  linker sections may resize / addresses may relocate
# - zephyr_pre1:  All linker section sizes are fixed, addresses cannot change
# - zephyr_final: Final image.
#
set(ZEPHYR_CURRENT_LINKER_PASS 0)
set(ZEPHYR_CURRENT_LINKER_CMD    linker_zephyr_pre${ZEPHYR_CURRENT_LINKER_PASS}.cmd)
set(ZEPHYR_LINK_STAGE_EXECUTABLE zephyr_pre${ZEPHYR_CURRENT_LINKER_PASS})

# Make kconfig variables available to the linker script generator
zephyr_linker_include_generated(KCONFIG ${CMAKE_CURRENT_BINARY_DIR}/.config)

# The linker generator also needs sections.h to be able to access e.g. _APP_SMEM_SECTION_NAME
# for linkerscripts that do not support c-preprocessing.
zephyr_linker_include_generated(HEADER ${ZEPHYR_BASE}/include/zephyr/linker/sections.h)

zephyr_linker_include_var(VAR CMAKE_VERBOSE_MAKEFILE VALUE ${CMAKE_VERBOSE_MAKEFILE})

# ZEPHYR_PREBUILT_EXECUTABLE is used outside of this file, therefore keep the
# existing variable to allow slowly cleanup of linking stage handling.
# Three stage linking active: pre0 -> pre1 -> final, this will correspond to `pre1`
# Two stage linking active:   pre0 -> final, this will correspond to `pre0`
if(CONFIG_USERSPACE OR CONFIG_DEVICE_DEPS)
  set(ZEPHYR_PREBUILT_EXECUTABLE zephyr_pre1)
else()
  set(ZEPHYR_PREBUILT_EXECUTABLE zephyr_pre0)
endif()
set(ZEPHYR_FINAL_EXECUTABLE zephyr_final)

# Set some phony targets to collect dependencies
set(OFFSETS_H_TARGET           offsets_h)
set(SYSCALL_LIST_H_TARGET      syscall_list_h_target)
set(DRIVER_VALIDATION_H_TARGET driver_validation_h_target)
set(KOBJ_TYPES_H_TARGET        kobj_types_h_target)
set(PARSE_SYSCALLS_TARGET      parse_syscalls_target)
set(DEVICE_API_LD_TARGET       device_api_ld_target)

define_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT BRIEF_DOCS " " FULL_DOCS " ")
set_property(   GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-little${ARCH}) # BFD format

# Contains the list of files with syscall function prototypes.
add_library(syscalls_interface INTERFACE)
set(syscalls_file_list_output
    ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/syscalls_file_list.txt)

# "zephyr_interface" is a source-less library that encapsulates all the global
# compiler options needed by all source files. All zephyr libraries,
# including the library named "zephyr" link with this library to
# obtain these flags.
# https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#interface-libraries
add_library(zephyr_interface INTERFACE)

# "zephyr" is a catch-all CMake library for source files that can be
# built purely with the include paths, defines, and other compiler
# flags that come with zephyr_interface.
zephyr_library_named(zephyr)

if(CONFIG_LEGACY_GENERATED_INCLUDE_PATH)
  zephyr_include_directories(${PROJECT_BINARY_DIR}/include/generated/zephyr)
endif()

zephyr_include_directories(
  include
  ${PROJECT_BINARY_DIR}/include/generated
  ${USERINCLUDE}
  ${STDINCLUDE}
)

include(${ZEPHYR_BASE}/cmake/linker_script/${ARCH}/linker.cmake OPTIONAL)

zephyr_include_directories(${SOC_FULL_DIR})

# Don't inherit compiler flags from the environment
foreach(var AFLAGS CFLAGS CXXFLAGS CPPFLAGS LDFLAGS)
  if(DEFINED ENV{${var}})
    message(WARNING "The environment variable '${var}' was set to $ENV{${var}}, "
                    "but Zephyr ignores flags from the environment. Use 'cmake "
                    "-DEXTRA_${var}=$ENV{${var}}' instead."
    )
    unset(ENV{${var}})
  endif()
endforeach()

zephyr_compile_definitions(
  KERNEL
  __ZEPHYR__=1
)

# Ensure that include/zephyr/toolchain.h includes toolchain/other.h for all off-tree toolchains
if(TOOLCHAIN_USE_CUSTOM)
  zephyr_compile_definitions(__TOOLCHAIN_CUSTOM__)
endif()

# @Intent: Set compiler specific flag for disabling strict aliasing rule
zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,no_strict_aliasing>>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,no_strict_aliasing>>)

# Extra warnings options for twister run
if (CONFIG_COMPILER_WARNINGS_AS_ERRORS)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warnings_as_errors>>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,warnings_as_errors>>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:asm,warnings_as_errors>>)
  zephyr_link_libraries($<TARGET_PROPERTY:linker,warnings_as_errors>)
endif()

# @Intent: Set compiler flags to enable buffer overflow checks in libc functions
# @details:
#  Kconfig.zephyr "Detect buffer overflows in libc calls" is a kconfig choice,
#  ensuring at most *one* of CONFIG_FORTIFY_SOURCE_{COMPILE_TIME,RUN_TIME} is
#  set. Refer to Kconfig.zephyr for selection logic and description of these
#  choices. Toolchains set both of the security_fortify_{compile_time,run_time}
#  properties and the Kconfig settings are used here to select between those.
#
if(CONFIG_FORTIFY_SOURCE_RUN_TIME)
  zephyr_compile_definitions($<TARGET_PROPERTY:compiler,security_fortify_run_time> )
elseif(CONFIG_FORTIFY_SOURCE_COMPILE_TIME)
  zephyr_compile_definitions($<TARGET_PROPERTY:compiler,security_fortify_compile_time> )
endif()

# @Intent: Set compiler flags to detect general stack overflows across all functions
if(CONFIG_STACK_CANARIES)
  zephyr_compile_options("$<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,security_canaries>>")
  zephyr_compile_options("$<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,security_canaries>>")
elseif(CONFIG_STACK_CANARIES_STRONG)
  zephyr_compile_options("$<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,security_canaries_strong>>")
  zephyr_compile_options("$<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,security_canaries_strong>>")
elseif(CONFIG_STACK_CANARIES_ALL)
  zephyr_compile_options("$<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,security_canaries_all>>")
  zephyr_compile_options("$<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,security_canaries_all>>")
elseif(CONFIG_STACK_CANARIES_EXPLICIT)
  zephyr_compile_options("$<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,security_canaries_explicit>>")
  zephyr_compile_options("$<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,security_canaries_explicit>>")
endif()

# @Intent: Obtain compiler optimizations flags and store in variables
# @details:
#   Kconfig.zephyr "Optimization level" is a kconfig choice, ensuring
#   only *one* of CONFIG_{NO,DEBUG,SPEED,SIZE}_OPTIMIZATIONS is set.
#   Refer to Kconfig.zephyr for selection logic and description of these choices.
#   toolchain_cc_optimize_*() macros must provide the mapping from these kconfigs
#   to compiler flags. Each macro will store the flags in a CMake variable, whose
#   name is passed as argument (somewhat like by reference).
#
#   If the user wants to tweak the optimizations, there are two ways:
#    1) Using EXTRA_CFLAGS which is applied regardless of kconfig choice, or
#    2) Rely on override support being implemented by your toolchain_cc_optimize_*()
#
get_property(OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG TARGET compiler PROPERTY no_optimization)
get_property(OPTIMIZE_FOR_DEBUG_FLAG TARGET compiler PROPERTY optimization_debug)
get_property(OPTIMIZE_FOR_SPEED_FLAG TARGET compiler PROPERTY optimization_speed)
get_property(OPTIMIZE_FOR_SIZE_FLAG  TARGET compiler PROPERTY optimization_size)
get_property(OPTIMIZE_FOR_SIZE_AGGRESSIVE_FLAG TARGET compiler PROPERTY optimization_size_aggressive)

# From kconfig choice, pick the actual OPTIMIZATION_FLAG to use.
# Kconfig choice ensures only one of these CONFIG_*_OPTIMIZATIONS is set.
if(CONFIG_NO_OPTIMIZATIONS)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG})
elseif(CONFIG_DEBUG_OPTIMIZATIONS)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_DEBUG_FLAG})
elseif(CONFIG_SPEED_OPTIMIZATIONS)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SPEED_FLAG})
elseif(CONFIG_SIZE_OPTIMIZATIONS)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SIZE_FLAG}) # Default in kconfig
elseif(CONFIG_SIZE_OPTIMIZATIONS_AGGRESSIVE)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SIZE_AGGRESSIVE_FLAG})
else()
  message(FATAL_ERROR
    "Unreachable code. Expected optimization level to have been chosen. See Kconfig.zephyr")
endif()

if(NOT CONFIG_ARCH_IS_SET)
  message(WARNING "\
None of the CONFIG_<arch> (e.g. CONFIG_X86) symbols are set. \
Select one of them from the SOC_SERIES_* symbol or, lacking that, from the \
SOC_* symbol.")
endif()

# Apply the final optimization flag(s)
zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:${OPTIMIZATION_FLAG}>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${OPTIMIZATION_FLAG}>)

if(CONFIG_LTO)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,optimization_lto>)
  add_link_options($<TARGET_PROPERTY:linker,lto_arguments>)
endif()

if(CONFIG_STD_C23)
  set(CSTD c2x)
elseif(CONFIG_STD_C17)
  set(CSTD c17)
elseif(CONFIG_STD_C11)
  set(CSTD c11)
elseif(CONFIG_STD_C99)
  set(CSTD c99)
elseif(CONFIG_STD_C90)
  set(CSTD c90)
else()
  message(FATAL_ERROR "Unreachable code. Expected C standard to have been chosen.")
endif()

if(CONFIG_GNU_C_EXTENSIONS)
  string(REPLACE "c" "gnu" CSTD "${CSTD}")
endif()

list(APPEND CMAKE_C_COMPILE_FEATURES ${compile_features_${CSTD}})

# @Intent: Obtain compiler specific flags related to C++ that are not influenced by kconfig
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,required>>)

# @Intent: Obtain compiler specific flags for compiling under different ISO standards of C++
if(CONFIG_CPP)
  # From kconfig choice, pick a single dialect.
  # Kconfig choice ensures only one of these CONFIG_STD_CPP* is set.
  if(CONFIG_STD_CPP98)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp98>)
    list(APPEND CMAKE_CXX_COMPILE_FEATURES ${compile_features_cpp98})
  elseif(CONFIG_STD_CPP11)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp11>) # Default in kconfig
    list(APPEND CMAKE_CXX_COMPILE_FEATURES ${compile_features_cpp11})
  elseif(CONFIG_STD_CPP14)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp14>)
    list(APPEND CMAKE_CXX_COMPILE_FEATURES ${compile_features_cpp14})
  elseif(CONFIG_STD_CPP17)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp17>)
    list(APPEND CMAKE_CXX_COMPILE_FEATURES ${compile_features_cpp17})
  elseif(CONFIG_STD_CPP2A)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp2a>)
    list(APPEND CMAKE_CXX_COMPILE_FEATURES ${compile_features_cpp20})
  elseif(CONFIG_STD_CPP20)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp20>)
    list(APPEND CMAKE_CXX_COMPILE_FEATURES ${compile_features_cpp20})
  elseif(CONFIG_STD_CPP2B)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp2b>)
    list(APPEND CMAKE_CXX_COMPILE_FEATURES ${compile_features_cpp20})
  else()
    message(FATAL_ERROR
      "Unreachable code. Expected C++ standard to have been chosen. See Kconfig.zephyr.")
  endif()
  set(CMAKE_CXX_COMPILE_FEATURES ${CMAKE_CXX_COMPILE_FEATURES} PARENT_SCOPE)

  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${STD_CPP_DIALECT_FLAGS}>)
endif()

if(NOT CONFIG_CPP_EXCEPTIONS)
  # @Intent: Obtain compiler specific flags related to C++ Exceptions
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,no_exceptions>>)
endif()

if(NOT CONFIG_CPP_RTTI)
  # @Intent: Obtain compiler specific flags related to C++ Run Time Type Information
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,no_rtti>>)
endif()

if(CONFIG_MISRA_SANE)
  # @Intent: Obtain toolchain compiler flags relating to MISRA.
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_error_misra_sane>>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_error_misra_sane>>)
endif()

# This is intend to be temporary. Once we have fixed the violations that
# prevents build Zephyr, these flags shall be part of the default flags.
if(CONFIG_CODING_GUIDELINE_CHECK)
  # @Intent: Obtain toolchain compiler flags relating to coding guideline
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_error_coding_guideline>>)
endif()

# @Intent: Set compiler specific macro inclusion of AUTOCONF_H
zephyr_compile_options("SHELL: $<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,imacros> ${AUTOCONF_H}>")
zephyr_compile_options("SHELL: $<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,imacros> ${AUTOCONF_H}>")
zephyr_compile_options("SHELL: $<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:asm,imacros> ${AUTOCONF_H}>")

if(CONFIG_COMPILER_FREESTANDING)
  # @Intent: Set compiler specific flag for bare metal freestanding option
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,freestanding>>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:compiler,freestanding>>)
endif()

if (CONFIG_PICOLIBC AND NOT CONFIG_PICOLIBC_IO_FLOAT)
  # @Intent: Set compiler specific flag to disable printf-related optimizations
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,no_printf_return_value>>)
endif()

# @Intent: Set compiler specific flag for tentative definitions, no-common
zephyr_compile_options($<TARGET_PROPERTY:compiler,no_common>)

# @Intent: Set compiler specific flag for production of debug information
zephyr_compile_options($<TARGET_PROPERTY:compiler,debug>)

if(CONFIG_COMPILER_SAVE_TEMPS)
  # @Intent: Set compiler specific flag for saving temporary object files
  zephyr_compile_options($<TARGET_PROPERTY:compiler,save_temps>)
endif()

if(NOT CONFIG_COMPILER_TRACK_MACRO_EXPANSION)
  # @Intent: Set compiler specific flags to not track macro expansion
  zephyr_compile_options($<TARGET_PROPERTY:compiler,no_track_macro_expansion>)
endif()

if(CONFIG_COMPILER_COLOR_DIAGNOSTICS)
# @Intent: Set compiler specific flag for diagnostic messages
zephyr_compile_options($<TARGET_PROPERTY:compiler,diagnostic>)
endif()

zephyr_compile_options(
  ${TOOLCHAIN_C_FLAGS}
)

# @Intent: Obtain compiler specific flags related to assembly
# ToDo: Remember to get feedback from Oticon on this, as they might use the `ASM_BASE_FLAG` since this is done this way.
zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:asm,required>>)

# @Intent: Enforce standard integer type correspondence to match Zephyr usage.
# (must be after compiler specific flags)
if(CONFIG_ENFORCE_ZEPHYR_STDINT)
  zephyr_compile_options("SHELL:$<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,imacros> ${ZEPHYR_BASE}/include/zephyr/toolchain/zephyr_stdint.h>")
  zephyr_compile_options("SHELL:$<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,imacros> ${ZEPHYR_BASE}/include/zephyr/toolchain/zephyr_stdint.h>")
  zephyr_compile_options("SHELL:$<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:asm,imacros> ${ZEPHYR_BASE}/include/zephyr/toolchain/zephyr_stdint.h>")
endif()

# Common toolchain-agnostic assembly flags
zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:ASM>:-D_ASMLANGUAGE>
)

find_package(Deprecated COMPONENTS toolchain_ld_base)

if(DEFINED TOOLCHAIN_LD_FLAGS)
  zephyr_ld_options(${TOOLCHAIN_LD_FLAGS})
endif()

zephyr_link_libraries(PROPERTY base)

zephyr_link_libraries_ifndef(CONFIG_LINKER_USE_RELAX PROPERTY no_relax)

zephyr_link_libraries_ifdef(CONFIG_LINKER_USE_RELAX PROPERTY relax)

# Sort the common symbols and each input section by alignment
# in descending order to minimize padding between these symbols.
zephyr_link_libraries_ifdef(CONFIG_LINKER_SORT_BY_ALIGNMENT PROPERTY sort_alignment)

toolchain_ld_force_undefined_symbols(
  _OffsetAbsSyms
  _ConfigAbsSyms
)

if(NOT CONFIG_NATIVE_BUILD)
  find_package(Deprecated COMPONENTS toolchain_ld_baremetal)

  zephyr_link_libraries(PROPERTY baremetal)

  # Note that some architectures will skip this flag if set to error, even
  # though the compiler flag check passes (e.g. ARC and Xtensa). So warning
  # should be the default for now.
  #
  # Skip this for native application as Zephyr only provides
  # additions to the host toolchain linker script. The relocation
  # sections (.rel*) requires us to override those provided
  # by host toolchain. As we can't account for all possible
  # combination of compiler and linker on all machines used
  # for development, it is better to turn this off.
  #
  # CONFIG_LINKER_ORPHAN_SECTION_PLACE is to place the orphan sections
  # without any warnings or errors, which is the default behavior.
  # So there is no need to explicitly set a linker flag.
  if(CONFIG_LINKER_ORPHAN_SECTION_WARN)
    zephyr_link_libraries(PROPERTY orphan_warning)
  elseif(CONFIG_LINKER_ORPHAN_SECTION_ERROR)
    zephyr_link_libraries(PROPERTY orphan_error)
  endif()
endif()

if(CONFIG_CPP)
  if(NOT CONFIG_MINIMAL_LIBCPP AND NOT CONFIG_NATIVE_LIBRARY)
    find_package(Deprecated COMPONENTS toolchain_ld_cpp)
  endif()

  zephyr_link_libraries(PROPERTY cpp_base)
endif()

# @Intent: Add the basic toolchain warning flags
zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_base>>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_base>>)

# ==========================================================================
#
# cmake -DW=... settings
#
# W=1 - warnings that may be relevant and does not occur too often
# W=2 - warnings that occur quite often but may still be relevant
# W=3 - the more obscure warnings, can most likely be ignored
# ==========================================================================
# @Intent: Add cmake -DW toolchain supported warnings, if any
if(W MATCHES "1")
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_dw_1>>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_dw_1>>)
endif()

if(W MATCHES "2")
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_dw_2>>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_dw_2>>)
endif()

if(W MATCHES "3")
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_dw_3>>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_dw_3>>)
endif()

# @Intent: Add extended, more specific, toolchain warning flags
zephyr_compile_options($<TARGET_PROPERTY:compiler,warning_extended>)

# @Intent: Trigger an error when a declaration does not specify a type
zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_error_implicit_int>>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_error_implicit_int>>)

# @Intent: Do not make position independent code / executable
zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,no_position_independent>>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler,no_position_independent>>)

# In case of CONFIG_NATIVE_LIBRARY we also don't want position independent code,
# but we cannot tell that to the linker yet as we are first only doing a
# relocatable link into a static library
zephyr_link_libraries_ifndef(CONFIG_NATIVE_LIBRARY
                             $<TARGET_PROPERTY:linker,no_position_independent>)

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'
include(cmake/extra_flags.cmake)

zephyr_cc_option(-fno-asynchronous-unwind-tables)

if(CONFIG_USERSPACE)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,no_global_merge>)
endif()

if(CONFIG_THREAD_LOCAL_STORAGE)
# Only support local exec TLS model at this point.
zephyr_cc_option(-ftls-model=local-exec)
endif()

if(CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT)
  if(CONFIG_OMIT_FRAME_POINTER)
    zephyr_cc_option(-fomit-frame-pointer)
  else()
    zephyr_cc_option(-fno-omit-frame-pointer)
  endif()
endif()

separate_arguments(COMPILER_OPT_AS_LIST UNIX_COMMAND ${CONFIG_COMPILER_OPT})
zephyr_compile_options(${COMPILER_OPT_AS_LIST})

# TODO: Include arch compiler options at this point.

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang" AND
   NOT CMAKE_C_COMPILER_ID STREQUAL "IntelLLVM" AND
   NOT CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
  # GCC assumed
  zephyr_cc_option(-fno-reorder-functions)

  # GCC 11 and above may generate a warning when dereferencing a constant
  # address pointer whose address is below the value specified by the
  # `min-pagesize` parameter (defaults to 0x1000). The `min-pagesize` parameter
  # is set to 0 such that GCC never generates any warnings for the constant
  # address pointers. For more details, refer to the GCC PR99578.
  zephyr_cc_option(--param=min-pagesize=0)

  if(NOT ${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "xcc")
    zephyr_cc_option(-fno-defer-pop)
  endif()
else()
  # Clang produces false positive vla warnings
  zephyr_cc_option(-Wno-vla)
endif()

zephyr_cc_option_ifdef(CONFIG_STACK_USAGE            -fstack-usage)

# If the compiler supports it, strip the ${ZEPHYR_BASE} prefix from the
# __FILE__ macro used in __ASSERT*, in the
# .noinit."/home/joe/zephyr/fu/bar.c" section names and in any
# application code. This saves some memory, stops leaking user locations
# in binaries, makes failure logs more deterministic and most
# importantly makes builds more deterministic
if(CONFIG_BUILD_OUTPUT_STRIP_PATHS)
  # If several match then the last one wins. This matters for instances
  # like tests/ and samples/: they're inside all of them! Then let's
  # strip as little as possible.
  zephyr_cc_option(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=CMAKE_SOURCE_DIR)
  zephyr_cc_option(-fmacro-prefix-map=${ZEPHYR_BASE}=ZEPHYR_BASE)
  if(WEST_TOPDIR)
    zephyr_cc_option(-fmacro-prefix-map=${WEST_TOPDIR}=WEST_TOPDIR)
  endif()
endif()

# TODO: Archiver arguments
# ar_option(D)

# Declare MPU userspace dependencies before the linker scripts to make
# sure the order of dependencies are met
if(CONFIG_USERSPACE)
  add_custom_target(app_smem)
  set(APP_SMEM_ALIGNED_DEP app_smem_aligned_linker)
  set(APP_SMEM_UNALIGNED_DEP app_smem_unaligned_linker)
endif()

if(CONFIG_USERSPACE)
  set(KOBJECT_LINKER_DEP kobject_linker)
endif()

if(DEFINED BUILD_VERSION)
  set(build_version_argument "-DBUILD_VERSION=${BUILD_VERSION}")
elseif(NOT ZEPHYR_GIT_INDEX)
  if(EXISTS ${ZEPHYR_BASE}/.git/index)
    set(ZEPHYR_GIT_INDEX ${ZEPHYR_BASE}/.git/index CACHE PATH
      "Path to Zephyr git repository index file")
  elseif(EXISTS ${ZEPHYR_BASE}/.git)
    # Likely a git-submodule. Let's ask git where the real database is located.
    find_package(Git QUIET)
    if(GIT_FOUND)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --absolute-git-dir
        WORKING_DIRECTORY ${ZEPHYR_BASE}
        OUTPUT_VARIABLE zephyr_git_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
        ERROR_VARIABLE stderr
        RESULT_VARIABLE return_code)
      if(return_code)
        message(WARNING "BUILD_VERSION: git rev-parse failed: ${stderr}")
      else()
        if(NOT "${stderr}" STREQUAL "")
          message(WARNING "BUILD_VERSION: git rev-parse warned: ${stderr}")
        endif()
        set(ZEPHYR_GIT_INDEX ${zephyr_git_dir}/index CACHE PATH
          "Path to Zephyr git repository index file")
      endif()
    else()
      message(WARNING "Could not find git installation, "
        "please specify '-DBUILD_VERSION=<version>'")
    endif()
  else()
    message(WARNING "ZEPHYR_BASE=${ZEPHYR_BASE} doesn't appear to be a git "
      "repository, please specify '-DBUILD_VERSION=<version>'")
  endif()
endif()

if(ZEPHYR_GIT_INDEX)
  set(git_dependency ${ZEPHYR_GIT_INDEX})
endif()

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/include/generated/zephyr/version.h
  COMMAND ${CMAKE_COMMAND} -DZEPHYR_BASE=${ZEPHYR_BASE}
    -DOUT_FILE=${PROJECT_BINARY_DIR}/include/generated/zephyr/version.h
    -DVERSION_TYPE=KERNEL
    -DVERSION_FILE=${ZEPHYR_BASE}/VERSION
    -DKERNEL_VERSION_CUSTOMIZATION="$<TARGET_PROPERTY:version_h,KERNEL_VERSION_CUSTOMIZATION>"
    ${build_version_argument}
    -P ${ZEPHYR_BASE}/cmake/gen_version_h.cmake
  DEPENDS ${ZEPHYR_BASE}/VERSION ${git_dependency}
  COMMAND_EXPAND_LISTS
)
add_custom_target(version_h DEPENDS ${PROJECT_BINARY_DIR}/include/generated/zephyr/version.h)
zephyr_get(KERNEL_VERSION_CUSTOMIZATION SYSBUILD LOCAL)
set_property(TARGET version_h PROPERTY KERNEL_VERSION_CUSTOMIZATION ${KERNEL_VERSION_CUSTOMIZATION})

if(EXISTS ${APPLICATION_SOURCE_DIR}/VERSION)
  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/include/generated/zephyr/app_version.h
    COMMAND ${CMAKE_COMMAND} -DZEPHYR_BASE=${ZEPHYR_BASE}
      -DOUT_FILE=${PROJECT_BINARY_DIR}/include/generated/zephyr/app_version.h
      -DVERSION_TYPE=APP
      -DVERSION_FILE=${APPLICATION_SOURCE_DIR}/VERSION
      -DAPP_VERSION_CUSTOMIZATION="$<TARGET_PROPERTY:app_version_h,APP_VERSION_CUSTOMIZATION>"
      ${build_version_argument}
      -P ${ZEPHYR_BASE}/cmake/gen_version_h.cmake
    DEPENDS ${APPLICATION_SOURCE_DIR}/VERSION ${git_dependency}
    COMMAND_EXPAND_LISTS
  )
  add_custom_target(
    app_version_h
    DEPENDS ${PROJECT_BINARY_DIR}/include/generated/zephyr/app_version.h)
  add_dependencies(zephyr_interface app_version_h)
  zephyr_get(APP_VERSION_CUSTOMIZATION SYSBUILD LOCAL)
  set_property(TARGET app_version_h PROPERTY APP_VERSION_CUSTOMIZATION ${APP_VERSION_CUSTOMIZATION})
endif()

# Unfortunately, the order in which CMakeLists.txt code is processed
# matters so we need to be careful about how we order the processing
# of subdirectories. One example is "Compiler flags added late in the
# build are not exported to external build systems #5605"; when we
# integrate with an external build system we read out all compiler
# flags when the external project is created. So an external project
# defined in subsys or ext will not get global flags added by drivers/
# or tests/ as the subdirectories are ordered now.
#
# Another example of when the order matters is the reading and writing
# of global properties such as ZEPHYR_LIBS or
# GENERATED_KERNEL_OBJECT_FILES.
#
# Arch is placed early because it defines important compiler flags
# that must be exported to external build systems defined in
# e.g. subsys/.
add_subdirectory(arch)
add_subdirectory(lib)
# We use include instead of add_subdirectory to avoid creating a new directory scope.
# This is because source file properties are directory scoped, including the GENERATED
# property which is set implicitly for custom command outputs
include(misc/generated/CMakeLists.txt)

add_subdirectory(soc)
add_subdirectory(boards)
add_subdirectory(subsys)
add_subdirectory(drivers)

# Include zephyr modules generated CMake file.
foreach(module_name ${ZEPHYR_MODULE_NAMES})
  # Note the second, binary_dir parameter requires the added
  # subdirectory to have its own, local cmake target(s). If not then
  # this binary_dir is created but stays empty. Object files land in
  # the main binary dir instead.
  # https://cmake.org/pipermail/cmake/2019-June/069547.html
  zephyr_string(SANITIZE TOUPPER MODULE_NAME_UPPER ${module_name})
  if(NOT ${ZEPHYR_${MODULE_NAME_UPPER}_CMAKE_DIR} STREQUAL "")
    set(ZEPHYR_CURRENT_MODULE_NAME ${ZEPHYR_${MODULE_NAME_UPPER}_MODULE_NAME})
    set(ZEPHYR_CURRENT_MODULE_DIR ${ZEPHYR_${MODULE_NAME_UPPER}_MODULE_DIR})
    set(ZEPHYR_CURRENT_CMAKE_DIR ${ZEPHYR_${MODULE_NAME_UPPER}_CMAKE_DIR})
    add_subdirectory(${ZEPHYR_CURRENT_CMAKE_DIR} ${CMAKE_BINARY_DIR}/modules/${module_name})
  endif()
endforeach()
# Done processing modules, clear module variables
set(ZEPHYR_CURRENT_MODULE_NAME)
set(ZEPHYR_CURRENT_MODULE_DIR)
set(ZEPHYR_CURRENT_CMAKE_DIR)

get_property(LIBC_LINK_LIBRARIES TARGET zephyr_interface PROPERTY LIBC_LINK_LIBRARIES)
zephyr_link_libraries(${LIBC_LINK_LIBRARIES})

set(syscall_list_h   ${CMAKE_CURRENT_BINARY_DIR}/include/generated/zephyr/syscall_list.h)
set(edk_syscall_list_h   ${CMAKE_CURRENT_BINARY_DIR}/edk/include/generated/zephyr/syscall_list.h)
set(syscalls_json    ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/syscalls.json)
set(struct_tags_json ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/struct_tags.json)

# The syscalls subdirs txt file is constructed by python containing a list of folders to use for
# dependency handling, including empty folders.
# Windows:  The list is used to specify DIRECTORY list with CMAKE_CONFIGURE_DEPENDS attribute.
# Other OS: The list will update whenever a file is added/removed/modified and ensure a re-build.
set(syscalls_subdirs_txt ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/syscalls_subdirs.txt)

# As syscalls_subdirs_txt is updated whenever a file is modified, this file can not be used for
# monitoring of added / removed folders. A trigger file is thus used for correct dependency
# handling. The trigger file will update when a folder is added / removed.
set(syscalls_subdirs_trigger ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/syscalls_subdirs.trigger)

if(NOT (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows))
  set(syscalls_links --create-links ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/syscalls_links)
endif()

# When running CMake it must be ensured that all dependencies are correctly acquired.
execute_process(
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/subfolder_list.py
  --directory        ${ZEPHYR_BASE}/include      # Walk this directory
  --out-file         ${syscalls_subdirs_txt}     # Write file with discovered folder
  --trigger-file     ${syscalls_subdirs_trigger} # Trigger file that is used for json generation
  ${syscalls_links}                              # If defined, create symlinks for dependencies
)
file(STRINGS ${syscalls_subdirs_txt} PARSE_SYSCALLS_PATHS_DEPENDS ENCODING UTF-8)

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
  # On windows only adding/removing files or folders will be reflected in depends.
  # Hence adding a file requires CMake to re-run to add this file to the file list.
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${PARSE_SYSCALLS_PATHS_DEPENDS})

  # Also On Windows each header file must be monitored as file modifications are not reflected
  # on directory level.
  file(GLOB_RECURSE PARSE_SYSCALLS_HEADER_DEPENDS ${ZEPHYR_BASE}/include/*.h)
else()
  # The syscall parsing depends on the folders in order to detect add/removed/modified files.
  # When a folder is removed, CMake will try to find a target that creates that dependency.
  # This command sets up the target for CMake to find.
  # Without this code, CMake will fail with the following error:
  #   <folder> needed by '<target>', missing and no known rule to make it
  # when a folder is removed.
  add_custom_command(OUTPUT ${PARSE_SYSCALLS_PATHS_DEPENDS}
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Preparing syscall dependency handling"
  )

  add_custom_command(
    OUTPUT
    ${syscalls_subdirs_trigger}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/subfolder_list.py
    --directory        ${ZEPHYR_BASE}/include      # Walk this directory
    --out-file         ${syscalls_subdirs_txt}     # Write file with discovered folder
    --trigger-file     ${syscalls_subdirs_trigger} # Trigger file that is used for json generation
    ${syscalls_links}                              # If defined, create symlinks for dependencies
    DEPENDS ${PARSE_SYSCALLS_PATHS_DEPENDS}
  )

  # Ensure subdir file always exists when specifying CMake dependency.
  if(NOT EXISTS ${syscalls_subdirs_txt})
    file(WRITE ${syscalls_subdirs_txt} "")
  endif()

  # On other OS'es, modifying a file is reflected on the folder timestamp and hence detected
  # when using depend on directory level.
  # Thus CMake only needs to re-run when sub-directories are added / removed, which is indicated
  # using a trigger file.
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${syscalls_subdirs_txt})
endif()

# syscall declarations are searched for in the SYSCALL_INCLUDE_DIRS
if(CONFIG_APPLICATION_DEFINED_SYSCALL)
  list(APPEND SYSCALL_INCLUDE_DIRS ${APPLICATION_SOURCE_DIR})
endif()

if(CONFIG_ZTEST)
  list(APPEND SYSCALL_INCLUDE_DIRS ${ZEPHYR_BASE}/subsys/testsuite/ztest/include)

  if(CONFIG_NO_OPTIMIZATIONS AND CONFIG_ZTEST_WARN_NO_OPTIMIZATIONS)
    message(WARNING "Running tests with CONFIG_NO_OPTIMIZATIONS is generally "
    "not supported and known to break in many cases due to stack overflow or "
    "other problems. Please do not file issues about it unless the test is "
    "specifically tuned to run in this configuration. To disable this warning "
    "set CONFIG_ZTEST_WARN_NO_OPTIMIZATIONS=n.")
  endif()

endif()

get_property(
  syscalls_include_list
  TARGET syscalls_interface
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES
)
list(APPEND SYSCALL_INCLUDE_DIRS ${syscalls_include_list})

foreach(d ${SYSCALL_INCLUDE_DIRS})
  list(APPEND parse_syscalls_include_args
    --include ${d}
    )
endforeach()

add_custom_command(
  OUTPUT
  ${syscalls_json}
  ${struct_tags_json}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/parse_syscalls.py
  --scan             ${ZEPHYR_BASE}/include         # Read files from this dir
  --scan             ${ZEPHYR_BASE}/drivers         # For net sockets
  --scan             ${ZEPHYR_BASE}/subsys/net      # More net sockets
  ${parse_syscalls_include_args}                    # Read files from these dirs also
  --json-file        ${syscalls_json}               # Write this file
  --tag-struct-file  ${struct_tags_json}            # Write subsystem list to this file
  --file-list        ${syscalls_file_list_output}
  $<$<BOOL:${CONFIG_EMIT_ALL_SYSCALLS}>:--emit-all-syscalls>
  DEPENDS ${syscalls_subdirs_trigger} ${PARSE_SYSCALLS_HEADER_DEPENDS}
          ${syscalls_file_list_output} ${syscalls_interface}
  )

# Make sure Picolibc is built before the rest of the system; there's no explicit
# reference to any of the files as they're all picked up by various compiler
# settings
if(CONFIG_PICOLIBC_USE_MODULE)
  set(picolibc_dependency PicolibcBuild)
endif()

add_custom_target(${SYSCALL_LIST_H_TARGET} DEPENDS ${syscall_list_h} ${picolibc_dependency})

set_property(TARGET ${SYSCALL_LIST_H_TARGET}
             APPEND PROPERTY
             ADDITIONAL_CLEAN_FILES
             ${CMAKE_CURRENT_BINARY_DIR}/include/generated/zephyr/syscalls
)

add_custom_target(${PARSE_SYSCALLS_TARGET}
  DEPENDS
  ${syscalls_json}
  ${struct_tags_json}
  )

# 64-bit systems do not require special handling of 64-bit system call
# parameters or return values, indicate this to the system call boilerplate
# generation script.
if(CONFIG_64BIT)
  set(SYSCALL_LONG_REGISTERS_ARG --long-registers)
endif()

if(CONFIG_TIMEOUT_64BIT)
  set(SYSCALL_SPLIT_TIMEOUT_ARG --split-type k_timeout_t --split-type k_ticks_t)
endif()

# percepio/TraceRecorder/kernelports/Zephyr/scripts/tz_parse_syscalls.py hardcodes the path
# to the `syscall_list.h`, make a copy of the generated file so that percepio is able to build
if(CONFIG_LEGACY_GENERATED_INCLUDE_PATH)
  set(LEGACY_SYSCALL_LIST_H_ARGS
    ${CMAKE_COMMAND} -E copy
    ${syscall_list_h}
    ${CMAKE_CURRENT_BINARY_DIR}/include/generated/syscall_list.h)
endif()

add_custom_command(
  OUTPUT
  include/generated/zephyr/syscall_dispatch.c
  include/generated/zephyr/syscall_exports_llext.c
  syscall_weakdefs_llext.c
  ${syscall_list_h}
  # Also, some files are written to include/generated/zephyr/syscalls/
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/gen_syscalls.py
  --json-file        ${syscalls_json}                     # Read this file
  --base-output      include/generated/zephyr/syscalls    # Write to this dir
  --syscall-dispatch include/generated/zephyr/syscall_dispatch.c # Write this file
  --syscall-exports-llext  include/generated/zephyr/syscall_exports_llext.c
  --syscall-weakdefs-llext syscall_weakdefs_llext.c # compiled in CMake library 'syscall_weakdefs'
  --syscall-list     ${syscall_list_h}
  $<$<BOOL:${CONFIG_USERSPACE}>:--gen-mrsh-files>
  ${SYSCALL_LONG_REGISTERS_ARG}
  ${SYSCALL_SPLIT_TIMEOUT_ARG}
  COMMAND
  ${LEGACY_SYSCALL_LIST_H_ARGS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${PARSE_SYSCALLS_TARGET}
  )

# This is passed into all calls to the gen_kobject_list.py script.
set(gen_kobject_list_include_args --include-subsystem-list ${struct_tags_json})

set(DRV_VALIDATION ${PROJECT_BINARY_DIR}/include/generated/zephyr/driver-validation.h)
add_custom_command(
  OUTPUT ${DRV_VALIDATION}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/gen_kobject_list.py
  --validation-output ${DRV_VALIDATION}
  ${gen_kobject_list_include_args}
  $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
  DEPENDS
  ${ZEPHYR_BASE}/scripts/build/gen_kobject_list.py
  ${PARSE_SYSCALLS_TARGET}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
add_custom_target(${DRIVER_VALIDATION_H_TARGET} DEPENDS ${DRV_VALIDATION})

include(${ZEPHYR_BASE}/cmake/kobj.cmake)
gen_kobj(KOBJ_INCLUDE_PATH)

# Generate sections for kernel device subsystems
set(
  DEVICE_API_LD_SECTIONS
  ${CMAKE_CURRENT_BINARY_DIR}/include/generated/device-api-sections.ld
  )

set(DEVICE_API_LINKER_SECTIONS_CMAKE
    ${CMAKE_CURRENT_BINARY_DIR}/include/generated/device-api-sections.cmake
)

add_custom_command(
  OUTPUT ${DEVICE_API_LD_SECTIONS} ${DEVICE_API_LINKER_SECTIONS_CMAKE}
  COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_iter_sections.py
    --alignment ${CONFIG_LINKER_ITERABLE_SUBALIGN}
    --input ${struct_tags_json}
    --tag __subsystem
    --ld-output ${DEVICE_API_LD_SECTIONS}
    --cmake-output ${DEVICE_API_LINKER_SECTIONS_CMAKE}
  DEPENDS
    ${ZEPHYR_BASE}/scripts/build/gen_iter_sections.py
    ${struct_tags_json}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

add_custom_target(${DEVICE_API_LD_TARGET}
                  DEPENDS ${DEVICE_API_LD_SECTIONS}
                          ${DEVICE_API_LINKER_SECTIONS_CMAKE}
)

zephyr_linker_include_generated(CMAKE ${DEVICE_API_LINKER_SECTIONS_CMAKE})

# Add a pseudo-target that is up-to-date when all generated headers
# are up-to-date.

add_custom_target(zephyr_generated_headers)
add_dependencies(zephyr_generated_headers
  offsets_h version_h
  )

# Generate offsets.c.obj from offsets.c
# Generate offsets.h     from offsets.c.obj

set(OFFSETS_LIB offsets)

set(OFFSETS_C_PATH ${ARCH_DIR}/${ARCH}/core/offsets/offsets.c)
set(OFFSETS_H_PATH ${PROJECT_BINARY_DIR}/include/generated/zephyr/offsets.h)

add_library(          ${OFFSETS_LIB} OBJECT ${OFFSETS_C_PATH})
target_include_directories(${OFFSETS_LIB} PRIVATE
  kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )

# Make sure that LTO will never be enabled when compiling offsets.c
set_source_files_properties(${OFFSETS_C_PATH} PROPERTIES COMPILE_OPTIONS $<TARGET_PROPERTY:compiler,prohibit_lto>)

target_link_libraries(${OFFSETS_LIB} zephyr_interface)
add_dependencies(zephyr_interface
  ${SYSCALL_LIST_H_TARGET}
  ${DRIVER_VALIDATION_H_TARGET}
  ${KOBJ_TYPES_H_TARGET}
  ${DEVICE_API_LD_TARGET}
  )

add_custom_command(
  OUTPUT ${OFFSETS_H_PATH}
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/gen_offset_header.py
  -i $<TARGET_OBJECTS:${OFFSETS_LIB}>
  -o ${OFFSETS_H_PATH}
  DEPENDS
  ${OFFSETS_LIB}
  $<TARGET_OBJECTS:${OFFSETS_LIB}>
)
add_custom_target(${OFFSETS_H_TARGET} DEPENDS ${OFFSETS_H_PATH})

zephyr_get_include_directories_for_lang(C ZEPHYR_INCLUDES)

add_subdirectory(kernel)

get_property(
  syscalls_file_list
  TARGET syscalls_interface
  PROPERTY INTERFACE_SOURCES
)
file(CONFIGURE OUTPUT ${syscalls_file_list_output}
     CONTENT "@syscalls_file_list@" @ONLY)

# Read list content
get_property(ZEPHYR_LIBS_PROPERTY GLOBAL PROPERTY ZEPHYR_LIBS)

foreach(zephyr_lib ${ZEPHYR_LIBS_PROPERTY})
  get_property(lib_type     TARGET ${zephyr_lib} PROPERTY TYPE)
  # To prevent CMake failure when a driver is enabled, for example: REGULATOR=y
  # we disable any Zephyr libraries without sources and adds the `empty_file.c`.
  if(${lib_type} STREQUAL STATIC_LIBRARY
     AND NOT ${zephyr_lib} STREQUAL app
  )
    get_property(source_list  TARGET ${zephyr_lib} PROPERTY SOURCES)
    get_property(lib_imported TARGET ${zephyr_lib} PROPERTY IMPORTED)
    if(NOT source_list
       AND NOT ${lib_imported}
    )
      get_property(allow_empty TARGET ${zephyr_lib} PROPERTY ALLOW_EMPTY)
      if(NOT "${allow_empty}")
        message(WARNING
          "No SOURCES given to Zephyr library: ${zephyr_lib}\nExcluding target from build."
        )
      endif()
      target_sources(${zephyr_lib} PRIVATE ${ZEPHYR_BASE}/misc/empty_file.c)
      set_property(TARGET ${zephyr_lib} PROPERTY EXCLUDE_FROM_ALL TRUE)
      list(REMOVE_ITEM ZEPHYR_LIBS_PROPERTY ${zephyr_lib})
      continue()
    endif()
  endif()

  # TODO: Could this become an INTERFACE property of zephyr_interface?
  add_dependencies(${zephyr_lib} zephyr_generated_headers)
endforeach()

if(CONFIG_KERNEL_WHOLE_ARCHIVE)
  set(WHOLE_ARCHIVE_LIBS ${ZEPHYR_LIBS_PROPERTY} kernel)
else()
  set(WHOLE_ARCHIVE_LIBS ${ZEPHYR_LIBS_PROPERTY})
  set(NO_WHOLE_ARCHIVE_LIBS kernel)
endif()

if(CONFIG_LLEXT)
  # LLEXT exports symbols for all syscalls, including unimplemented ones.
  # Weak definitions for these must be added at the end of the link order
  # to avoid shadowing actual implementations.
  add_library(syscall_weakdefs syscall_weakdefs_llext.c)
  add_dependencies(syscall_weakdefs zephyr_generated_headers)
  target_link_libraries(syscall_weakdefs zephyr_interface)
  list(APPEND NO_WHOLE_ARCHIVE_LIBS syscall_weakdefs)
endif()

get_property(OUTPUT_FORMAT        GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)

if (CONFIG_CODE_DATA_RELOCATION)
  set(CODE_RELOCATION_DEP code_relocation_source_lib)
endif() # CONFIG_CODE_DATA_RELOCATION

# Give the linker script targets all of the include directories so
# that cmake can successfully find the linker scripts' header
# dependencies.
zephyr_get_include_directories_for_lang(C
  ZEPHYR_INCLUDE_DIRS
  STRIP_PREFIX # Don't use a -I prefix
  )

if(CONFIG_DEVICE_DEPS)
  if(CONFIG_DEVICE_DEPS_DYNAMIC)
    set(dynamic_deps --dynamic-deps)
  endif()

  if(CONFIG_PM_DEVICE_POWER_DOMAIN_DYNAMIC)
    set(number_of_dynamic_devices ${CONFIG_PM_DEVICE_POWER_DOMAIN_DYNAMIC_NUM})
  else()
    set(number_of_dynamic_devices 0)
  endif()

  # device_deps.c is generated from ${ZEPHYR_LINK_STAGE_EXECUTABLE} by
  # gen_device_deps.py
  add_custom_command(
    OUTPUT device_deps.c
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_device_deps.py
    --output-source device_deps.c
    --output-graphviz dev_graph.dot
    ${dynamic_deps}
    --num-dynamic-devices ${number_of_dynamic_devices}
    --kernel $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    --zephyr-base ${ZEPHYR_BASE}
    --start-symbol "$<TARGET_PROPERTY:linker,devices_start_symbol>"
    VERBATIM
    DEPENDS ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    )
  set_property(GLOBAL APPEND PROPERTY GENERATED_APP_SOURCE_FILES device_deps.c)

  # gen_device_deps runs on `__device_deps_pass1` so pass this info to the linker script generator
  list(APPEND LINKER_PASS_${ZEPHYR_CURRENT_LINKER_PASS}_DEFINE "LINKER_DEVICE_DEPS_PASS1")
endif()

if(CONFIG_CODE_DATA_RELOCATION)
  # @Intent: Linker script to relocate .text, data and .bss sections
  toolchain_ld_relocation()
endif()

if(CONFIG_USERSPACE)
  zephyr_get_compile_options_for_lang_as_string(C compiler_flags_priv)
  string(REPLACE "$<TARGET_PROPERTY:compiler,coverage>" ""
         NO_COVERAGE_FLAGS "${compiler_flags_priv}"
  )

  set(GEN_KOBJ_LIST ${ZEPHYR_BASE}/scripts/build/gen_kobject_list.py)
  set(PROCESS_GPERF ${ZEPHYR_BASE}/scripts/build/process_gperf.py)
endif()

# @Intent: Obtain compiler specific flag for specifying the c standard
zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,cstd>${CSTD}>
)
set(CMAKE_C_COMPILE_FEATURES ${CMAKE_C_COMPILE_FEATURES} PARENT_SCOPE)

# @Intent: Configure linker scripts, i.e. generate linker scripts with variables substituted
toolchain_ld_configure_files()

get_property(TOPT GLOBAL PROPERTY TOPT)
get_property(COMPILER_TOPT TARGET compiler PROPERTY linker_script)
set_ifndef(  TOPT "${COMPILER_TOPT}")
set_ifndef(  TOPT -Wl,-T) # Use this if the compiler driver doesn't set a value

if(CONFIG_HAVE_CUSTOM_LINKER_SCRIPT)
  string(CONFIGURE ${APPLICATION_SOURCE_DIR}/${CONFIG_CUSTOM_LINKER_SCRIPT} LINKER_SCRIPT)
  if(NOT EXISTS ${LINKER_SCRIPT})
    string(CONFIGURE ${CONFIG_CUSTOM_LINKER_SCRIPT} LINKER_SCRIPT)
    assert_exists(LINKER_SCRIPT)
  endif()
elseif(DEFINED BOARD_LINKER_SCRIPT)
  set(LINKER_SCRIPT ${BOARD_LINKER_SCRIPT})
elseif(DEFINED SOC_LINKER_SCRIPT)
  set(LINKER_SCRIPT ${SOC_LINKER_SCRIPT})
endif()

if(NOT EXISTS ${LINKER_SCRIPT})
  message(FATAL_ERROR "Could not find linker script: '${LINKER_SCRIPT}'. Corrupted configuration?")
endif()

if(CONFIG_USERSPACE)
  if(CONFIG_CMAKE_LINKER_GENERATOR)
    set(APP_SMEM_LD_EXT "cmake")
  else()
    set(APP_SMEM_LD_EXT "ld")
  endif()

  set(APP_SMEM_ALIGNED_LD "${PROJECT_BINARY_DIR}/include/generated/app_smem_aligned.${APP_SMEM_LD_EXT}")
  set(APP_SMEM_UNALIGNED_LD "${PROJECT_BINARY_DIR}/include/generated/app_smem_unaligned.${APP_SMEM_LD_EXT}")

  if(CONFIG_LINKER_USE_PINNED_SECTION)
    set(APP_SMEM_PINNED_ALIGNED_LD
        "${PROJECT_BINARY_DIR}/include/generated/app_smem_pinned_aligned.${APP_SMEM_LD_EXT}")
    set(APP_SMEM_PINNED_UNALIGNED_LD
        "${PROJECT_BINARY_DIR}/include/generated/app_smem_pinned_unaligned.${APP_SMEM_LD_EXT}")

    if(NOT CONFIG_LINKER_GENERIC_SECTIONS_PRESENT_AT_BOOT)
      # The libc partition may hold symbols that are required during boot process,
      # for example, stack guard (if enabled). So the libc partition must be pinned
      # if not sections are in physical memory at boot, as the paging mechanism is
      # only initialized post-kernel.
      set_property(TARGET app_smem APPEND PROPERTY pinned_partitions "z_libc_partition")
    endif()

    get_property(APP_SMEM_PINNED_PARTITION_LIST TARGET app_smem PROPERTY pinned_partitions)
    if(APP_SMEM_PINNED_PARTITION_LIST)
      list(JOIN APP_SMEM_PINNED_PARTITION_LIST "," APP_SMEM_PINNED_PARTITION_LIST_ARG_CSL)
      set(APP_SMEM_PINNED_PARTITION_LIST_ARG "--pinpartitions=${APP_SMEM_PINNED_PARTITION_LIST_ARG_CSL}")
    endif()
  endif()

  set(OBJ_FILE_DIR "${PROJECT_BINARY_DIR}/../")

  if(CONFIG_NEWLIB_LIBC)
    set(LIBC_PART -l libc.a z_libc_partition -l libm.a z_libc_partition)
  endif()
  if(CONFIG_NEWLIB_LIBC_NANO)
    set(LIBC_PART -l libc_nano.a z_libc_partition -l libm_nano.a z_libc_partition)
  endif()
  if(CONFIG_PICOLIBC)
    set(LIBC_PART -l libc.a z_libc_partition)
  endif()

  add_custom_command(
    OUTPUT ${APP_SMEM_UNALIGNED_LD} ${APP_SMEM_PINNED_UNALIGNED_LD}
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_app_partitions.py
    -f ${CMAKE_BINARY_DIR}/compile_commands.json
    -o ${APP_SMEM_UNALIGNED_LD}
    $<$<BOOL:${APP_SMEM_PINNED_UNALIGNED_LD}>:--pinoutput=${APP_SMEM_PINNED_UNALIGNED_LD}>
    ${APP_SMEM_PINNED_PARTITION_LIST_ARG}
    ${LIBC_PART}
    $<TARGET_PROPERTY:zephyr_property_target,COMPILE_OPTIONS>
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    kernel
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${ZEPHYR_LIBS_PROPERTY}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/
    COMMAND_EXPAND_LISTS
    COMMENT "Generating app_smem_unaligned linker section"
    )

  add_custom_target(
    ${APP_SMEM_ALIGNED_DEP}
    DEPENDS
    ${APP_SMEM_ALIGNED_LD}
    ${APP_SMEM_PINNED_ALIGNED_LD}
    )

  add_custom_target(
    ${APP_SMEM_UNALIGNED_DEP}
    DEPENDS
    ${APP_SMEM_UNALIGNED_LD}
    ${APP_SMEM_PINNED_UNALIGNED_LD}
    )

  set(APP_SMEM_UNALIGNED_LIB app_smem_unaligned_output_obj_renamed_lib)
  list(APPEND LINKER_PASS_${ZEPHYR_CURRENT_LINKER_PASS}_DEFINE "LINKER_APP_SMEM_UNALIGNED")

  foreach(dep ${APP_SMEM_UNALIGNED_LD} ${APP_SMEM_PINNED_UNALIGNED_LD})
    zephyr_linker_include_generated(CMAKE ${dep} PASS LINKER_APP_SMEM_UNALIGNED)
  endforeach()

  add_custom_command(
    OUTPUT ${APP_SMEM_ALIGNED_LD} ${APP_SMEM_PINNED_ALIGNED_LD}
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_app_partitions.py
    -e $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    -o ${APP_SMEM_ALIGNED_LD}
    $<$<BOOL:${APP_SMEM_PINNED_ALIGNED_LD}>:--pinoutput=${APP_SMEM_PINNED_ALIGNED_LD}>
    ${APP_SMEM_PINNED_PARTITION_LIST_ARG}
    ${LIBC_PART}
    $<TARGET_PROPERTY:zephyr_property_target,COMPILE_OPTIONS>
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    kernel
    ${ZEPHYR_LIBS_PROPERTY}
    ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/
    COMMAND_EXPAND_LISTS
    COMMENT "Generating app_smem_aligned linker section"
    )
    foreach(dep ${APP_SMEM_ALIGNED_LD} ${APP_SMEM_PINNED_ALIGNED_LD})
      zephyr_linker_include_generated(CMAKE ${dep} PASS NOT LINKER_APP_SMEM_UNALIGNED)
    endforeach()
endif()

if(CONFIG_USERSPACE)
  # This CONFIG_USERSPACE block is to create place holders to reserve space
  # for the gperf generated structures for zephyr_prebuilt.elf.
  # These place holders are there so that the placement of kobjects would be
  # the same between linking zephyr_prebuilt.elf and zephyr.elf, as
  # the gperf hash table is hashed on the addresses of kobjects.
  # The placeholders are generated from app_smem_unaligned_prebuilt.elf.

  set(KOBJECT_PREBUILT_HASH_LIST           kobject_prebuilt_hash.gperf)
  set(KOBJECT_PREBUILT_HASH_OUTPUT_SRC_PRE kobject_prebuilt_hash_preprocessed.c)
  set(KOBJECT_PREBUILT_HASH_OUTPUT_SRC     kobject_prebuilt_hash.c)

  add_custom_command(
    OUTPUT ${KOBJECT_PREBUILT_HASH_LIST}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${GEN_KOBJ_LIST}
    --kernel $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    --gperf-output ${KOBJECT_PREBUILT_HASH_LIST}
    ${gen_kobject_list_include_args}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(
    kobj_prebuilt_hash_list
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_PREBUILT_HASH_LIST}
  )

  add_custom_command(
    OUTPUT ${KOBJECT_PREBUILT_HASH_OUTPUT_SRC_PRE}
    COMMAND
    ${GPERF}
    --output-file ${KOBJECT_PREBUILT_HASH_OUTPUT_SRC_PRE}
    --multiple-iterations 10
    ${KOBJECT_PREBUILT_HASH_LIST}
    DEPENDS kobj_prebuilt_hash_list ${KOBJECT_PREBUILT_HASH_LIST}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(
    kobj_prebuilt_hash_output_src_pre
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_PREBUILT_HASH_OUTPUT_SRC_PRE}
  )

  add_custom_command(
    OUTPUT ${KOBJECT_PREBUILT_HASH_OUTPUT_SRC}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${PROCESS_GPERF}
    -i ${KOBJECT_PREBUILT_HASH_OUTPUT_SRC_PRE}
    -o ${KOBJECT_PREBUILT_HASH_OUTPUT_SRC}
    -p "struct k_object"
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS kobj_prebuilt_hash_output_src_pre ${KOBJECT_PREBUILT_HASH_OUTPUT_SRC_PRE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(
    kobj_prebuilt_hash_output_src
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_PREBUILT_HASH_OUTPUT_SRC}
  )

  add_library(
    kobj_prebuilt_hash_output_lib
    OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_PREBUILT_HASH_OUTPUT_SRC}
    )

  set_source_files_properties(${KOBJECT_PREBUILT_HASH_OUTPUT_SRC}
    PROPERTIES COMPILE_FLAGS
    "${NO_COVERAGE_FLAGS} -fno-function-sections -fno-data-sections")

  target_compile_definitions(kobj_prebuilt_hash_output_lib
    PRIVATE $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>
  )

  target_include_directories(kobj_prebuilt_hash_output_lib
    PUBLIC $<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>
  )

  target_include_directories(kobj_prebuilt_hash_output_lib SYSTEM
    PUBLIC $<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
  )

  set(KOBJECT_LINKER_HEADER_DATA "${PROJECT_BINARY_DIR}/include/generated/zephyr/linker-kobject-prebuilt-data.h")

  add_custom_command(
    OUTPUT ${KOBJECT_LINKER_HEADER_DATA}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_kobject_placeholders.py
    --object $<TARGET_OBJECTS:kobj_prebuilt_hash_output_lib>
    --outdir ${PROJECT_BINARY_DIR}/include/generated/zephyr
    --datapct ${CONFIG_KOBJECT_DATA_AREA_RESERVE_EXTRA_PERCENT}
    --rodata ${CONFIG_KOBJECT_RODATA_AREA_EXTRA_BYTES}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    kobj_prebuilt_hash_output_lib $<TARGET_OBJECTS:kobj_prebuilt_hash_output_lib>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

  add_custom_target(
    ${KOBJECT_LINKER_DEP}
    DEPENDS
    ${KOBJECT_LINKER_HEADER_DATA}
  )

  # gen_kobject_placeholders.py generates linker-kobject-prebuild-data.h,
  # linker-kobject-prebuild-priv-stacks.h and linker-kobject-prebuild-rodata.h
  foreach(ext "-data.h" "-priv-stacks.h" "-rodata.h")
    string(REGEX REPLACE "-data.h$" ${ext} file ${KOBJECT_LINKER_HEADER_DATA})
    zephyr_linker_include_generated(HEADER ${file} PASS LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL)
  endforeach()
endif()

if(CONFIG_USERSPACE OR CONFIG_DEVICE_DEPS)
  configure_linker_script(
    ${ZEPHYR_CURRENT_LINKER_CMD}
    "${LINKER_PASS_${ZEPHYR_CURRENT_LINKER_PASS}_DEFINE}"
    ${CODE_RELOCATION_DEP}
    ${APP_SMEM_UNALIGNED_DEP}
    ${APP_SMEM_UNALIGNED_LD}
    ${APP_SMEM_PINNED_UNALIGNED_LD}
    zephyr_generated_headers
    )

  add_custom_target(
    linker_zephyr_pre${ZEPHYR_CURRENT_LINKER_PASS}_script
    DEPENDS
    ${ZEPHYR_CURRENT_LINKER_CMD}
    )

  set_property(TARGET
    linker_zephyr_pre${ZEPHYR_CURRENT_LINKER_PASS}_script
    PROPERTY INCLUDE_DIRECTORIES
    ${ZEPHYR_INCLUDE_DIRS}
    )

  add_executable(${ZEPHYR_LINK_STAGE_EXECUTABLE} misc/empty_file.c)
  toolchain_ld_link_elf(
    TARGET_ELF            ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${ZEPHYR_LINK_STAGE_EXECUTABLE}.map
    LIBRARIES_PRE_SCRIPT  ""
    LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/${ZEPHYR_CURRENT_LINKER_CMD}
    LIBRARIES_POST_SCRIPT ""
    DEPENDENCIES          ${CODE_RELOCATION_DEP}
  )
  target_link_libraries_ifdef(CONFIG_NATIVE_LIBRARY ${ZEPHYR_LINK_STAGE_EXECUTABLE}
                              $<TARGET_PROPERTY:linker,no_position_independent>)
  target_byproducts(TARGET ${ZEPHYR_LINK_STAGE_EXECUTABLE}
                    BYPRODUCTS ${PROJECT_BINARY_DIR}/${ZEPHYR_LINK_STAGE_EXECUTABLE}.map
  )
  set_property(TARGET ${ZEPHYR_LINK_STAGE_EXECUTABLE} PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/${ZEPHYR_CURRENT_LINKER_CMD})
  add_dependencies(${ZEPHYR_LINK_STAGE_EXECUTABLE} linker_zephyr_pre${ZEPHYR_CURRENT_LINKER_PASS}_script ${OFFSETS_LIB})

  math(EXPR ZEPHYR_CURRENT_LINKER_PASS "1 + ${ZEPHYR_CURRENT_LINKER_PASS}")
endif()

set(ZEPHYR_CURRENT_LINKER_CMD  linker_zephyr_pre${ZEPHYR_CURRENT_LINKER_PASS}.cmd)
set(ZEPHYR_LINK_STAGE_EXECUTABLE zephyr_pre${ZEPHYR_CURRENT_LINKER_PASS})
list(APPEND LINKER_PASS_${ZEPHYR_CURRENT_LINKER_PASS}_DEFINE "LINKER_ZEPHYR_PREBUILT")

if(CONFIG_GEN_ISR_TABLES)
  if(CONFIG_GEN_SW_ISR_TABLE)
    list(APPEND GEN_ISR_TABLE_EXTRA_ARG --sw-isr-table)
  endif()

  if(CONFIG_GEN_IRQ_VECTOR_TABLE)
    list(APPEND GEN_ISR_TABLE_EXTRA_ARG --vector-table)
  endif()

  # isr_tables.c is generated from ${ZEPHYR_LINK_STAGE_EXECUTABLE} by
  # gen_isr_tables.py
  add_custom_command(
    OUTPUT isr_tables.c isr_tables_vt.ld isr_tables_swi.ld
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_isr_tables.py
    --output-source isr_tables.c
    --linker-output-files isr_tables_vt.ld isr_tables_swi.ld
    --kernel $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    --intlist-section .intList
    --intlist-section intList
    $<$<BOOL:${CONFIG_BIG_ENDIAN}>:--big-endian>
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--debug>
    ${GEN_ISR_TABLE_EXTRA_ARG}
    DEPENDS ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    COMMAND_EXPAND_LISTS
    )
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES isr_tables.c)
endif()

if(CONFIG_SYMTAB)
  add_custom_command(
    OUTPUT symtab.c
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_symtab.py
    -k $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    -o symtab.c
    DEPENDS ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    COMMAND_EXPAND_LISTS
  )
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES symtab.c)
endif()

if(CONFIG_USERSPACE)
  set(KOBJECT_HASH_LIST               kobject_hash.gperf)
  set(KOBJECT_HASH_OUTPUT_SRC_PRE     kobject_hash_preprocessed.c)
  set(KOBJECT_HASH_OUTPUT_SRC         kobject_hash.c)
  set(KOBJECT_HASH_OUTPUT_OBJ_RENAMED kobject_hash_renamed.o)

  # Essentially what we are doing here is extracting some information
  # out of the nearly finished elf file, generating the source code
  # for a hash table based on that information, and then compiling and
  # linking the hash table back into a now even more nearly finished
  # elf file. More information in gen_kobject_list.py --help.

  # Use the script GEN_KOBJ_LIST to scan the kernel binary's
  # (${ZEPHYR_LINK_STAGE_EXECUTABLE}) DWARF information to produce a table of kernel
  # objects (KOBJECT_HASH_LIST) which we will then pass to gperf
  add_custom_command(
    OUTPUT ${KOBJECT_HASH_LIST}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${GEN_KOBJ_LIST}
    --kernel $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    --gperf-output ${KOBJECT_HASH_LIST}
    ${gen_kobject_list_include_args}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(
    kobj_hash_list
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_HASH_LIST}
  )

  # Use gperf to generate C code (KOBJECT_HASH_OUTPUT_SRC_PRE) which implements a
  # perfect hashtable based on KOBJECT_HASH_LIST
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_HASH_OUTPUT_SRC_PRE}
    COMMAND
    ${GPERF}
    --output-file ${KOBJECT_HASH_OUTPUT_SRC_PRE}
    --multiple-iterations 10
    ${KOBJECT_HASH_LIST}
    DEPENDS kobj_hash_list ${KOBJECT_HASH_LIST}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(
    kobj_hash_output_src_pre
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_HASH_OUTPUT_SRC_PRE}
  )

  # For our purposes the code/data generated by gperf is not optimal.
  #
  # The script PROCESS_GPERF creates a new c file KOBJECT_HASH_OUTPUT_SRC based on
  # KOBJECT_HASH_OUTPUT_SRC_PRE to greatly reduce the amount of code/data generated
  # since we know we are always working with pointer values
  add_custom_command(
    OUTPUT ${KOBJECT_HASH_OUTPUT_SRC}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${PROCESS_GPERF}
    -i ${KOBJECT_HASH_OUTPUT_SRC_PRE}
    -o ${KOBJECT_HASH_OUTPUT_SRC}
    -p "struct k_object"
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS kobj_hash_output_src_pre ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_HASH_OUTPUT_SRC_PRE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(
    kobj_hash_output_src
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_HASH_OUTPUT_SRC}
  )

  # We need precise control of where generated text/data ends up in the final
  # kernel image. Disable function/data sections and use objcopy to move
  # generated data into special section names
  add_library(
    kobj_hash_output_lib
    OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_HASH_OUTPUT_SRC}
    )

  set_source_files_properties(${KOBJECT_HASH_OUTPUT_SRC}
    PROPERTIES COMPILE_FLAGS
    "${NO_COVERAGE_FLAGS} -fno-function-sections -fno-data-sections")

  target_compile_definitions(kobj_hash_output_lib
    PRIVATE $<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>
  )

  target_include_directories(kobj_hash_output_lib
    PUBLIC $<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>
  )

  target_include_directories(kobj_hash_output_lib SYSTEM
    PUBLIC $<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>
  )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_HASH_OUTPUT_OBJ_RENAMED}
    COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
            $<TARGET_PROPERTY:bintools,elfconvert_flag>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_rename>.literal=.kobject_data.literal
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_rename>.data=.kobject_data.data
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_rename>.sdata=.kobject_data.sdata
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_rename>.text=.kobject_data.text
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_rename>.rodata=.kobject_data.rodata
            $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>$<TARGET_OBJECTS:kobj_hash_output_lib>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KOBJECT_HASH_OUTPUT_OBJ_RENAMED}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
    DEPENDS kobj_hash_output_lib $<TARGET_OBJECTS:kobj_hash_output_lib>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND_EXPAND_LISTS
    )
  add_custom_target(
    kobj_hash_output_obj_renamed
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_HASH_OUTPUT_OBJ_RENAMED}
  )

  add_library(kobj_hash_output_obj_renamed_lib STATIC IMPORTED GLOBAL)
  set_property(
    TARGET kobj_hash_output_obj_renamed_lib
    PROPERTY
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${KOBJECT_HASH_OUTPUT_OBJ_RENAMED}
    )
  add_dependencies(
    kobj_hash_output_obj_renamed_lib
    kobj_hash_output_obj_renamed
    )

  set_property(
    GLOBAL APPEND PROPERTY
    GENERATED_KERNEL_OBJECT_FILES kobj_hash_output_obj_renamed_lib
  )
endif()

configure_linker_script(
  ${ZEPHYR_CURRENT_LINKER_CMD}
  "${LINKER_PASS_${ZEPHYR_CURRENT_LINKER_PASS}_DEFINE}"
  ${APP_SMEM_ALIGNED_DEP}
  ${KOBJECT_LINKER_DEP}
  ${CODE_RELOCATION_DEP}
  zephyr_generated_headers
  )

add_custom_target(
  linker_zephyr_prebuilt_script_target
  DEPENDS
  ${ZEPHYR_CURRENT_LINKER_CMD}
  )

set_property(TARGET
  linker_zephyr_prebuilt_script_target
  PROPERTY INCLUDE_DIRECTORIES
  ${ZEPHYR_INCLUDE_DIRS}
  )

# Read global variables into local variables
get_property(GASF GLOBAL PROPERTY GENERATED_APP_SOURCE_FILES)
get_property(GKOF GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES)
get_property(GKSF GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES)

# FIXME: Is there any way to get rid of empty_file.c?
add_executable(       ${ZEPHYR_LINK_STAGE_EXECUTABLE} misc/empty_file.c ${GASF})
toolchain_ld_link_elf(
  TARGET_ELF            ${ZEPHYR_LINK_STAGE_EXECUTABLE}
  OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${ZEPHYR_LINK_STAGE_EXECUTABLE}.map
  LIBRARIES_PRE_SCRIPT  ""
  LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/${ZEPHYR_CURRENT_LINKER_CMD}
  DEPENDENCIES          ${CODE_RELOCATION_DEP}
)
target_link_libraries_ifdef(CONFIG_NATIVE_LIBRARY ${ZEPHYR_LINK_STAGE_EXECUTABLE}
                            $<TARGET_PROPERTY:linker,partial_linking>)
target_byproducts(TARGET ${ZEPHYR_LINK_STAGE_EXECUTABLE}
                  BYPRODUCTS ${PROJECT_BINARY_DIR}/${ZEPHYR_LINK_STAGE_EXECUTABLE}.map
)
set(BYPRODUCT_KERNEL_ELF_NAME "${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}" CACHE FILEPATH "Kernel elf file" FORCE)
set_property(TARGET
  ${ZEPHYR_LINK_STAGE_EXECUTABLE}
  PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/${ZEPHYR_CURRENT_LINKER_CMD}
  )
add_dependencies(
  ${ZEPHYR_LINK_STAGE_EXECUTABLE}
  linker_zephyr_prebuilt_script_target
  ${OFFSETS_LIB}
  )

set(generated_kernel_files ${GKSF} ${GKOF})
if(NOT generated_kernel_files)
  # Use the prebuilt elf as the final elf since we don't have a
  # generation stage.
  set(logical_target_for_zephyr_elf ${ZEPHYR_LINK_STAGE_EXECUTABLE})
else()
  # The final linker pass uses the same source linker script of the
  # previous passes, but this time with a different output
  # file and preprocessed with the define LINKER_ZEPHYR_FINAL.
  configure_linker_script(
    linker.cmd
    "LINKER_ZEPHYR_FINAL"
    ${CODE_RELOCATION_DEP}
    ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    zephyr_generated_headers
    )

  add_custom_target(
    linker_zephyr_final_script_target
    DEPENDS
    linker.cmd
    )
  set_property(TARGET
    linker_zephyr_final_script_target
    PROPERTY INCLUDE_DIRECTORIES
    ${ZEPHYR_INCLUDE_DIRS}
  )

  add_executable(       ${ZEPHYR_FINAL_EXECUTABLE} misc/empty_file.c ${GASF} ${GKSF})
  toolchain_ld_link_elf(
    TARGET_ELF            ${ZEPHYR_FINAL_EXECUTABLE}
    OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${ZEPHYR_FINAL_EXECUTABLE}.map
    LIBRARIES_PRE_SCRIPT  ${GKOF}
    LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/linker.cmd
    LIBRARIES_POST_SCRIPT ""
    DEPENDENCIES          ${CODE_RELOCATION_DEP}
  )
  set_property(TARGET   ${ZEPHYR_FINAL_EXECUTABLE} PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.cmd)
  add_dependencies(     ${ZEPHYR_FINAL_EXECUTABLE} linker_zephyr_final_script_target)

  # Use the pass2 elf as the final elf
  set(logical_target_for_zephyr_elf ${ZEPHYR_FINAL_EXECUTABLE})
endif()

# Export the variable to the application's scope to allow the
# application to know what the name of the final elf target is.
set(logical_target_for_zephyr_elf ${logical_target_for_zephyr_elf} PARENT_SCOPE)

# Override the base name of the last, "logical" .elf output (and last .map) so:
# 1. it doesn't depend on the number of passes above and the
#    post_build_commands below can always find it no matter which is it;
# 2. it can be defined in Kconfig
set_target_properties(${logical_target_for_zephyr_elf} PROPERTIES OUTPUT_NAME ${KERNEL_NAME})

set(post_build_commands "")
set(post_build_byproducts "")

list(APPEND
  post_build_commands
  COMMAND
  ${CMAKE_COMMAND} -E copy ${logical_target_for_zephyr_elf}.map ${KERNEL_MAP_NAME}
)
list(APPEND post_build_byproducts ${KERNEL_MAP_NAME})

# Use ';' as separator to get proper space in resulting command.
set(gap_fill_prop "$<TARGET_PROPERTY:bintools,elfconvert_flag_gapfill>")
set(gap_fill "$<$<BOOL:${gap_fill_prop}>:${gap_fill_prop}${CONFIG_BUILD_GAP_FILL_PATTERN}>")

if(CONFIG_OUTPUT_PRINT_MEMORY_USAGE)
  target_link_libraries(${logical_target_for_zephyr_elf} $<TARGET_PROPERTY:linker,memusage>)

  get_property(memusage_build_command TARGET bintools PROPERTY memusage_command)
  if(memusage_build_command)
    # Note: The use of generator expressions allows downstream extensions to add/change the post build.
    # Unfortunately, the BYPRODUCTS does not allow for generator expression, so question is if we
    # should remove the downstream ability from start.
    # Or fix the output name, by the use of `get_property`
    list(APPEND
      post_build_commands
      COMMAND $<TARGET_PROPERTY:bintools,memusage_command>
              $<TARGET_PROPERTY:bintools,memusage_flag>
              $<TARGET_PROPERTY:bintools,memusage_infile>${KERNEL_ELF_NAME}
      )

    # For now, the byproduct can only be supported upstream on byproducts name,
    # cause byproduct does not support generator expressions
    get_property(memusage_byproducts TARGET bintools PROPERTY memusage_byproducts)
    list(APPEND
      post_build_byproducts
      ${memusage_byproducts}
      )
  endif()
endif()

if(CONFIG_BUILD_OUTPUT_ADJUST_LMA)
  math(EXPR adjustment "${CONFIG_BUILD_OUTPUT_ADJUST_LMA}" OUTPUT_FORMAT DECIMAL)
  set(args_adjustment ${CONFIG_BUILD_OUTPUT_ADJUST_LMA_SECTIONS})
  list(TRANSFORM args_adjustment PREPEND $<TARGET_PROPERTY:bintools,elfconvert_flag_lma_adjust>)
  list(TRANSFORM args_adjustment APPEND +${adjustment})
  list(APPEND
    post_build_commands
    COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
            ${args_adjustment}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KERNEL_ELF_NAME}
    )
endif()

if(NOT CONFIG_CPP_EXCEPTIONS)
  set(eh_frame_section ".eh_frame")
else()
  set(eh_frame_section "")
endif()
set(remove_sections_argument_list "")
foreach(section .comment COMMON ${eh_frame_section})
  list(APPEND remove_sections_argument_list
    $<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>${section})
endforeach()

if(CONFIG_BUILD_OUTPUT_HEX OR BOARD_FLASH_RUNNER STREQUAL openocd)
  get_property(elfconvert_formats TARGET bintools PROPERTY elfconvert_formats)
  if(ihex IN_LIST elfconvert_formats)
    list(APPEND
      post_build_commands
      COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
              $<TARGET_PROPERTY:bintools,elfconvert_flag>
              $<$<BOOL:${CONFIG_BUILD_OUTPUT_HEX_GAP_FILL}>:${gap_fill}>
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>ihex
              ${remove_sections_argument_list}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KERNEL_HEX_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
      )
    list(APPEND
      post_build_byproducts
      ${KERNEL_HEX_NAME}
      )
    set(BYPRODUCT_KERNEL_HEX_NAME "${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}" CACHE FILEPATH "Kernel hex file" FORCE)
  endif()
endif()

if(CONFIG_BUILD_OUTPUT_BIN)
  get_property(elfconvert_formats TARGET bintools PROPERTY elfconvert_formats)
  if(binary IN_LIST elfconvert_formats)
    list(APPEND
      post_build_commands
      COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
              $<TARGET_PROPERTY:bintools,elfconvert_flag>
              ${gap_fill}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>binary
              ${remove_sections_argument_list}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KERNEL_BIN_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
      )
    list(APPEND
      post_build_byproducts
      ${KERNEL_BIN_NAME}
      )
    set(BYPRODUCT_KERNEL_BIN_NAME "${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}" CACHE FILEPATH "Kernel binary file" FORCE)
  endif()
endif()

if(CONFIG_BUILD_OUTPUT_BIN AND CONFIG_BUILD_OUTPUT_UF2)
  if(CONFIG_BUILD_OUTPUT_UF2_USE_FLASH_BASE)
    set(flash_addr "${CONFIG_FLASH_BASE_ADDRESS}")
  else()
    set(flash_addr "${CONFIG_FLASH_LOAD_OFFSET}")
  endif()

  if(CONFIG_BUILD_OUTPUT_UF2_USE_FLASH_OFFSET)
    # Note, the `+ 0` in formula below avoids errors in cases where a Kconfig
    #       variable is undefined and thus expands to nothing.
    math(EXPR flash_addr
        "${flash_addr} + ${CONFIG_FLASH_LOAD_OFFSET} + 0"
         OUTPUT_FORMAT HEXADECIMAL
    )
  endif()

  list(APPEND
    post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/uf2conv.py
            -c
            -f ${CONFIG_BUILD_OUTPUT_UF2_FAMILY_ID}
            -b ${flash_addr}
            -o ${KERNEL_UF2_NAME}
            ${KERNEL_BIN_NAME}
  )
  list(APPEND
    post_build_byproducts
    ${KERNEL_UF2_NAME}
  )
  set(BYPRODUCT_KERNEL_UF2_NAME "${PROJECT_BINARY_DIR}/${KERNEL_UF2_NAME}" CACHE FILEPATH "Kernel uf2 file" FORCE)
endif()

set(KERNEL_META_PATH  ${PROJECT_BINARY_DIR}/${KERNEL_META_NAME} CACHE INTERNAL "")
if(CONFIG_BUILD_OUTPUT_META)
  list(APPEND
    post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/zephyr_module.py
            ${ZEPHYR_MODULES_ARG}
            ${EXTRA_ZEPHYR_MODULES_ARG}
            --meta-out ${KERNEL_META_PATH}
            --zephyr-base=${ZEPHYR_BASE}
            $<$<BOOL:${CONFIG_BUILD_OUTPUT_META_STATE_PROPAGATE}>:--meta-state-propagate>
  )
  list(APPEND
    post_build_byproducts
    ${KERNEL_META_PATH}
  )
else(CONFIG_BUILD_OUTPUT_META)
  # Prevent spdx to use invalid data
  file(REMOVE ${KERNEL_META_PATH})
endif()

# Cleanup intermediate files
if(CONFIG_CLEANUP_INTERMEDIATE_FILES)
  foreach(index RANGE ${ZEPHYR_CURRENT_LINKER_PASS})
    # Those files can be very large in some cases, delete them as we do not need them.
    list(APPEND
      post_build_commands
      COMMAND
      ${CMAKE_COMMAND} -E remove zephyr_pre${index}.elf
      )
  endforeach()
endif()

if(CONFIG_BUILD_OUTPUT_S19)
  get_property(elfconvert_formats TARGET bintools PROPERTY elfconvert_formats)
  if(srec IN_LIST elfconvert_formats)
    # Should we print a warning if case the tools does not support converting to s19 ?
    list(APPEND
      post_build_commands
      COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
              $<TARGET_PROPERTY:bintools,elfconvert_flag>
              $<$<BOOL:${CONFIG_BUILD_OUTPUT_S19_GAP_FILL}>:${gap_fill}>
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>srec
              $<TARGET_PROPERTY:bintools,elfconvert_flag_srec_len>1
              $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KERNEL_S19_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
      )
    list(APPEND
      post_build_byproducts
      ${KERNEL_S19_NAME}
      )
    set(BYPRODUCT_KERNEL_S19_NAME "${PROJECT_BINARY_DIR}/${KERNEL_S19_NAME}" CACHE FILEPATH "Kernel s19 file" FORCE)
  endif()
endif()

if(CONFIG_OUTPUT_DISASSEMBLY)
if(CONFIG_OUTPUT_DISASSEMBLE_ALL)
    set(disassembly_type "$<TARGET_PROPERTY:bintools,disassembly_flag_all>")
  elseif (CONFIG_OUTPUT_DISASSEMBLY_WITH_SOURCE)
    set(disassembly_type "$<TARGET_PROPERTY:bintools,disassembly_flag_inline_source>")
  endif()
  list(APPEND
    post_build_commands
    COMMAND $<TARGET_PROPERTY:bintools,disassembly_command>
            $<TARGET_PROPERTY:bintools,disassembly_flag>
            ${disassembly_type}
            $<TARGET_PROPERTY:bintools,disassembly_flag_infile>${KERNEL_ELF_NAME}
            $<TARGET_PROPERTY:bintools,disassembly_flag_outfile>${KERNEL_LST_NAME}
            $<TARGET_PROPERTY:bintools,disassembly_flag_final>
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_LST_NAME}
    )
endif()

if(CONFIG_OUTPUT_SYMBOLS)
  list(APPEND
    post_build_commands
    COMMAND $<TARGET_PROPERTY:bintools,symbols_command>
            $<TARGET_PROPERTY:bintools,symbols_flag>
            $<TARGET_PROPERTY:bintools,symbols_infile>${KERNEL_ELF_NAME}
            $<TARGET_PROPERTY:bintools,symbols_outfile>${KERNEL_SYMBOLS_NAME}
            $<TARGET_PROPERTY:bintools,symbols_final>
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_SYMBOLS_NAME}
    )
endif()

if(CONFIG_OUTPUT_STAT)
  list(APPEND
    post_build_commands
    COMMAND $<TARGET_PROPERTY:bintools,readelf_command>
            $<TARGET_PROPERTY:bintools,readelf_flag>
            $<TARGET_PROPERTY:bintools,readelf_flag_headers>
            $<TARGET_PROPERTY:bintools,readelf_flag_infile>${KERNEL_ELF_NAME}
            $<TARGET_PROPERTY:bintools,readelf_flag_outfile>${KERNEL_STAT_NAME}
            $<TARGET_PROPERTY:bintools,readelf_flag_final>
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_STAT_NAME}
    )
endif()

if(CONFIG_BUILD_OUTPUT_STRIPPED)
  list(APPEND
    post_build_commands
    COMMAND $<TARGET_PROPERTY:bintools,strip_command>
            $<TARGET_PROPERTY:bintools,strip_flag>
            $<TARGET_PROPERTY:bintools,strip_flag_all>
            $<TARGET_PROPERTY:bintools,strip_flag_infile>${KERNEL_ELF_NAME}
            $<TARGET_PROPERTY:bintools,strip_flag_outfile>${KERNEL_STRIP_NAME}
            $<TARGET_PROPERTY:bintools,strip_flag_final>
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_STRIP_NAME}
    )
endif()

if(CONFIG_BUILD_OUTPUT_COMPRESS_DEBUG_SECTIONS)
  list(APPEND
    post_build_commands
    COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
            $<TARGET_PROPERTY:bintools,elfconvert_flag>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_compress_debug_sections>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
)
endif()

if(CONFIG_BUILD_OUTPUT_EXE)
  if (NOT CONFIG_NATIVE_LIBRARY)
    list(APPEND
      post_build_commands
      COMMAND
      ${CMAKE_COMMAND} -E copy ${KERNEL_ELF_NAME} ${KERNEL_EXE_NAME}
      )
    list(APPEND
      post_build_byproducts
      ${KERNEL_EXE_NAME}
      )
  else()
    if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
      set(MAKE "${CMAKE_MAKE_PROGRAM}" CACHE FILEPATH "cmake defined make")
    endif()
    find_program(MAKE make REQUIRED)
    add_custom_target(native_runner_executable
      ALL
      COMMENT "Building native simulator runner, and linking final executable"
      COMMAND
      ${MAKE} -f ${ZEPHYR_BASE}/scripts/native_simulator/Makefile all --warn-undefined-variables
        -r NSI_CONFIG_FILE=${APPLICATION_BINARY_DIR}/zephyr/NSI/nsi_config
      # nsi_config is created by the board cmake file
      DEPENDS ${logical_target_for_zephyr_elf}
      BYPRODUCTS ${KERNEL_EXE_NAME}
    )
  endif()
  set(BYPRODUCT_KERNEL_EXE_NAME "${PROJECT_BINARY_DIR}/${KERNEL_EXE_NAME}" CACHE FILEPATH "Kernel exe file" FORCE)
endif()

if(CONFIG_BUILD_OUTPUT_INFO_HEADER)
  list(APPEND
    post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/gen_image_info.py
    --elf-file=${KERNEL_ELF_NAME}
    --header-file=${PROJECT_BINARY_DIR}/include/public/zephyr_image_info.h
    $<$<BOOL:${adjustment}>:--adjusted-lma=${adjustment}>
    )
  list(APPEND
    post_build_byproducts
    ${PROJECT_BINARY_DIR}/include/public/zephyr_image_info.h
    )
endif()

if (CONFIG_LLEXT AND CONFIG_LLEXT_EXPORT_BUILTINS_BY_SLID)
  #slidgen must be the first post-build command to be executed
  #on the Zephyr ELF to ensure that all other commands, such as
  #binary file generation, are operating on a preparated ELF.
  list(PREPEND
    post_build_commands
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/llext_prepare_exptab.py
    --elf-file ${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}
    --slid-listing ${PROJECT_BINARY_DIR}/slid_listing.txt
  )
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
  set(check_init_priorities_input
      $<IF:$<TARGET_EXISTS:native_runner_executable>,${BYPRODUCT_KERNEL_EXE_NAME},${BYPRODUCT_KERNEL_ELF_NAME}>
  )
  set(check_init_priorities_command
      ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/check_init_priorities.py
      --elf-file=${check_init_priorities_input}
  )
  set(check_init_priorities_dependencies
      ${logical_target_for_zephyr_elf}
      $<$<TARGET_EXISTS:native_runner_executable>:native_runner_executable>
  )

  if(CONFIG_CHECK_INIT_PRIORITIES)
    if(TARGET native_runner_executable)
      add_custom_command(TARGET native_runner_executable POST_BUILD
                         COMMAND ${check_init_priorities_command}
      )
    else()
      list(APPEND post_build_commands COMMAND ${check_init_priorities_command})
    endif()
  endif()

  add_custom_target(
    initlevels
    COMMAND ${check_init_priorities_command} --initlevels
    DEPENDS ${check_init_priorities_dependencies}
    USES_TERMINAL
  )
endif()

# Generate signed (MCUboot or other) related artifacts as needed. Priority is:
# * Sysbuild (if set)
# * SIGNING_SCRIPT target property (if set)
# * MCUboot signing script (if MCUboot is enabled)
zephyr_get(signing_script VAR SIGNING_SCRIPT SYSBUILD)

if(NOT signing_script)
  get_target_property(signing_script zephyr_property_target SIGNING_SCRIPT)

  if(NOT signing_script AND CONFIG_BOOTLOADER_MCUBOOT)
    set(signing_script ${CMAKE_CURRENT_LIST_DIR}/cmake/mcuboot.cmake)
  endif()
endif()

# Include signing script, if set
if(signing_script)
  message(STATUS "Including signing script: ${signing_script}")

  include(${signing_script})
endif()

# Generate USB-C VIF policies in XML format
if (CONFIG_BUILD_OUTPUT_VIF)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/vif.cmake)
endif()

get_property(extra_post_build_commands
  GLOBAL PROPERTY
  extra_post_build_commands
  )

list(APPEND
  post_build_commands
  ${extra_post_build_commands}
  )

get_property(extra_post_build_byproducts
  GLOBAL PROPERTY
  extra_post_build_byproducts
  )

list(APPEND
  post_build_byproducts
  ${extra_post_build_byproducts}
  )

if(CONFIG_LOG_DICTIONARY_DB)
  set(LOG_DICT_DB_NAME ${PROJECT_BINARY_DIR}/log_dictionary.json)
  set(LOG_DICT_DB_NAME_ARG --json)
elseif(CONFIG_LOG_MIPI_SYST_USE_CATALOG)
  set(LOG_DICT_DB_NAME ${PROJECT_BINARY_DIR}/mipi_syst_collateral.xml)
  set(LOG_DICT_DB_NAME_ARG --syst)
endif()

if(LOG_DICT_DB_NAME_ARG)
  set(log_dict_gen_command
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/logging/dictionary/database_gen.py
      ${KERNEL_ELF_NAME}
      ${LOG_DICT_DB_NAME_ARG}=${LOG_DICT_DB_NAME}
      --build-header ${PROJECT_BINARY_DIR}/include/generated/zephyr/version.h
  )

  if (NOT CONFIG_LOG_DICTIONARY_DB_TARGET)
    # If not using a separate target for generating logging dictionary
    # database, add the generation to post build command to make sure
    # the database is actually being generated.
    list(APPEND
      post_build_commands
      COMMAND ${CMAKE_COMMAND} -E echo "Generating logging dictionary database: ${LOG_DICT_DB_NAME}"
      COMMAND ${log_dict_gen_command}
    )
    list(APPEND
      post_build_byproducts
      ${LOG_DICT_DB_NAME}
    )
  else()
    # Seprate build target for generating logging dictionary database.
    # This needs to be explicitly called/used to generate the database.
    add_custom_command(
      OUTPUT ${LOG_DICT_DB_NAME}
      COMMAND ${log_dict_gen_command}
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      COMMENT "Generating logging dictionary database: ${LOG_DICT_DB_NAME}"
      DEPENDS ${logical_target_for_zephyr_elf}
    )
    add_custom_target(log_dict_db_gen DEPENDS ${LOG_DICT_DB_NAME})
  endif()
endif()

# Add post_build_commands to post-process the final .elf file produced by
# either the ZEPHYR_LINK_STAGE_EXECUTABLE or the KERNEL_ELF executable
# targets above.
add_custom_command(
  TARGET ${logical_target_for_zephyr_elf}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Generating files from ${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME} for board: ${BOARD}"
  ${post_build_commands}
  BYPRODUCTS
  ${post_build_byproducts}
  COMMAND_EXPAND_LISTS
  )

# To populate with hex files to merge, do the following:
#  set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE ${my_local_list})
# Note that the zephyr.hex file will not be included automatically.
get_property(HEX_FILES_TO_MERGE GLOBAL PROPERTY HEX_FILES_TO_MERGE)
if(HEX_FILES_TO_MERGE)
  # Merge in out-of-tree hex files.
  set(MERGED_HEX_NAME merged.hex)

  add_custom_command(
    OUTPUT ${MERGED_HEX_NAME}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/mergehex.py
    -o ${MERGED_HEX_NAME}
    ${HEX_FILES_TO_MERGE}
    DEPENDS ${HEX_FILES_TO_MERGE} ${logical_target_for_zephyr_elf}
    )

  add_custom_target(mergehex ALL DEPENDS ${MERGED_HEX_NAME})
  list(APPEND RUNNERS_DEPS mergehex)

  message(VERBOSE "Merging hex files: ${HEX_FILES_TO_MERGE}")
endif()

if(SUPPORTED_EMU_PLATFORMS)
  list(GET SUPPORTED_EMU_PLATFORMS 0 default_emu)
  if(EXISTS ${ZEPHYR_BASE}/cmake/emu/${default_emu}.cmake)
    add_custom_target(run DEPENDS run_${default_emu})
  endif()

  foreach(EMU_PLATFORM ${SUPPORTED_EMU_PLATFORMS})
    if(EXISTS ${ZEPHYR_BASE}/cmake/emu/${EMU_PLATFORM}.cmake)
      include(${ZEPHYR_BASE}/cmake/emu/${EMU_PLATFORM}.cmake)
    endif()
  endforeach()

  if(TARGET debugserver_${default_emu})
    add_custom_target(debugserver DEPENDS debugserver_${default_emu})
  endif()
else()
  add_custom_target(run
    COMMAND
    ${CMAKE_COMMAND} -E echo
    "==================================================="
    "Emulation/Simulation not supported with this board."
    "==================================================="
    )
endif()

add_subdirectory(cmake/flash)
add_subdirectory(cmake/usage)
add_subdirectory(cmake/reports)

if(NOT CONFIG_TEST)
if(CONFIG_ASSERT AND (NOT CONFIG_FORCE_NO_ASSERT))
  message(WARNING "__ASSERT() statements are globally ENABLED")
endif()
endif()

if(CONFIG_BOARD_DEPRECATED_RELEASE)
  message(WARNING "
      WARNING:  The board '${BOARD}' is deprecated and will be
      removed in version ${CONFIG_BOARD_DEPRECATED_RELEASE}"
  )
endif()

if(CONFIG_SOC_DEPRECATED_RELEASE)
  message(WARNING "
      WARNING:  The SoC '${SOC_NAME}' is deprecated and will be
      removed in version ${CONFIG_SOC_DEPRECATED_RELEASE}"
  )
endif()

# In CMake projects, 'CMAKE_BUILD_TYPE' usually determines the
# optimization flag, but in Zephyr it is determined through
# Kconfig. Here we give a warning when there is a mismatch between the
# two in case the user is not aware of this.
set(build_types None Debug Release RelWithDebInfo MinSizeRel)

if((CMAKE_BUILD_TYPE IN_LIST build_types) AND (NOT NO_BUILD_TYPE_WARNING))
  string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_uppercase)
  # The CMAKE_C_FLAGS_<build_type> is a string, so we do a regex to see if the
  # optimization flag is present in that string.
  # To avoid false-positive matches, the flag must either be matched first
  # or last in string, or come after / followed by minimum a space.
  if(NOT (CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_uppercase} MATCHES "(^| )${OPTIMIZATION_FLAG}($| )"))
    message(WARNING "
      The CMake build type was set to '${CMAKE_BUILD_TYPE}', but the optimization flag was set to '${OPTIMIZATION_FLAG}'.
      This may be intentional and the warning can be turned off by setting the CMake variable 'NO_BUILD_TYPE_WARNING'"
      )
  endif()
endif()

# Extension Development Kit (EDK) generation.
if(CONFIG_LLEXT_EDK)
  if(CONFIG_LLEXT_EDK_FORMAT_TAR_XZ)
    set(llext_edk_extension "tar.xz")
  elseif(CONFIG_LLEXT_EDK_FORMAT_TAR_ZSTD)
    set(llext_edk_extension "tar.Z")
  elseif(CONFIG_LLEXT_EDK_FORMAT_ZIP)
    set(llext_edk_extension "zip")
  else()
    message(FATAL_ERROR "Unsupported LLEXT_EDK_FORMAT choice")
  endif()
  set(llext_edk_file ${PROJECT_BINARY_DIR}/${CONFIG_LLEXT_EDK_NAME}.${llext_edk_extension})

  # TODO maybe generate flags for C CXX ASM
  zephyr_get_compile_definitions_for_lang(C zephyr_defs)
  zephyr_get_compile_options_for_lang(C zephyr_flags)

  # Filter out non LLEXT and LLEXT_EDK flags - and add required ones
  llext_filter_zephyr_flags(LLEXT_REMOVE_FLAGS ${zephyr_flags} llext_filt_flags)
  llext_filter_zephyr_flags(LLEXT_EDK_REMOVE_FLAGS ${llext_filt_flags} llext_filt_flags)

  set(llext_edk_cflags ${zephyr_defs} -DLL_EXTENSION_BUILD)
  list(APPEND llext_edk_cflags ${llext_filt_flags})
  list(APPEND llext_edk_cflags ${LLEXT_APPEND_FLAGS})
  list(APPEND llext_edk_cflags ${LLEXT_EDK_APPEND_FLAGS})

  build_info(llext-edk file PATH ${llext_edk_file})
  build_info(llext-edk cflags VALUE ${llext_edk_cflags})
  build_info(llext-edk include-dirs VALUE "$<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>")

  add_custom_command(
    OUTPUT ${llext_edk_file}
    # Regenerate syscalls in case CONFIG_LLEXT_EDK_USERSPACE_ONLY
    COMMAND ${CMAKE_COMMAND}
      -E make_directory edk/include/generated/zephyr
    COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/build/gen_syscalls.py
      --json-file        ${syscalls_json}                     # Read this file
      --base-output      edk/include/generated/zephyr/syscalls           # Write to this dir
      --syscall-dispatch edk/include/generated/zephyr/syscall_dispatch.c # Write this file
      --syscall-list     ${edk_syscall_list_h}
      $<$<BOOL:${CONFIG_LLEXT_EDK_USERSPACE_ONLY}>:--userspace-only>
      ${SYSCALL_LONG_REGISTERS_ARG}
      ${SYSCALL_SPLIT_TIMEOUT_ARG}
    COMMAND ${CMAKE_COMMAND}
        -P ${ZEPHYR_BASE}/cmake/llext-edk.cmake
    DEPENDS ${logical_target_for_zephyr_elf} build_info_yaml_saved
    COMMAND_EXPAND_LISTS
  )
  add_custom_target(llext-edk DEPENDS ${llext_edk_file})
endif()

# @Intent: Set compiler specific flags for standard C/C++ includes
# Done at the very end, so any other system includes which may
# be added by Zephyr components were first in list.
# Note, the compile flags are moved, but the system include is still present here.
zephyr_compile_options($<TARGET_PROPERTY:compiler,nostdinc>)
target_include_directories(zephyr_interface SYSTEM INTERFACE $<TARGET_PROPERTY:compiler,nostdinc_include>)

if(CONFIG_MINIMAL_LIBCPP)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,nostdincxx>>)
endif()

# Finally export all build flags from Zephyr
add_subdirectory_ifdef(
  CONFIG_MAKEFILE_EXPORTS
  cmake/makefile_exports
  )

toolchain_linker_finalize()

# export build information
build_info(zephyr version VALUE ${PROJECT_VERSION_STR})
build_info(zephyr zephyr-base VALUE ${ZEPHYR_BASE})

yaml_save(NAME build_info)
