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
# - device handles structs must be generated (CONFIG_HAS_DTS=y)
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

# ZEPHYR_PREBUILT_EXECUTABLE is used outside of this file, therefore keep the
# existing variable to allow slowly cleanup of linking stage handling.
# Three stage linking active: pre0 -> pre1 -> final, this will correspond to `pre1`
# Two stage linking active:   pre0 -> final, this will correspond to `pre0`
if(CONFIG_USERSPACE OR CONFIG_HAS_DTS)
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

define_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT BRIEF_DOCS " " FULL_DOCS " ")
set_property(   GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-little${ARCH}) # BFD format
define_property(GLOBAL PROPERTY PROPERTY_CODE_RELOCATION_TARGET
  BRIEF_DOCS "Code relocation target location" FULL_DOCS " ")

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

if(CONFIG_LEGACY_INCLUDE_PATH)
  zephyr_include_directories(include/zephyr)
endif()

zephyr_include_directories(
  include
  ${PROJECT_BINARY_DIR}/include/generated
  ${USERINCLUDE}
  ${STDINCLUDE}
)

include(${ZEPHYR_BASE}/cmake/linker_script/${ARCH}/linker.cmake OPTIONAL)

# Don't add non-existing include directories, it creates noise and
# warnings in some tooling
foreach(optional_include_dir
  ${SOC_DIR}/${ARCH}/${SOC_PATH}
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/include
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/include/${SOC_NAME}
  ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/include
  ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/common/include
  )
  if(EXISTS ${optional_include_dir})
    zephyr_include_directories(${optional_include_dir})
  endif()
endforeach()

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

# @Intent: Set compiler flags to enable buffer overflow checks in libc functions
# @config in CONFIG_NO_OPTIMIZATIONS optional : Optimizations may affect security
zephyr_compile_definitions($<TARGET_PROPERTY:compiler,security_fortify> )

# @Intent: Set compiler flags to detect general stack overflows across all functions
if(CONFIG_STACK_CANARIES)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,security_canaries>)
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
else()
  assert(0 "Unreachable code. Expected optimization level to have been chosen. See Kconfig.zephyr")
endif()

if(NOT CONFIG_ARCH_IS_SET)
  message(WARNING "\
None of the CONFIG_<arch> (e.g. CONFIG_X86) symbols are set. \
Select one of them from the SOC_SERIES_* symbol or, lacking that, from the \
SOC_* symbol.")
endif()

# Apply the final optimization flag(s)
zephyr_compile_options(${OPTIMIZATION_FLAG})

# @Intent: Obtain compiler specific flags related to C++ that are not influenced by kconfig
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,required>>)

# @Intent: Obtain compiler specific flags for compiling under different ISO standards of C++
if(CONFIG_CPLUSPLUS)
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
    assert(0 "Unreachable code. Expected C++ standard to have been chosen. See Kconfig.zephyr.")
  endif()
  set(CMAKE_CXX_COMPILE_FEATURES ${CMAKE_CXX_COMPILE_FEATURES} PARENT_SCOPE)

  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${STD_CPP_DIALECT_FLAGS}>)
endif()

if(NOT CONFIG_EXCEPTIONS)
  # @Intent: Obtain compiler specific flags related to C++ Exceptions
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,no_exceptions>>)
endif()

if(NOT CONFIG_RTTI)
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
zephyr_compile_options("SHELL: $<TARGET_PROPERTY:compiler,imacros> ${AUTOCONF_H}")

if(NOT CONFIG_PICOLIBC)
  # @Intent: Set compiler specific flag for bare metal freestanding option
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,freestanding>>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:compiler,freestanding>>)
endif()

# @Intent: Set compiler specific flag for tentative definitions, no-common
zephyr_compile_options($<TARGET_PROPERTY:compiler,no_common>)

# @Intent: Set compiler specific flag for production of debug information
zephyr_compile_options($<TARGET_PROPERTY:compiler,debug>)

if(CONFIG_COMPILER_COLOR_DIAGNOSTICS)
# @Intent: Set compiler specific flag for diagnostic messages
zephyr_compile_options($<TARGET_PROPERTY:compiler,diagnostic>)
endif()

if("${SPARSE}" STREQUAL "y")
  list(APPEND TOOLCHAIN_C_FLAGS -D__CHECKER__)
  # Avoid compiler "attribute directive ignored" warnings
  list(APPEND TOOLCHAIN_C_FLAGS -Wno-attributes)
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
  zephyr_compile_options("SHELL: $<TARGET_PROPERTY:compiler,imacros> ${ZEPHYR_BASE}/include/zephyr/toolchain/zephyr_stdint.h")
