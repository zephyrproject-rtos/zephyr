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
set(SYSCALL_MACROS_H_TARGET    syscall_macros_h_target)
set(SYSCALL_LIST_H_TARGET      syscall_list_h_target)
set(DRIVER_VALIDATION_H_TARGET driver_validation_h_target)
set(KOBJ_TYPES_H_TARGET        kobj_types_h_target)
set(LINKER_SCRIPT_TARGET       linker_script_target)


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
  kernel/include
  ${ARCH_DIR}/${ARCH}/include
  include
  include/drivers
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
toolchain_cc_security_fortify()

# @Intent: Set compiler flags to detect general stack overflows across all functions
if(CONFIG_STACK_CANARIES)
  toolchain_cc_security_canaries()
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
toolchain_cc_optimize_for_no_optimizations_flag(OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG)
toolchain_cc_optimize_for_debug_flag(OPTIMIZE_FOR_DEBUG_FLAG)
toolchain_cc_optimize_for_speed_flag(OPTIMIZE_FOR_SPEED_FLAG)
toolchain_cc_optimize_for_size_flag(OPTIMIZE_FOR_SIZE_FLAG)

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

# Apply the final optimization flag(s)
zephyr_compile_options(${OPTIMIZATION_FLAG})

# @Intent: Obtain compiler specific flags related to C++ that are not influenced by kconfig
toolchain_cc_cpp_base_flags(CPP_BASE_FLAGS)
foreach(flag ${CPP_BASE_FLAGS})
  zephyr_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:${flag}>
  )
endforeach()

# @Intent: Obtain compiler specific flags for compiling under different ISO standards of C++
toolchain_cc_cpp_dialect_std_98_flags(CPP_DIALECT_STD_98_FLAGS)
toolchain_cc_cpp_dialect_std_11_flags(CPP_DIALECT_STD_11_FLAGS)
toolchain_cc_cpp_dialect_std_14_flags(CPP_DIALECT_STD_14_FLAGS)
toolchain_cc_cpp_dialect_std_17_flags(CPP_DIALECT_STD_17_FLAGS)
toolchain_cc_cpp_dialect_std_2a_flags(CPP_DIALECT_STD_2A_FLAGS)

if(CONFIG_CPLUSPLUS)
  # From kconfig choice, pick a single dialect.
  # Kconfig choice ensures only one of these CONFIG_STD_CPP* is set.
  if(CONFIG_STD_CPP98)
    set(STD_CPP_DIALECT_FLAGS ${CPP_DIALECT_STD_98_FLAGS})
  elseif(CONFIG_STD_CPP11)
    set(STD_CPP_DIALECT_FLAGS ${CPP_DIALECT_STD_11_FLAGS}) # Default in kconfig
  elseif(CONFIG_STD_CPP14)
    set(STD_CPP_DIALECT_FLAGS ${CPP_DIALECT_STD_14_FLAGS})
  elseif(CONFIG_STD_CPP17)
    set(STD_CPP_DIALECT_FLAGS ${CPP_DIALECT_STD_17_FLAGS})
  elseif(CONFIG_STD_CPP2A)
    set(STD_CPP_DIALECT_FLAGS ${CPP_DIALECT_STD_2A_FLAGS})
  else()
    assert(0 "Unreachable code. Expected C++ standard to have been chosen. See Kconfig.zephyr.")
  endif()

  foreach(flag ${STD_CPP_DIALECT_FLAGS})
    zephyr_compile_options(
      $<$<COMPILE_LANGUAGE:CXX>:${flag}>
    )
  endforeach()
endif()

if(NOT CONFIG_EXCEPTIONS)
  # @Intent: Obtain compiler specific flags related to C++ Exceptions
  toolchain_cc_cpp_no_exceptions_flag(CPP_NO_EXCEPTIONS_FLAG)
  zephyr_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:${CPP_NO_EXCEPTIONS_FLAG}>
  )
endif()

if(NOT CONFIG_RTTI)
  # @Intent: Obtain compiler specific flags related to C++ Run Time Type Information
  toolchain_cc_cpp_no_rtti_flag(CPP_NO_RTTI_FLAG)
  zephyr_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:${CPP_NO_RTTI_FLAG}>
  )
