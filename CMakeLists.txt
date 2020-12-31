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


# Verify that the toolchain can compile a dummy file, if it is not we
# won't be able to test for compatibility with certain C flags.
zephyr_check_compiler_flag(C "" toolchain_is_ok)
assert(toolchain_is_ok "The toolchain is unable to build a dummy C file. See CMakeError.log.")

# In some cases the "final" things are not used at all and "_prebuilt"
# is the last station. See "logical_target_for_zephyr_elf" below for
# details.
set(CMAKE_EXECUTABLE_SUFFIX .elf)
set(ZEPHYR_PREBUILT_EXECUTABLE zephyr_prebuilt)
set(ZEPHYR_FINAL_EXECUTABLE    zephyr_final)

# Set some phony targets to collect dependencies
set(OFFSETS_H_TARGET           offsets_h)
set(SYSCALL_LIST_H_TARGET      syscall_list_h_target)
set(DRIVER_VALIDATION_H_TARGET driver_validation_h_target)
set(KOBJ_TYPES_H_TARGET        kobj_types_h_target)
set(LINKER_SCRIPT_TARGET       linker_script_target)
set(PARSE_SYSCALLS_TARGET      parse_syscalls_target)

define_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT BRIEF_DOCS " " FULL_DOCS " ")
set_property(   GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-little${ARCH}) # BFD format

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

zephyr_include_directories(
  include
  ${PROJECT_BINARY_DIR}/include/generated
  ${USERINCLUDE}
  ${STDINCLUDE}
)

# Don't add non-existing include directories, it creates noise and
# warnings in some tooling
foreach(optional_include_dir
  ${SOC_DIR}/${ARCH}/${SOC_PATH}
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/include
  ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/include
  ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/common/include
  )
  if(EXISTS ${optional_include_dir})
    zephyr_include_directories(${optional_include_dir})
  endif()
endforeach()

zephyr_compile_definitions(
  KERNEL
  __ZEPHYR__=1
)

# @Intent: Set compiler flags to enable buffer overflow checks in libc functions
# @config in CONFIG_NO_OPTIMIZATIONS optional : Optimizations may affect security
zephyr_compile_definitions($<TARGET_PROPERTY:compiler,security_fortify> )

# @Intent: Set compiler flags to detect general stack overflows across all functions
if(CONFIG_STACK_CANARIES)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,security_canaries>)
endif()

if(BUILD_VERSION)
  zephyr_compile_definitions(
    BUILD_VERSION=${BUILD_VERSION}
  )
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
  elseif(CONFIG_STD_CPP11)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp11>) # Default in kconfig
  elseif(CONFIG_STD_CPP14)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp14>)
  elseif(CONFIG_STD_CPP17)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp17>)
  elseif(CONFIG_STD_CPP2A)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp2a>)
  else()
    assert(0 "Unreachable code. Expected C++ standard to have been chosen. See Kconfig.zephyr.")
  endif()

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

# @Intent: Set compiler specific flag for bare metal freestanding option
zephyr_compile_options($<TARGET_PROPERTY:compiler,freestanding>)

# @Intent: Set compiler specific flag for tentative definitions, no-common
zephyr_compile_options($<TARGET_PROPERTY:compiler,no_common>)

# @Intent: Set compiler specific flag for production of debug information
zephyr_compile_options($<TARGET_PROPERTY:compiler,debug>)

zephyr_compile_options(
  ${TOOLCHAIN_C_FLAGS}
)

# @Intent: Obtain compiler specific flags related to assembly
# ToDo: Remember to get feedback from Oticon on this, as they might use the `ASM_BASE_FLAG` since this is done this way.
zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:asm,required>>)

# @Intent: Enforce standard integer type correspondance to match Zephyr usage.
# (must be after compiler specific flags)
zephyr_compile_options("SHELL: $<TARGET_PROPERTY:compiler,imacros> ${ZEPHYR_BASE}/include/toolchain/zephyr_stdint.h")

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

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'
include(cmake/extra_flags.cmake)