endif()

# Common toolchain-agnostic assembly flags
zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:ASM>:-D_ASMLANGUAGE>
)

# @Intent: Set fundamental linker specific flags
toolchain_ld_base()

toolchain_ld_force_undefined_symbols(
  _OffsetAbsSyms
  _ConfigAbsSyms
)

if(NOT CONFIG_NATIVE_APPLICATION)
  # @Intent: Set linker specific flags for bare metal target
  toolchain_ld_baremetal()
endif()

if(CONFIG_LIB_CPLUSPLUS)
  # @Intent: Set linker specific flags for C++
  toolchain_ld_cpp()
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

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'
include(cmake/extra_flags.cmake)

zephyr_cc_option(-fno-asynchronous-unwind-tables)

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

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
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

# If several match then the last one wins. This matters for instances
# like tests/ and samples/: they're inside all of them! Then let's
# strip as little as possible.
zephyr_cc_option(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=CMAKE_SOURCE_DIR)
zephyr_cc_option(-fmacro-prefix-map=${ZEPHYR_BASE}=ZEPHYR_BASE)
if(WEST_TOPDIR)
  zephyr_cc_option(-fmacro-prefix-map=${WEST_TOPDIR}=WEST_TOPDIR)
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

get_property(TOPT GLOBAL PROPERTY TOPT)
set_ifndef(  TOPT -Wl,-T) # clang doesn't pick -T for some reason and complains,
                          # while -Wl,-T works for both, gcc and clang

if(CONFIG_HAVE_CUSTOM_LINKER_SCRIPT)
  set(LINKER_SCRIPT ${APPLICATION_SOURCE_DIR}/${CONFIG_CUSTOM_LINKER_SCRIPT})
  if(NOT EXISTS ${LINKER_SCRIPT})
    set(LINKER_SCRIPT ${CONFIG_CUSTOM_LINKER_SCRIPT})
    assert_exists(CONFIG_CUSTOM_LINKER_SCRIPT)
  endif()
else()
  # Try a board specific linker file
  set(LINKER_SCRIPT ${BOARD_DIR}/linker.ld)
  if(NOT EXISTS ${LINKER_SCRIPT})
    # If not available, try an SoC specific linker file
    set(LINKER_SCRIPT ${SOC_DIR}/${ARCH}/${SOC_PATH}/linker.ld)
  endif()
endif()

if(NOT EXISTS ${LINKER_SCRIPT})
  message(FATAL_ERROR "Could not find linker script: '${LINKER_SCRIPT}'. Corrupted configuration?")
endif()

if(DEFINED BUILD_VERSION)
  set(build_version_argument "-DBUILD_VERSION=${BUILD_VERSION}")
elseif(NOT ZEPHYR_GIT_INDEX)
  set(ZEPHYR_GIT_INDEX ZEPHYR_GIT_INDEX-NOTFOUND CACHE PATH
    "Path to Zephyr git repository index file")
  if(EXISTS ${ZEPHYR_BASE}/.git/index)
    set(ZEPHYR_GIT_DIR ${ZEPHYR_BASE}/.git/index CACHE PATH
      "Path to Zephyr git repository index file" FORCE)
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
          "Path to Zephyr git repository index file" FORCE)
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
  OUTPUT ${PROJECT_BINARY_DIR}/include/generated/version.h
  COMMAND ${CMAKE_COMMAND} -DZEPHYR_BASE=${ZEPHYR_BASE}
    -DOUT_FILE=${PROJECT_BINARY_DIR}/include/generated/version.h
    ${build_version_argument}
    -P ${ZEPHYR_BASE}/cmake/gen_version_h.cmake
  DEPENDS ${ZEPHYR_BASE}/VERSION ${git_dependency}
)
add_custom_target(version_h DEPENDS ${PROJECT_BINARY_DIR}/include/generated/version.h)

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

if(EXISTS ${SOC_DIR}/${ARCH}/CMakeLists.txt)
  add_subdirectory(${SOC_DIR}/${ARCH} soc/${ARCH})
else()
  add_subdirectory(${SOC_DIR}/${ARCH}/${SOC_PATH} soc/${ARCH}/${SOC_PATH})