endif()

if(CONFIG_MISRA_SANE)
  # @Intent: Obtain toolchain compiler flags relating to MISRA.
  toolchain_cc_warning_error_misra_sane(CC_MISRA_SANE_FLAG)
  toolchain_cc_cpp_warning_error_misra_sane(CPP_MISRA_SANE_FLAG)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:${CC_MISRA_SANE_FLAG}>)
  zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${CPP_MISRA_SANE_FLAG}>)
endif()

# @Intent: Set compiler specific macro inclusion of AUTOCONF_H
toolchain_cc_imacros(${AUTOCONF_H})

# @Intent: Set compiler specific flag for bare metal freestanding option
toolchain_cc_freestanding()

# @Intent: Set compiler specific flag for tentative definitions, no-common
toolchain_cc_nocommon()

# @Intent: Set compiler specific flag for production of debug information
toolchain_cc_produce_debug_info()

zephyr_compile_options(
  ${TOOLCHAIN_C_FLAGS}
)

# @Intent: Obtain compiler specific flags related to assembly
toolchain_cc_asm_base_flags(ASM_BASE_FLAG)
zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:ASM>:${ASM_BASE_FLAG}>
)

# @Intent: Enforce standard integer type correspondance to match Zephyr usage.
# (must be after compiler specific flags)
toolchain_cc_imacros(${ZEPHYR_BASE}/include/toolchain/zephyr_stdint.h)

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
toolchain_cc_warning_base()

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
  toolchain_cc_warning_dw_1()
endif()

if(W MATCHES "2")
  toolchain_cc_warning_dw_2()
endif()

if(W MATCHES "3")
  toolchain_cc_warning_dw_3()
endif()

# @Intent: Add extended, more specific, toolchain warning flags
toolchain_cc_warning_extended()

# @Intent: Trigger an error when a declaration does not specify a type
toolchain_cc_warning_error_implicit_int()

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'
include(cmake/extra_flags.cmake)

zephyr_cc_option(-fno-asynchronous-unwind-tables)
zephyr_cc_option(-fno-pie)
zephyr_cc_option(-fno-pic)
zephyr_cc_option(-fno-strict-overflow)

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

# If both match then the last one wins. This matters for tests/ and
# samples/ inside *both* CMAKE_SOURCE_DIR and ZEPHYR_BASE: for them
# let's strip the shortest prefix.
zephyr_cc_option(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=CMAKE_SOURCE_DIR)
zephyr_cc_option(-fmacro-prefix-map=${ZEPHYR_BASE}=ZEPHYR_BASE)
# TODO: -fmacro-prefix-map=modules/etc. "build/zephyr_modules.txt" might help.

# TODO: Archiver arguments
# ar_option(D)

# Declare MPU userspace dependencies before the linker scripts to make
# sure the order of dependencies are met
if(CONFIG_USERSPACE)
  set(APP_SMEM_ALIGNED_DEP app_smem_aligned_linker)
  set(APP_SMEM_UNALIGNED_DEP app_smem_unaligned_linker)
  if(CONFIG_ARM)
    set(PRIV_STACK_DEP priv_stacks_prebuilt)
  endif()
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

# Custom section support in linker scripts requires that the application source
# directory is in the preprocessor search path, in order to find the custom
# linker script fragments.
if(CONFIG_CUSTOM_RODATA_LD OR CONFIG_CUSTOM_RWDATA_LD OR CONFIG_CUSTOM_SECTIONS_LD)
  zephyr_include_directories(${APPLICATION_SOURCE_DIR})
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

set_ifndef(DTS_CAT_OF_FIXUP_FILES ${ZEPHYR_BINARY_DIR}/include/generated/generated_dts_board_fixups.h)

# Concatenate the fixups into a single header file for easy
# #include'ing
file(WRITE ${DTS_CAT_OF_FIXUP_FILES} "/* May only be included by generated_dts_board.h */\n\n")
foreach(fixup_file
    ${DTS_BOARD_FIXUP_FILE}
    ${DTS_SOC_FIXUP_FILE}
    ${DTS_APP_FIXUP_FILE}
    ${shield_dts_fixups}
    )
  if(EXISTS ${fixup_file})
    file(READ ${fixup_file} contents)
    file(APPEND ${DTS_CAT_OF_FIXUP_FILES} "${contents}")
  endif()