zephyr_cc_option(-fno-asynchronous-unwind-tables)
zephyr_cc_option(-fno-pie)
zephyr_cc_option(-fno-pic)
zephyr_cc_option(-fno-strict-overflow)

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

  if(NOT ${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "xcc")
    zephyr_cc_option(-fno-defer-pop)
  endif()
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
  set(APP_SMEM_ALIGNED_DEP app_smem_aligned_linker)
  set(APP_SMEM_UNALIGNED_DEP app_smem_unaligned_linker)
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

configure_file(version.h.in ${PROJECT_BINARY_DIR}/include/generated/version.h)

# Error-out when the deprecated naming convention is found (until
# after 1.14.0 has been released)
foreach(path
    ${BOARD_DIR}/dts.fixup
    ${PROJECT_SOURCE_DIR}/soc/${ARCH}/${SOC_PATH}/dts.fixup
    ${APPLICATION_SOURCE_DIR}/dts.fixup
    )
  if(EXISTS ${path})
    message(FATAL_ERROR
      "A deprecated filename has been detected. Porting is required."
      "The file '${path}' exists, but it should be named dts_fixup.h instead."
      "See https://github.com/zephyrproject-rtos/zephyr/pull/10352 for more details"
      )
  endif()
endforeach()

set_ifndef(  DTS_BOARD_FIXUP_FILE                                   ${BOARD_DIR}/dts_fixup.h)
set_ifndef(    DTS_SOC_FIXUP_FILE                 ${SOC_DIR}/${ARCH}/${SOC_PATH}/dts_fixup.h)
set(           DTS_APP_FIXUP_FILE                      ${APPLICATION_SOURCE_DIR}/dts_fixup.h)

set_ifndef(DTS_CAT_OF_FIXUP_FILES ${ZEPHYR_BINARY_DIR}/include/generated/devicetree_fixups.h)

# Concatenate the fixups into a single header file for easy
# #include'ing
file(WRITE ${DTS_CAT_OF_FIXUP_FILES} "/* May only be included by devicetree.h */\n\n")
set(DISCOVERED_FIXUP_FILES)
foreach(fixup_file
    ${DTS_BOARD_FIXUP_FILE}
    ${DTS_SOC_FIXUP_FILE}
    ${DTS_APP_FIXUP_FILE}
    ${shield_dts_fixups}
    )
  if(EXISTS ${fixup_file})
    file(READ ${fixup_file} contents)
    file(APPEND ${DTS_CAT_OF_FIXUP_FILES} "${contents}")
    string(APPEND DISCOVERED_FIXUP_FILES "- ${fixup_file}\n")
  endif()
endforeach()

if (DISCOVERED_FIXUP_FILES)
  message(WARNING "One or more dts_fixup.h files detected:\n${DISCOVERED_FIXUP_FILES}Use of these files is deprecated; use the devicetree.h API instead.")
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
  ${ZEPHYR_BASE}/scripts/subfolder_list.py
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
    ${ZEPHYR_BASE}/scripts/subfolder_list.py
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
  ${ZEPHYR_BASE}/scripts/parse_syscalls.py
   --include          ${ZEPHYR_BASE}/include        # Read files from this dir
   --include          ${ZEPHYR_BASE}/drivers        # For net sockets
   --include          ${ZEPHYR_BASE}/subsys/net     # More net sockets
  ${parse_syscalls_include_args}                    # Read files from these dirs also
  --json-file        ${syscalls_json}               # Write this file
  --tag-struct-file  ${struct_tags_json}            # Write subsystem list to this file
  DEPENDS ${syscalls_subdirs_trigger} ${PARSE_SYSCALLS_HEADER_DEPENDS}
  )

add_custom_target(${SYSCALL_LIST_H_TARGET} DEPENDS ${syscall_list_h})

# This only works for CMake version >=3.15, but as the property is ignored on
# older CMake versions there is no reason to check for version.
set_property(TARGET ${SYSCALL_LIST_H_TARGET}
             APPEND PROPERTY
             ADDITIONAL_CLEAN_FILES
             ${CMAKE_CURRENT_BINARY_DIR}/include/generated/syscalls
)

# Only works with make.
if(${CMAKE_VERSION} VERSION_LESS 3.15)
  set_property(DIRECTORY
               APPEND PROPERTY
               ADDITIONAL_MAKE_CLEAN_FILES
               ${CMAKE_CURRENT_BINARY_DIR}/include/generated/syscalls
  )
endif()

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
  set(SYSCALL_SPLIT_TIMEOUT_ARG --split-type k_timeout_t)
endif()