endif()

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
    set(ZEPHYR_CURRENT_MODULE_DIR ${ZEPHYR_${MODULE_NAME_UPPER}_MODULE_DIR})
    set(ZEPHYR_CURRENT_CMAKE_DIR ${ZEPHYR_${MODULE_NAME_UPPER}_CMAKE_DIR})
    add_subdirectory(${ZEPHYR_CURRENT_CMAKE_DIR} ${CMAKE_BINARY_DIR}/modules/${module_name})
  endif()
endforeach()
# Done processing modules, clear ZEPHYR_CURRENT_MODULE_DIR and ZEPHYR_CURRENT_CMAKE_DIR.
set(ZEPHYR_CURRENT_MODULE_DIR)
set(ZEPHYR_CURRENT_CMAKE_DIR)

set(syscall_list_h   ${CMAKE_CURRENT_BINARY_DIR}/include/generated/syscall_list.h)
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
  --trigger          ${syscalls_subdirs_trigger} # Trigger file that is used for json generation
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
    --trigger          ${syscalls_subdirs_trigger} # Trigger file that is used for json generation
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
endif()

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
   --include          ${ZEPHYR_BASE}/include        # Read files from this dir
   --include          ${ZEPHYR_BASE}/drivers        # For net sockets
   --include          ${ZEPHYR_BASE}/subsys/net     # More net sockets
  ${parse_syscalls_include_args}                    # Read files from these dirs also
  --json-file        ${syscalls_json}               # Write this file
  --tag-struct-file  ${struct_tags_json}            # Write subsystem list to this file
  DEPENDS ${syscalls_subdirs_trigger} ${PARSE_SYSCALLS_HEADER_DEPENDS}
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
             ${CMAKE_CURRENT_BINARY_DIR}/include/generated/syscalls
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

add_custom_command(OUTPUT include/generated/syscall_dispatch.c ${syscall_list_h}
  # Also, some files are written to include/generated/syscalls/
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/gen_syscalls.py
  --json-file        ${syscalls_json}                     # Read this file
  --base-output      include/generated/syscalls           # Write to this dir
  --syscall-dispatch include/generated/syscall_dispatch.c # Write this file
  --syscall-list     ${syscall_list_h}
  ${SYSCALL_LONG_REGISTERS_ARG}
  ${SYSCALL_SPLIT_TIMEOUT_ARG}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${PARSE_SYSCALLS_TARGET}
  )

# This is passed into all calls to the gen_kobject_list.py script.
set(gen_kobject_list_include_args --include ${struct_tags_json})

set(DRV_VALIDATION ${PROJECT_BINARY_DIR}/include/generated/driver-validation.h)
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
set(OFFSETS_H_PATH ${PROJECT_BINARY_DIR}/include/generated/offsets.h)

add_library(          ${OFFSETS_LIB} OBJECT ${OFFSETS_C_PATH})
target_include_directories(${OFFSETS_LIB} PRIVATE
  kernel/include
  ${ARCH_DIR}/${ARCH}/include
  )
target_link_libraries(${OFFSETS_LIB} zephyr_interface)
add_dependencies(zephyr_interface
  ${SYSCALL_LIST_H_TARGET}
  ${DRIVER_VALIDATION_H_TARGET}
  ${KOBJ_TYPES_H_TARGET}
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
    # In order to allow zephyr_library_relocate to be called prior to adding
    # library sources, we must parse and generate relocation information
    # once all libraries have been defined
    get_property(relocation_target TARGET ${zephyr_lib} PROPERTY
      PROPERTY_CODE_RELOCATION_TARGET)
    if(NOT "${relocation_target}" STREQUAL "")
      get_property(lib_source_dir TARGET ${zephyr_lib} PROPERTY SOURCE_DIR)
      foreach(file ${source_list})
        if(NOT IS_ABSOLUTE ${file})
          set(file ${lib_source_dir}/${file})
        endif()
        set_property(TARGET code_data_relocation_target APPEND PROPERTY
          COMPILE_DEFINITIONS "${relocation_target}:${file}")
      endforeach()
    endif()
  endif()

  # TODO: Could this become an INTERFACE property of zephyr_interface?
  add_dependencies(${zephyr_lib} zephyr_generated_headers)
endforeach()

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

if(CONFIG_PM_DEVICE_POWER_DOMAIN_DYNAMIC)
  set(number_of_dynamic_devices ${CONFIG_PM_DEVICE_POWER_DOMAIN_DYNAMIC_NUM})
else()
  set(number_of_dynamic_devices 0)
endif()

if(CONFIG_HAS_DTS)
  # dev_handles.c is generated from ${ZEPHYR_LINK_STAGE_EXECUTABLE} by
  # gen_handles.py
  add_custom_command(
    OUTPUT dev_handles.c
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_handles.py
    --output-source dev_handles.c
    --output-graphviz dev_graph.dot
    --num-dynamic-devices ${number_of_dynamic_devices}
    --kernel $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    --zephyr-base ${ZEPHYR_BASE}
    --start-symbol "$<TARGET_PROPERTY:linker,devices_start_symbol>"
    VERBATIM
    DEPENDS ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    )
  set_property(GLOBAL APPEND PROPERTY GENERATED_APP_SOURCE_FILES dev_handles.c)

  # gen_handles runs on `__device_handles_pass1` so pass this info to the linker script generator
  list(APPEND LINKER_PASS_${ZEPHYR_CURRENT_LINKER_PASS}_DEFINE "LINKER_DEVICE_HANDLES_PASS1")
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

get_property(CSTD GLOBAL PROPERTY CSTD)
set_ifndef(CSTD c99)

# @Intent: Obtain compiler specific flag for specifying the c standard
zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,cstd>${CSTD}>
)
set(CMAKE_C_COMPILE_FEATURES ${compile_features_${CSTD}} PARENT_SCOPE)