endforeach()

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
add_subdirectory(ext)
add_subdirectory(subsys)
add_subdirectory(drivers)

# Include zephyr modules generated CMake file.
if(EXISTS ${CMAKE_BINARY_DIR}/zephyr_modules.txt)
	file(STRINGS ${CMAKE_BINARY_DIR}/zephyr_modules.txt ZEPHYR_MODULES_TXT
		ENCODING UTF-8)

  foreach(module ${ZEPHYR_MODULES_TXT})
    # Match "<name>":"<path>" for each line of file, each corresponding to
    # one module. The use of quotes is required due to CMake not supporting
    # lazy regexes (it supports greedy only).
    string(REGEX REPLACE "\"(.*)\":\".*\"" "\\1" module_name ${module})
    string(REGEX REPLACE "\".*\":\"(.*)\"" "\\1" module_path ${module})
    # Note the second, binary_dir parameter requires the added
    # subdirectory to have its own, local cmake target(s). If not then
    # this binary_dir is created but stays empty. Object files land in
    # the main binary dir instead.
    # https://cmake.org/pipermail/cmake/2019-June/069547.html
    set(ZEPHYR_CURRENT_MODULE_DIR ${module_path})
    add_subdirectory(${module_path} ${CMAKE_BINARY_DIR}/modules/${module_name})
  endforeach()
  # Done processing modules, clear ZEPHYR_CURRENT_MODULE_DIR.
  set(ZEPHYR_CURRENT_MODULE_DIR)
endif()

set(syscall_macros_h ${ZEPHYR_BINARY_DIR}/include/generated/syscall_macros.h)

add_custom_target(${SYSCALL_MACROS_H_TARGET} DEPENDS ${syscall_macros_h})
add_custom_command(                          OUTPUT  ${syscall_macros_h}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/gen_syscall_header.py
  > ${syscall_macros_h}
  DEPENDS ${ZEPHYR_BASE}/scripts/gen_syscall_header.py
  )


set(syscall_list_h ${CMAKE_CURRENT_BINARY_DIR}/include/generated/syscall_list.h)
set(syscalls_json  ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/syscalls.json)

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

# SYSCALL_INCLUDE_DIRECTORY will include the directories that needs to be
# searched for syscall declarations if CONFIG_APPLICATION_DEFINED_SYSCALL is set
if(CONFIG_APPLICATION_DEFINED_SYSCALL)
  set(SYSCALL_INCLUDE_DIRECTORY --include ${APPLICATION_SOURCE_DIR})
endif()

add_custom_command(
  OUTPUT
  ${syscalls_json}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/parse_syscalls.py
   --include          ${ZEPHYR_BASE}/include        # Read files from this dir
  ${SYSCALL_INCLUDE_DIRECTORY}
  --json-file        ${syscalls_json}              # Write this file
  DEPENDS ${syscalls_subdirs_trigger} ${PARSE_SYSCALLS_HEADER_DEPENDS}
  )

add_custom_target(${SYSCALL_LIST_H_TARGET} DEPENDS             ${syscall_list_h})
add_custom_command(OUTPUT include/generated/syscall_dispatch.c ${syscall_list_h}
  # Also, some files are written to include/generated/syscalls/
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/gen_syscalls.py
  --json-file        ${syscalls_json}                     # Read this file
  --base-output      include/generated/syscalls           # Write to this dir
  --syscall-dispatch include/generated/syscall_dispatch.c # Write this file
  --syscall-list     ${syscall_list_h}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${syscalls_json}
  )