add_custom_command(OUTPUT include/generated/syscall_dispatch.c ${syscall_list_h}
  # Also, some files are written to include/generated/syscalls/
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/gen_syscalls.py
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
  ${ZEPHYR_BASE}/scripts/gen_kobject_list.py
  --validation-output ${DRV_VALIDATION}
  ${gen_kobject_list_include_args}
  $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
  DEPENDS
  ${ZEPHYR_BASE}/scripts/gen_kobject_list.py
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
  offsets_h
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
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/gen_offset_header.py
  -i $<TARGET_OBJECTS:${OFFSETS_LIB}>
  -o ${OFFSETS_H_PATH}
  DEPENDS
  ${OFFSETS_LIB}
  $<TARGET_OBJECTS:${OFFSETS_LIB}>
)
add_custom_target(${OFFSETS_H_TARGET} DEPENDS ${OFFSETS_H_PATH})

zephyr_include_directories(${TOOLCHAIN_INCLUDES})

zephyr_get_include_directories_for_lang(C ZEPHYR_INCLUDES)

add_subdirectory(kernel)

# Read list content
get_property(ZEPHYR_LIBS_PROPERTY GLOBAL PROPERTY ZEPHYR_LIBS)

foreach(zephyr_lib ${ZEPHYR_LIBS_PROPERTY})
  # TODO: Could this become an INTERFACE property of zephyr_interface?
  add_dependencies(${zephyr_lib} zephyr_generated_headers)
endforeach()

get_property(OUTPUT_FORMAT        GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)

if (CONFIG_CODE_DATA_RELOCATION)
  set(CODE_RELOCATION_DEP code_relocation_source_lib)
endif() # CONFIG_CODE_DATA_RELOCATION

configure_linker_script(
  linker.cmd
  ""
  ${APP_SMEM_ALIGNED_DEP}
  ${CODE_RELOCATION_DEP}
  zephyr_generated_headers
  )

add_custom_target(
  ${LINKER_SCRIPT_TARGET}
  DEPENDS
  linker.cmd
  )

# Give the '${LINKER_SCRIPT_TARGET}' target all of the include directories so
# that cmake can successfully find the linker_script's header
# dependencies.
zephyr_get_include_directories_for_lang(C
  ZEPHYR_INCLUDE_DIRS
  STRIP_PREFIX # Don't use a -I prefix
  )
set_property(TARGET
  ${LINKER_SCRIPT_TARGET}
  PROPERTY INCLUDE_DIRECTORIES
  ${ZEPHYR_INCLUDE_DIRS}
  )

if(CONFIG_GEN_ISR_TABLES)
  if(CONFIG_GEN_SW_ISR_TABLE)
    list(APPEND GEN_ISR_TABLE_EXTRA_ARG --sw-isr-table)
  endif()

  if(CONFIG_GEN_IRQ_VECTOR_TABLE)
    list(APPEND GEN_ISR_TABLE_EXTRA_ARG --vector-table)
  endif()

  # isr_tables.c is generated from ${ZEPHYR_PREBUILT_EXECUTABLE} by
  # gen_isr_tables.py
  add_custom_command(
    OUTPUT isr_tables.c isrList.bin
    COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
            $<TARGET_PROPERTY:bintools,elfconvert_flag>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_intarget>${OUTPUT_FORMAT}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>binary
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_only>.intList
            $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>$<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>isrList.bin
            $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/arch/common/gen_isr_tables.py
    --output-source isr_tables.c
    --kernel $<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
    --intlist isrList.bin
    $<$<BOOL:${CONFIG_BIG_ENDIAN}>:--big-endian>
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--debug>
    ${GEN_ISR_TABLE_EXTRA_ARG}
    DEPENDS ${ZEPHYR_PREBUILT_EXECUTABLE}
    COMMAND_EXPAND_LISTS
    )
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES isr_tables.c)
endif()

# dev_handles.c is generated from ${ZEPHYR_PREBUILT_EXECUTABLE} by
# gen_handles.py
add_custom_command(
  OUTPUT dev_handles.c
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/gen_handles.py
  --output-source dev_handles.c
  --kernel $<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
  DEPENDS ${ZEPHYR_PREBUILT_EXECUTABLE}
  )
set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES dev_handles.c)

if(CONFIG_CODE_DATA_RELOCATION)
  # @Intent: Linker script to relocate .text, data and .bss sections
  toolchain_ld_relocation()