# @Intent: Configure linker scripts, i.e. generate linker scripts with variables substituted
toolchain_ld_configure_files()

if(CONFIG_USERSPACE)
  set(APP_SMEM_ALIGNED_LD "${PROJECT_BINARY_DIR}/include/generated/app_smem_aligned.ld")
  set(APP_SMEM_UNALIGNED_LD "${PROJECT_BINARY_DIR}/include/generated/app_smem_unaligned.ld")

  if(CONFIG_LINKER_USE_PINNED_SECTION)
    set(APP_SMEM_PINNED_ALIGNED_LD
        "${PROJECT_BINARY_DIR}/include/generated/app_smem_pinned_aligned.ld")
    set(APP_SMEM_PINNED_UNALIGNED_LD
        "${PROJECT_BINARY_DIR}/include/generated/app_smem_pinned_unaligned.ld")

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
endif()

if (CONFIG_USERSPACE)
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
    -p "struct z_object"
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

  set(KOBJECT_LINKER_HEADER_DATA "${PROJECT_BINARY_DIR}/include/generated/linker-kobject-prebuilt-data.h")

  add_custom_command(
    OUTPUT ${KOBJECT_LINKER_HEADER_DATA}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_kobject_placeholders.py
    --object $<TARGET_OBJECTS:kobj_prebuilt_hash_output_lib>
    --outdir ${PROJECT_BINARY_DIR}/include/generated
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
endif()

if(CONFIG_USERSPACE OR CONFIG_HAS_DTS)
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
    OUTPUT isr_tables.c isrList.bin
    COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
            $<TARGET_PROPERTY:bintools,elfconvert_flag>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_intarget>${OUTPUT_FORMAT}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>binary
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_only>.intList
            $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>$<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>isrList.bin
            $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_isr_tables.py
    --output-source isr_tables.c
    --kernel $<TARGET_FILE:${ZEPHYR_LINK_STAGE_EXECUTABLE}>
    --intlist isrList.bin
    $<$<BOOL:${CONFIG_BIG_ENDIAN}>:--big-endian>
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--debug>
    ${GEN_ISR_TABLE_EXTRA_ARG}
    DEPENDS ${ZEPHYR_LINK_STAGE_EXECUTABLE}
    COMMAND_EXPAND_LISTS
    )
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES isr_tables.c)
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
    -p "struct z_object"
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
target_byproducts(TARGET ${ZEPHYR_LINK_STAGE_EXECUTABLE}
                  BYPRODUCTS ${PROJECT_BINARY_DIR}/${ZEPHYR_LINK_STAGE_EXECUTABLE}.map
)
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

if(NOT CONFIG_BUILD_NO_GAP_FILL)
  # Use ';' as separator to get proper space in resulting command.
  set(GAP_FILL "$<TARGET_PROPERTY:bintools,elfconvert_flag_gapfill>0xff")
endif()

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
  list(APPEND
    post_build_commands
    COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_lma_adjust>${adjustment}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KERNEL_ELF_NAME}
    )
endif()

if(NOT CONFIG_EXCEPTIONS)
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
              ${GAP_FILL}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>ihex
              ${remove_sections_argument_list}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KERNEL_HEX_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
      )
    list(APPEND
      post_build_byproducts
      ${KERNEL_HEX_NAME}
      # ${out_hex_byprod} # Is this needed ?
      )
  endif()