set(DRV_VALIDATION ${PROJECT_BINARY_DIR}/include/generated/driver-validation.h)
add_custom_command(
  OUTPUT ${DRV_VALIDATION}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/gen_kobject_list.py
  --validation-output ${DRV_VALIDATION}
  $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
  DEPENDS ${ZEPHYR_BASE}/scripts/gen_kobject_list.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
add_custom_target(${DRIVER_VALIDATION_H_TARGET} DEPENDS ${DRV_VALIDATION})

include($ENV{ZEPHYR_BASE}/cmake/kobj.cmake)
gen_kobj(KOBJ_INCLUDE_PATH)

# Generate offsets.c.obj from offsets.c
# Generate offsets.h     from offsets.c.obj

set(OFFSETS_LIB offsets)

set(OFFSETS_C_PATH ${ARCH_DIR}/${ARCH}/core/offsets/offsets.c)
set(OFFSETS_H_PATH ${PROJECT_BINARY_DIR}/include/generated/offsets.h)

add_library(          ${OFFSETS_LIB} OBJECT ${OFFSETS_C_PATH})
target_link_libraries(${OFFSETS_LIB} zephyr_interface)
add_dependencies(     ${OFFSETS_LIB}
  ${SYSCALL_LIST_H_TARGET}
  ${SYSCALL_MACROS_H_TARGET}
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
  add_dependencies(${zephyr_lib} ${OFFSETS_H_TARGET})
endforeach()

get_property(OUTPUT_FORMAT        GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)

if (CONFIG_CODE_DATA_RELOCATION)
  set(CODE_RELOCATION_DEP code_relocation_source_lib)
endif() # CONFIG_CODE_DATA_RELOCATION

configure_linker_script(
  linker.cmd
  ""
  ${PRIV_STACK_DEP}
  ${APP_SMEM_ALIGNED_DEP}
  ${CODE_RELOCATION_DEP}
  ${OFFSETS_H_TARGET}
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
  set(obj_copy_cmd "")
  bintools_objcopy(
    RESULT_CMD_LIST obj_copy_cmd
    TARGET_INPUT    ${OUTPUT_FORMAT}
    TARGET_OUTPUT   "binary"
    SECTION_ONLY    ".intList"
    FILE_INPUT      $<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
    FILE_OUTPUT     "isrList.bin"
    )
  add_custom_command(
    OUTPUT isr_tables.c
    ${obj_copy_cmd}
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/arch/common/gen_isr_tables.py
    --output-source isr_tables.c
    --kernel $<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
    --intlist isrList.bin
    $<$<BOOL:${CONFIG_BIG_ENDIAN}>:--big-endian>
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--debug>
    ${GEN_ISR_TABLE_EXTRA_ARG}
    DEPENDS ${ZEPHYR_PREBUILT_EXECUTABLE}
    )
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES isr_tables.c)
endif()

if(CONFIG_CODE_DATA_RELOCATION)
  # @Intent: Linker script to relocate .text, data and .bss sections
  toolchain_ld_relocation()
endif()

if(CONFIG_USERSPACE)
  zephyr_get_compile_options_for_lang_as_string(C compiler_flags_priv)
  string(REPLACE "-ftest-coverage" "" NO_COVERAGE_FLAGS "${compiler_flags_priv}")
  string(REPLACE "-fprofile-arcs" "" NO_COVERAGE_FLAGS "${NO_COVERAGE_FLAGS}")
  string(REPLACE "-fno-inline" "" NO_COVERAGE_FLAGS "${NO_COVERAGE_FLAGS}")

  get_property(include_dir_in_interface TARGET zephyr_interface
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES)

  get_property(sys_include_dir_in_interface TARGET zephyr_interface
    PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)

  get_property(compile_definitions_interface TARGET zephyr_interface
    PROPERTY INTERFACE_COMPILE_DEFINITIONS)
endif()


# Warning most of this gperf code is duplicated below for
# gen_kobject_list.py / output_lib
if(CONFIG_ARM AND CONFIG_USERSPACE)
  set(GEN_PRIV_STACKS $ENV{ZEPHYR_BASE}/scripts/gen_priv_stacks.py)
  set(PROCESS_PRIV_STACKS_GPERF $ENV{ZEPHYR_BASE}/scripts/process_gperf.py)

  set(PRIV_STACKS                    priv_stacks_hash.gperf)
  set(PRIV_STACKS_OUTPUT_SRC_PRE     priv_stacks_hash_preprocessed.c)
  set(PRIV_STACKS_OUTPUT_SRC         priv_stacks_hash.c)
  set(PRIV_STACKS_OUTPUT_OBJ         priv_stacks_hash.c.obj)
  set(PRIV_STACKS_OUTPUT_OBJ_RENAMED priv_stacks_hash_renamed.o)

  # Essentially what we are doing here is extracting some information
  # out of the nearly finished elf file, generating the source code
  # for a hash table based on that information, and then compiling and
  # linking the hash table back into a now even more nearly finished
  # elf file.

  # Use the script GEN_PRIV_STACKS to scan the kernel binary's
  # (${ZEPHYR_PREBUILT_EXECUTABLE}) DWARF information to produce a table of kernel
  # objects (PRIV_STACKS) which we will then pass to gperf
  add_custom_command(
    OUTPUT ${PRIV_STACKS}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${GEN_PRIV_STACKS}
    --kernel $<TARGET_FILE:priv_stacks_prebuilt>
    --output ${PRIV_STACKS}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS priv_stacks_prebuilt
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(priv_stacks DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS})

  # Use gperf to generate C code (PRIV_STACKS_OUTPUT_SRC_PRE) which implements a
  # perfect hashtable based on PRIV_STACKS
  add_custom_command(
    OUTPUT ${PRIV_STACKS_OUTPUT_SRC_PRE}
    COMMAND
    ${GPERF} -C
    --output-file ${PRIV_STACKS_OUTPUT_SRC_PRE}
    ${PRIV_STACKS}
    DEPENDS priv_stacks ${PRIV_STACKS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(priv_stacks_output_src_pre DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_SRC_PRE})

  # For our purposes the code/data generated by gperf is not optimal.
  #
  # The script PROCESS_GPERF creates a new c file OUTPUT_SRC based on
  # OUTPUT_SRC_PRE to greatly reduce the amount of code/data generated
  # since we know we are always working with pointer values
  add_custom_command(
    OUTPUT ${PRIV_STACKS_OUTPUT_SRC}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${PROCESS_PRIV_STACKS_GPERF}
    -i ${PRIV_STACKS_OUTPUT_SRC_PRE}
    -o ${PRIV_STACKS_OUTPUT_SRC}
    -p "struct _k_priv_stack_map"
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS priv_stacks_output_src_pre ${PRIV_STACKS_OUTPUT_SRC_PRE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(priv_stacks_output_src DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_SRC})

  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_SRC}
    PROPERTIES COMPILE_DEFINITIONS "${compile_definitions_interface}")

  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_SRC}
    PROPERTIES COMPILE_FLAGS
    "${NO_COVERAGE_FLAGS} -fno-function-sections -fno-data-sections ")

  # We need precise control of where generated text/data ends up in the final
  # kernel image. Disable function/data sections and use objcopy to move
  # generated data into special section names
  add_library(priv_stacks_output_lib STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_SRC}
    )

  # Turn off -ffunction-sections, etc.
  # NB: Using a library instead of target_compile_options(priv_stacks_output_lib
  # [...]) because a library's options have precedence
  add_library(priv_stacks_output_lib_interface INTERFACE)
  foreach(incl ${include_dir_in_interface})
    target_include_directories(priv_stacks_output_lib_interface INTERFACE ${incl})
  endforeach()

  foreach(incl ${sys_include_dir_in_interface})
    target_include_directories(priv_stacks_output_lib_interface SYSTEM INTERFACE ${incl})
  endforeach()

  target_link_libraries(priv_stacks_output_lib priv_stacks_output_lib_interface)

  set(PRIV_STACKS_OUTPUT_OBJ_PATH ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/priv_stacks_output_lib.dir/${PRIV_STACKS_OUTPUT_OBJ})

  set(obj_copy_cmd "")
  set(obj_copy_sections_rename
    .bss=.priv_stacks.noinit
    .data=.priv_stacks.data
    .text=.priv_stacks.text
    .rodata=.priv_stacks.rodata
    )
  bintools_objcopy(
    RESULT_CMD_LIST obj_copy_cmd
    SECTION_RENAME  ${obj_copy_sections_rename}
    FILE_INPUT      ${PRIV_STACKS_OUTPUT_OBJ_PATH}
    FILE_OUTPUT     ${PRIV_STACKS_OUTPUT_OBJ_RENAMED}
    )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_OBJ_RENAMED}
    ${obj_copy_cmd}
    DEPENDS priv_stacks_output_lib
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(priv_stacks_output_obj_renamed DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_OBJ_RENAMED})

  add_library(priv_stacks_output_obj_renamed_lib STATIC IMPORTED GLOBAL)
  set_property(
    TARGET priv_stacks_output_obj_renamed_lib
    PROPERTY
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_OBJ_RENAMED}
    )
  add_dependencies(
    priv_stacks_output_obj_renamed_lib
    priv_stacks_output_obj_renamed
    )

  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_OBJECT_FILES priv_stacks_output_obj_renamed_lib)