endif()

if(CONFIG_USERSPACE)
  zephyr_get_compile_options_for_lang_as_string(C compiler_flags_priv)
  string(REPLACE "$<TARGET_PROPERTY:compiler,coverage>" ""
         NO_COVERAGE_FLAGS "${compiler_flags_priv}"
  )

  get_property(include_dir_in_interface TARGET zephyr_interface
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES)

  get_property(sys_include_dir_in_interface TARGET zephyr_interface
    PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)

  get_property(compile_definitions_interface TARGET zephyr_interface
    PROPERTY INTERFACE_COMPILE_DEFINITIONS)

  set(GEN_KOBJ_LIST ${ZEPHYR_BASE}/scripts/gen_kobject_list.py)
  set(PROCESS_GPERF ${ZEPHYR_BASE}/scripts/process_gperf.py)

  set(OBJ_LIST           kobject_hash.gperf)
  set(OUTPUT_SRC_PRE     kobject_hash_preprocessed.c)
  set(OUTPUT_SRC         kobject_hash.c)
  set(OUTPUT_OBJ         kobject_hash.c.obj)
  set(OUTPUT_OBJ_RENAMED kobject_hash_renamed.o)

  # Essentially what we are doing here is extracting some information
  # out of the nearly finished elf file, generating the source code
  # for a hash table based on that information, and then compiling and
  # linking the hash table back into a now even more nearly finished
  # elf file. More information in gen_kobject_list.py --help.

  # Use the script GEN_KOBJ_LIST to scan the kernel binary's
  # (${ZEPHYR_PREBUILT_EXECUTABLE}) DWARF information to produce a table of kernel
  # objects (OBJ_LIST) which we will then pass to gperf
  add_custom_command(
    OUTPUT ${OBJ_LIST}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${GEN_KOBJ_LIST}
    --kernel $<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
    --gperf-output ${OBJ_LIST}
    ${gen_kobject_list_include_args}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    ${ZEPHYR_PREBUILT_EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(obj_list DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${OBJ_LIST})

  # Use gperf to generate C code (OUTPUT_SRC_PRE) which implements a
  # perfect hashtable based on OBJ_LIST
  add_custom_command(
    OUTPUT ${OUTPUT_SRC_PRE}
    COMMAND
    ${GPERF}
    --output-file ${OUTPUT_SRC_PRE}
    ${OBJ_LIST}
    DEPENDS obj_list ${OBJ_LIST}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(output_src_pre DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_SRC_PRE})

  # For our purposes the code/data generated by gperf is not optimal.
  #
  # The script PROCESS_GPERF creates a new c file OUTPUT_SRC based on
  # OUTPUT_SRC_PRE to greatly reduce the amount of code/data generated
  # since we know we are always working with pointer values
  add_custom_command(
    OUTPUT ${OUTPUT_SRC}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${PROCESS_GPERF}
    -i ${OUTPUT_SRC_PRE}
    -o ${OUTPUT_SRC}
    -p "struct z_object"
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS output_src_pre ${OUTPUT_SRC_PRE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(output_src DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_SRC})

  # We need precise control of where generated text/data ends up in the final
  # kernel image. Disable function/data sections and use objcopy to move
  # generated data into special section names
  add_library(output_lib STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_SRC}
    )

  set_source_files_properties(${OUTPUT_SRC} PROPERTIES COMPILE_FLAGS
    "${NO_COVERAGE_FLAGS} -fno-function-sections -fno-data-sections")

  set_source_files_properties(${OUTPUT_SRC}
    PROPERTIES COMPILE_DEFINITIONS "${compile_definitions_interface}")

  # Turn off -ffunction-sections, etc.
  # NB: Using a library instead of target_compile_options(output_lib
  # [...]) because a library's options have precedence
  add_library(output_lib_interface INTERFACE)

  target_link_libraries(output_lib output_lib_interface)

  foreach(incl ${include_dir_in_interface})
    target_include_directories(output_lib_interface INTERFACE ${incl})
  endforeach()

  foreach(incl ${sys_include_dir_in_interface})
    target_include_directories(output_lib_interface SYSTEM INTERFACE ${incl})
  endforeach()

  set(OUTPUT_OBJ_PATH ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/output_lib.dir/${OUTPUT_OBJ})

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_OBJ_RENAMED}
    COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
            $<TARGET_PROPERTY:bintools,elfconvert_flag>
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_rename>.data=.kobject_data.data
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_rename>.text=.kobject_data.text
            $<TARGET_PROPERTY:bintools,elfconvert_flag_section_rename>.rodata=.kobject_data.rodata
            $<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${OUTPUT_OBJ_PATH}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${OUTPUT_OBJ_RENAMED}
            $<TARGET_PROPERTY:bintools,elfconvert_flag_final>
    DEPENDS output_lib
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND_EXPAND_LISTS
    )
  add_custom_target(output_obj_renamed DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_OBJ_RENAMED})

  add_library(output_obj_renamed_lib STATIC IMPORTED GLOBAL)
  set_property(
    TARGET output_obj_renamed_lib
    PROPERTY
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_OBJ_RENAMED}
    )
  add_dependencies(
    output_obj_renamed_lib
    output_obj_renamed
    )

  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_OBJECT_FILES output_obj_renamed_lib)