endif()

if(CONFIG_BUILD_OUTPUT_BIN)
  get_property(elfconvert_formats TARGET bintools PROPERTY elfconvert_formats)
  if(binary IN_LIST elfconvert_formats)
    list(APPEND
      post_build_commands
      COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
              $<TARGET_PROPERTY:bintools,elfconvert_flag>
              ${GAP_FILL}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>binary
              ${remove_sections_argument_list}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KERNEL_BIN_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
      )
    list(APPEND
      post_build_byproducts
      ${KERNEL_BIN_NAME}
      # ${out_hex_byprod} # Is this needed ?
      )
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
endif()

if(CONFIG_BUILD_OUTPUT_META)
  list(APPEND
    post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/zephyr_module.py
            ${WEST_ARG}
            ${ZEPHYR_MODULES_ARG}
            ${ZEPHYR_EXTRA_MODULES_ARG}
            --meta-out ${KERNEL_META_NAME}
            $<$<BOOL:${CONFIG_BUILD_OUTPUT_META_STATE_PROPAGATE}>:--meta-state-propagate>
  )
  list(APPEND
    post_build_byproducts
    ${KERNEL_META_NAME}
  )
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
              ${GAP_FILL}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>srec
              $<TARGET_PROPERTY:bintools,elfconvert_flag_srec_len>1
              $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KERNEL_S19_NAME}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
      )
    list(APPEND
      post_build_byproducts
      ${KERNEL_S19_NAME}
      # ${out_S19_byprod} # Is this needed ?

      )
  endif()
endif()

if(CONFIG_OUTPUT_DISASSEMBLY)
if(CONFIG_OUTPUT_DISASSEMBLE_ALL)
    set(disassembly_type "$<TARGET_PROPERTY:bintools,disassembly_flag_all>")
  else()
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
#    ${out_disassembly_byprod} # Needed ??
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
#  zephyr_post_build(TOOLS bintools COMMAND readelf FLAGS headers INFILE file OUTFILE outfile)
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

if(CONFIG_BUILD_OUTPUT_EXE)
  list(APPEND
    post_build_commands
    COMMAND
    ${CMAKE_COMMAND} -E copy ${KERNEL_ELF_NAME} ${KERNEL_EXE_NAME}
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_EXE_NAME}
    )
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

# Generate and use MCUboot related artifacts as needed.
if(CONFIG_BOOTLOADER_MCUBOOT)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/mcuboot.cmake)
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
  set(log_dict_db_output --json=${PROJECT_BINARY_DIR}/log_dictionary.json)
elseif(CONFIG_LOG_MIPI_SYST_USE_CATALOG)
  set(log_dict_db_output --syst=${PROJECT_BINARY_DIR}/mipi_syst_collateral.xml)
endif()

if(log_dict_db_output)
  list(APPEND
    post_build_commands
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/logging/dictionary/database_gen.py
    ${KERNEL_ELF_NAME}
    ${log_dict_db_output}
    --build-header ${PROJECT_BINARY_DIR}/include/generated/version.h
    )
  list(APPEND
    post_build_byproducts
    ${LOG_DICT_DB_NAME}
    )

  unset(log_dict_db_output)
endif()

# Add post_build_commands to post-process the final .elf file produced by
# either the ZEPHYR_LINK_STAGE_EXECUTABLE or the KERNEL_ELF executable
# targets above.
add_custom_command(
  TARGET ${logical_target_for_zephyr_elf}
  POST_BUILD
  ${post_build_commands}
  BYPRODUCTS
  ${post_build_byproducts}
  COMMENT "Generating files from ${KERNEL_ELF_NAME} for board: ${BOARD}"
  COMMAND_EXPAND_LISTS
  # NB: COMMENT only works for some CMake-Generators
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

# @Intent: Set compiler specific flags for standard C/C++ includes
# Done at the very end, so any other system includes which may
# be added by Zephyr components were first in list.
# Note, the compile flags are moved, but the system include is still present here.
zephyr_compile_options($<TARGET_PROPERTY:compiler,nostdinc>)
target_include_directories(zephyr_interface SYSTEM INTERFACE $<TARGET_PROPERTY:compiler,nostdinc_include>)

if(NOT CONFIG_LIB_CPLUSPLUS)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,nostdincxx>>)
endif()

# Finally export all build flags from Zephyr
add_subdirectory_ifdef(
  CONFIG_MAKEFILE_EXPORTS
  cmake/makefile_exports
  )