endif()

# Warning: most of this gperf code is duplicated above for
# gen_priv_stacks.py / priv_stacks_output_lib
if(CONFIG_USERSPACE)
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
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS ${ZEPHYR_PREBUILT_EXECUTABLE}
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
    -p "struct _k_object"
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

  set(obj_copy_cmd "")
  set(obj_copy_sections_rename
    .data=.kobject_data.data
    .text=.kobject_data.text
    .rodata=.kobject_data.rodata
    )
  bintools_objcopy(
    RESULT_CMD_LIST obj_copy_cmd
    SECTION_RENAME  ${obj_copy_sections_rename}
    FILE_INPUT      ${OUTPUT_OBJ_PATH}
    FILE_OUTPUT     ${OUTPUT_OBJ_RENAMED}
    )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_OBJ_RENAMED}
    ${obj_copy_cmd}
    DEPENDS output_lib
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
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
toolchain_cc_cstd_flag(CC_CSTD ${CSTD})
zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:C>:${CC_CSTD}>
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
  if(CONFIG_MBEDTLS)
    set(MBEDTLS_PART -l lib..__modules__crypto__mbedtls.a k_mbedtls_partition)
  endif()

  add_custom_command(
    OUTPUT ${APP_SMEM_UNALIGNED_LD}
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/gen_app_partitions.py
    -d ${OBJ_FILE_DIR}
    -o ${APP_SMEM_UNALIGNED_LD}
    ${NEWLIB_PART} ${MBEDTLS_PART}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    kernel
    ${ZEPHYR_LIBS_PROPERTY}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/
    COMMENT "Generating app_smem_unaligned linker section"
    )

  configure_linker_script(
    linker_app_smem_unaligned.cmd
    "-DLINKER_APP_SMEM_UNALIGNED"
    ${CODE_RELOCATION_DEP}
    ${APP_SMEM_UNALIGNED_DEP}
    ${APP_SMEM_UNALIGNED_LD}
    ${OFFSETS_H_TARGET}
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
  set_property(TARGET   app_smem_unaligned_prebuilt PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_app_smem_unaligned.cmd)
  add_dependencies(     app_smem_unaligned_prebuilt linker_app_smem_unaligned_script ${OFFSETS_LIB})

  add_custom_command(
    OUTPUT ${APP_SMEM_ALIGNED_LD}
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/gen_app_partitions.py
    -e $<TARGET_FILE:app_smem_unaligned_prebuilt>
    -o ${APP_SMEM_ALIGNED_LD}
    ${NEWLIB_PART} ${MBEDTLS_PART}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS
    kernel
    ${ZEPHYR_LIBS_PROPERTY}
    app_smem_unaligned_prebuilt
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/
    COMMENT "Generating app_smem_aligned linker section"
    )
endif()

if(CONFIG_USERSPACE AND CONFIG_ARM)
  configure_linker_script(
    linker_priv_stacks.cmd
    ""
    ${CODE_RELOCATION_DEP}
    ${APP_SMEM_ALIGNED_DEP}
    ${APP_SMEM_ALIGNED_LD}
    ${OFFSETS_H_TARGET}
    )

  add_custom_target(
    linker_priv_stacks_script
    DEPENDS
    linker_priv_stacks.cmd
    )

  set_property(TARGET
    linker_priv_stacks_script
    PROPERTY INCLUDE_DIRECTORIES
    ${ZEPHYR_INCLUDE_DIRS}
    )

  set(PRIV_STACK_LIB priv_stacks_output_obj_renamed_lib)
  add_executable(       priv_stacks_prebuilt misc/empty_file.c)
  toolchain_ld_link_elf(
    TARGET_ELF            priv_stacks_prebuilt
    OUTPUT_MAP            ${PROJECT_BINARY_DIR}/priv_stacks_prebuilt.map
    LIBRARIES_PRE_SCRIPT  ""
    LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/linker_priv_stacks.cmd
    LIBRARIES_POST_SCRIPT ""
    DEPENDENCIES          ${CODE_RELOCATION_DEP}
  )
  set_property(TARGET   priv_stacks_prebuilt PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_priv_stacks.cmd)
  add_dependencies(     priv_stacks_prebuilt linker_priv_stacks_script ${OFFSETS_LIB})
endif()

# FIXME: Is there any way to get rid of empty_file.c?
add_executable(       ${ZEPHYR_PREBUILT_EXECUTABLE} misc/empty_file.c)
toolchain_ld_link_elf(
  TARGET_ELF            ${ZEPHYR_PREBUILT_EXECUTABLE}
  OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${ZEPHYR_PREBUILT_EXECUTABLE}.map
  LIBRARIES_PRE_SCRIPT  ""
  LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/linker.cmd
  LIBRARIES_POST_SCRIPT ${PRIV_STACK_LIB}
  DEPENDENCIES          ${CODE_RELOCATION_DEP}
)
set_property(TARGET   ${ZEPHYR_PREBUILT_EXECUTABLE} PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.cmd)
add_dependencies(     ${ZEPHYR_PREBUILT_EXECUTABLE} ${PRIV_STACK_DEP} ${LINKER_SCRIPT_TARGET} ${OFFSETS_LIB})


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
    ${PRIV_STACK_DEP}
    ${CODE_RELOCATION_DEP}
    ${ZEPHYR_PREBUILT_EXECUTABLE}
    ${OFFSETS_H_TARGET}
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
  set_property(TARGET   ${ZEPHYR_FINAL_EXECUTABLE} PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_pass_final.cmd)
  add_dependencies(     ${ZEPHYR_FINAL_EXECUTABLE} ${PRIV_STACK_DEP} ${LINKER_PASS_FINAL_SCRIPT_TARGET})

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
  cmake -E rename ${logical_target_for_zephyr_elf}.map ${KERNEL_MAP_NAME}
)