endif()

# Read global variables into local variables
get_property(GKOF GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES)
get_property(GKSF GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES)


get_property(CSTD GLOBAL PROPERTY CSTD)
set_ifndef(CSTD c99)

# @Intent: Obtain compiler specific flag for specifying the c standard
zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,cstd>${CSTD}>
)

# @Intent: Configure linker scripts, i.e. generate linker scripts with variables substituted
toolchain_ld_configure_files()

if(CONFIG_USERSPACE)
  set(APP_SMEM_ALIGNED_LD "${PROJECT_BINARY_DIR}/include/generated/app_smem_aligned.ld")
  set(APP_SMEM_UNALIGNED_LD "${PROJECT_BINARY_DIR}/include/generated/app_smem_unaligned.ld")
  set(OBJ_FILE_DIR "${PROJECT_BINARY_DIR}/../")

  add_custom_target(
    ${APP_SMEM_ALIGNED_DEP}
    DEPENDS
    ${APP_SMEM_ALIGNED_LD}
    )

  add_custom_target(
    ${APP_SMEM_UNALIGNED_DEP}
    DEPENDS
    ${APP_SMEM_UNALIGNED_LD}
    )

  if(CONFIG_NEWLIB_LIBC)
    set(NEWLIB_PART -l libc.a z_libc_partition)
  endif()
  if(CONFIG_NEWLIB_LIBC_NANO)
    set(NEWLIB_PART -l libc_nano.a z_libc_partition)
  endif()

  add_custom_command(
    OUTPUT ${APP_SMEM_UNALIGNED_LD}
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/gen_app_partitions.py
    -d ${OBJ_FILE_DIR}
    -o ${APP_SMEM_UNALIGNED_LD}
    ${NEWLIB_PART}
    $<TARGET_PROPERTY:zephyr_property_target,COMPILE_OPTIONS>
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    kernel
    ${ZEPHYR_LIBS_PROPERTY}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/
    COMMAND_EXPAND_LISTS
    COMMENT "Generating app_smem_unaligned linker section"
    )

  configure_linker_script(
    linker_app_smem_unaligned.cmd
    "-DLINKER_APP_SMEM_UNALIGNED"
    ${CODE_RELOCATION_DEP}
    ${APP_SMEM_UNALIGNED_DEP}
    ${APP_SMEM_UNALIGNED_LD}
    zephyr_generated_headers
    )

  add_custom_target(
    linker_app_smem_unaligned_script
    DEPENDS
    linker_app_smem_unaligned.cmd
    )

  set_property(TARGET
    linker_app_smem_unaligned_script
    PROPERTY INCLUDE_DIRECTORIES
    ${ZEPHYR_INCLUDE_DIRS}
    )

  set(APP_SMEM_UNALIGNED_LIB app_smem_unaligned_output_obj_renamed_lib)
  add_executable(       app_smem_unaligned_prebuilt misc/empty_file.c)
  toolchain_ld_link_elf(
    TARGET_ELF            app_smem_unaligned_prebuilt
    OUTPUT_MAP            ${PROJECT_BINARY_DIR}/app_smem_unaligned_prebuilt.map
    LIBRARIES_PRE_SCRIPT  ""
    LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/linker_app_smem_unaligned.cmd
    LIBRARIES_POST_SCRIPT ""
    DEPENDENCIES          ${CODE_RELOCATION_DEP}
  )
  target_byproducts(TARGET app_smem_unaligned_prebuilt
                    BYPRODUCTS ${PROJECT_BINARY_DIR}/app_smem_unaligned_prebuilt.map
  )
  set_property(TARGET   app_smem_unaligned_prebuilt PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_app_smem_unaligned.cmd)
  add_dependencies(     app_smem_unaligned_prebuilt linker_app_smem_unaligned_script ${OFFSETS_LIB})

  add_custom_command(
    OUTPUT ${APP_SMEM_ALIGNED_LD}
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/gen_app_partitions.py
    -e $<TARGET_FILE:app_smem_unaligned_prebuilt>
    -o ${APP_SMEM_ALIGNED_LD}
    ${NEWLIB_PART}
    $<TARGET_PROPERTY:zephyr_property_target,COMPILE_OPTIONS>
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    kernel
    ${ZEPHYR_LIBS_PROPERTY}
    app_smem_unaligned_prebuilt
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/
    COMMAND_EXPAND_LISTS
    COMMENT "Generating app_smem_aligned linker section"
    )
endif()

# FIXME: Is there any way to get rid of empty_file.c?
add_executable(       ${ZEPHYR_PREBUILT_EXECUTABLE} misc/empty_file.c)
toolchain_ld_link_elf(
  TARGET_ELF            ${ZEPHYR_PREBUILT_EXECUTABLE}
  OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${ZEPHYR_PREBUILT_EXECUTABLE}.map
  LIBRARIES_PRE_SCRIPT  ""
  LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/linker.cmd
  DEPENDENCIES          ${CODE_RELOCATION_DEP}
)
target_byproducts(TARGET ${ZEPHYR_PREBUILT_EXECUTABLE}
                  BYPRODUCTS ${PROJECT_BINARY_DIR}/${ZEPHYR_PREBUILT_EXECUTABLE}.map
)
set_property(TARGET   ${ZEPHYR_PREBUILT_EXECUTABLE} PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.cmd)
add_dependencies(     ${ZEPHYR_PREBUILT_EXECUTABLE} ${LINKER_SCRIPT_TARGET} ${OFFSETS_LIB})


set(generated_kernel_files ${GKSF} ${GKOF})
if(NOT generated_kernel_files)
  # Use the prebuilt elf as the final elf since we don't have a
  # generation stage.
  set(logical_target_for_zephyr_elf ${ZEPHYR_PREBUILT_EXECUTABLE})
else()
  # The second linker pass uses the same source linker script of the
  # first pass (LINKER_SCRIPT), but this time with a different output
  # file and preprocessed with the define LINKER_PASS2.
  configure_linker_script(
    linker_pass_final.cmd
    "-DLINKER_PASS2"
    ${CODE_RELOCATION_DEP}
    ${ZEPHYR_PREBUILT_EXECUTABLE}
    zephyr_generated_headers
    )

  set(LINKER_PASS_FINAL_SCRIPT_TARGET linker_pass_final_script_target)
  add_custom_target(
    ${LINKER_PASS_FINAL_SCRIPT_TARGET}
    DEPENDS
    linker_pass_final.cmd
    )
  set_property(TARGET
    ${LINKER_PASS_FINAL_SCRIPT_TARGET}
    PROPERTY INCLUDE_DIRECTORIES
    ${ZEPHYR_INCLUDE_DIRS}
  )

  add_executable(       ${ZEPHYR_FINAL_EXECUTABLE} misc/empty_file.c ${GKSF})
  toolchain_ld_link_elf(
    TARGET_ELF            ${ZEPHYR_FINAL_EXECUTABLE}
    OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${ZEPHYR_FINAL_EXECUTABLE}.map
    LIBRARIES_PRE_SCRIPT  ${GKOF}
    LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/linker_pass_final.cmd
    LIBRARIES_POST_SCRIPT ""
    DEPENDENCIES          ${CODE_RELOCATION_DEP}
  )
  target_byproducts(TARGET ${ZEPHYR_FINAL_EXECUTABLE}
                    BYPRODUCTS ${PROJECT_BINARY_DIR}/${ZEPHYR_FINAL_EXECUTABLE}.map
  )
  set_property(TARGET   ${ZEPHYR_FINAL_EXECUTABLE} PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_pass_final.cmd)
  add_dependencies(     ${ZEPHYR_FINAL_EXECUTABLE} ${LINKER_PASS_FINAL_SCRIPT_TARGET})

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
  ${CMAKE_COMMAND} -E rename ${logical_target_for_zephyr_elf}.map ${KERNEL_MAP_NAME}
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