if(NOT CONFIG_BUILD_NO_GAP_FILL)
  # Use ';' as separator to get proper space in resulting command.
  set(GAP_FILL "0xff")
endif()

if(CONFIG_OUTPUT_PRINT_MEMORY_USAGE)
  # @Intent: Use the toolchain bintools method for printing memory usage
  set(memUsageCmd    "")
  set(memUsageByProd "")
  bintools_print_mem_usage(
    RESULT_CMD_LIST    memUsageCmd
    RESULT_BYPROD_LIST memUsageByProd
  )
  list(APPEND
    post_build_commands
    ${memUsageCmd}
    )
  list(APPEND
    post_build_byproducts
    ${memUsageByProd}
    )
endif()

if(CONFIG_BUILD_OUTPUT_HEX OR BOARD_FLASH_RUNNER STREQUAL openocd)
  set(out_hex_cmd    "")
  set(out_hex_byprod "")
  set(out_hex_sections_remove
    .comment
    COMMON
    .eh_frame
    )
  bintools_objcopy(
    RESULT_CMD_LIST    out_hex_cmd
    RESULT_BYPROD_LIST out_hex_byprod
    STRIP_ALL
    GAP_FILL           ${GAP_FILL}
    TARGET_OUTPUT      "ihex"
    SECTION_REMOVE     ${out_hex_sections_remove}
    FILE_INPUT         ${KERNEL_ELF_NAME}
    FILE_OUTPUT        ${KERNEL_HEX_NAME}
    )
  list(APPEND
    post_build_commands
    ${out_hex_cmd}
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_HEX_NAME}
    ${out_hex_byprod}
    )