if(CONFIG_BUILD_OUTPUT_HEX OR BOARD_FLASH_RUNNER STREQUAL openocd)
  get_property(elfconvert_formats TARGET bintools PROPERTY elfconvert_formats)
  if(ihex IN_LIST elfconvert_formats)
    list(APPEND
      post_build_commands
      COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
              $<TARGET_PROPERTY:bintools,elfconvert_flag>
              ${GAP_FILL}
              $<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>ihex
              $<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>.comment
              $<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>COMMON
              $<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>.eh_frame
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
              $<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>.comment
              $<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>COMMON
              $<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>.eh_frame
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
  list(APPEND
    post_build_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/uf2conv.py
            -c
            -f ${CONFIG_BUILD_OUTPUT_UF2_FAMILY_ID}
            -b ${CONFIG_FLASH_LOAD_OFFSET}
            -o ${KERNEL_UF2_NAME}
            ${KERNEL_BIN_NAME}
  )
  list(APPEND
    post_build_byproducts
    ${KERNEL_UF2_NAME}
  )
endif()

# Cleanup intermediate files
if(CONFIG_CLEANUP_INTERMEDIATE_FILES)
    list(APPEND
      post_build_commands
      COMMAND
      # This file can be very large in some cases, delete it as we do not need it.
      ${CMAKE_COMMAND} -E remove ${ZEPHYR_PREBUILT_EXECUTABLE}.elf
      )
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

if(CONFIG_OUTPUT_STAT)
#  zephyr_post_build(TOOLS bintools COMMAND readelf FLAGS headers INFILE file OUTFILE outfile)
  list(APPEND
    post_build_commands
    COMMAND $<TARGET_PROPERTY:bintools,readelf_command>
            $<TARGET_PROPERTY:bintools,readelf_flag>
            $<TARGET_PROPERTY:bintools,readelf_flag_headers>
            $<TARGET_PROPERTY:bintools,readelf_flag_infile> ${KERNEL_ELF_NAME}
            $<TARGET_PROPERTY:bintools,readelf_flag_outfile> ${KERNEL_STAT_NAME}
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

# Add post_build_commands to post-process the final .elf file produced by
# either the ZEPHYR_PREBUILT_EXECUTABLE or the KERNEL_ELF executable
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
    ${ZEPHYR_BASE}/scripts/mergehex.py
    -o ${MERGED_HEX_NAME}
    ${HEX_FILES_TO_MERGE}
    DEPENDS ${HEX_FILES_TO_MERGE} ${logical_target_for_zephyr_elf}
    )

  add_custom_target(mergehex ALL DEPENDS ${MERGED_HEX_NAME})
  list(APPEND RUNNERS_DEPS mergehex)

  message(VERBOSE "Merging hex files: ${HEX_FILES_TO_MERGE}")
endif()

if(EMU_PLATFORM)
  include(${ZEPHYR_BASE}/cmake/emu/${EMU_PLATFORM}.cmake)
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

  if(NOT (${OPTIMIZATION_FLAG} IN_LIST CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE_uppercase}))
    message(WARNING "
      The CMake build type was set to '${CMAKE_BUILD_TYPE}', but the optimization flag was set to '${OPTIMIZATION_FLAG}'.
      This may be intentional and the warning can be turned off by setting the CMake variable 'NO_BUILD_TYPE_WARNING'"
      )
  endif()
endif()

# @Intent: Set compiler specific flags for standard C includes
# Done at the very end, so any other system includes which may
# be added by Zephyr components were first in list.
# Note, the compile flags are moved, but the system include is still present here.
zephyr_compile_options($<TARGET_PROPERTY:compiler,nostdinc>)
target_include_directories(zephyr_interface SYSTEM INTERFACE $<TARGET_PROPERTY:compiler,nostdinc_include>)

# Finally export all build flags from Zephyr
add_subdirectory_ifdef(
  CONFIG_MAKEFILE_EXPORTS
  cmake/makefile_exports
  )