endif()

if(CONFIG_BUILD_OUTPUT_BIN)
  set(out_bin_cmd    "")
  set(out_bin_byprod "")
  set(out_bin_sections_remove
    .comment
    COMMON
    .eh_frame
    )
  bintools_objcopy(
    RESULT_CMD_LIST    out_bin_cmd
    RESULT_BYPROD_LIST out_bin_byprod
    STRIP_ALL
    GAP_FILL           ${GAP_FILL}
    TARGET_OUTPUT      "binary"
    SECTION_REMOVE     ${out_bin_sections_remove}
    FILE_INPUT         ${KERNEL_ELF_NAME}
    FILE_OUTPUT        ${KERNEL_BIN_NAME}
    )
  list(APPEND
    post_build_commands
    ${out_bin_cmd}
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_BIN_NAME}
    ${out_bin_byprod}
    )
endif()

if(CONFIG_BUILD_OUTPUT_S19)
  set(out_S19_cmd    "")
  set(out_S19_byprod "")
  bintools_objcopy(
    RESULT_CMD_LIST    out_S19_cmd
    RESULT_BYPROD_LIST out_S19_byprod
    GAP_FILL           ${GAP_FILL}
    TARGET_OUTPUT      "srec"
    SREC_LEN           1
    FILE_INPUT         ${KERNEL_ELF_NAME}
    FILE_OUTPUT        ${KERNEL_S19_NAME}
    )
  list(APPEND
    post_build_commands
    ${out_S19_cmd}
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_S19_NAME}
    ${out_S19_byprod}
    )
endif()

if(CONFIG_OUTPUT_DISASSEMBLY)
  set(out_disassembly_cmd    "")
  set(out_disassembly_byprod "")
  bintools_objdump(
    RESULT_CMD_LIST    out_disassembly_cmd
    RESULT_BYPROD_LIST out_disassembly_byprod
    DISASSEMBLE_SOURCE
    FILE_INPUT         ${KERNEL_ELF_NAME}
    FILE_OUTPUT        ${KERNEL_LST_NAME}
    )
  list(APPEND
    post_build_commands
    ${out_disassembly_cmd}
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_LST_NAME}
    ${out_disassembly_byprod}
    )
endif()

if(CONFIG_OUTPUT_STAT)
  set(out_stat_cmd    "")
  set(out_stat_byprod "")
  bintools_readelf(
    RESULT_CMD_LIST    out_stat_cmd
    RESULT_BYPROD_LIST out_stat_byprod
    HEADERS
    FILE_INPUT         ${KERNEL_ELF_NAME}
    FILE_OUTPUT        ${KERNEL_STAT_NAME}
    )
  list(APPEND
    post_build_commands
    ${out_stat_cmd}
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_STAT_NAME}
    ${out_stat_byprod}
    )
endif()

if(CONFIG_BUILD_OUTPUT_STRIPPED)
  set(out_stripped_cmd    "")
  set(out_stripped_byprod "")
  bintools_strip(
    RESULT_CMD_LIST    out_stripped_cmd
    RESULT_BYPROD_LIST out_stripped_byprod
    STRIP_ALL
    FILE_INPUT         ${KERNEL_ELF_NAME}
    FILE_OUTPUT        ${KERNEL_STRIP_NAME}
    )
  list(APPEND
    post_build_commands
    ${out_stripped_cmd}
    )
  list(APPEND
    post_build_byproducts
    ${KERNEL_STRIP_NAME}
    ${out_stripped_byprod}
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
  list(APPEND FLASH_DEPS mergehex)
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
  message(WARNING "
      ------------------------------------------------------------
      --- WARNING:  __ASSERT() statements are globally ENABLED ---
      --- The kernel will run more slowly and use more memory  ---
      ------------------------------------------------------------"
  )
endif()
endif()

if(CONFIG_BOARD_DEPRECATED)
  message(WARNING "
      WARNING:  The board '${BOARD}' is deprecated and will be
      removed in version ${CONFIG_BOARD_DEPRECATED}"
  )
endif()

# @Intent: Set compiler specific flags for standard C includes
# Done at the very end, so any other system includes which may
# be added by Zephyr components were first in list.
toolchain_cc_nostdinc()
