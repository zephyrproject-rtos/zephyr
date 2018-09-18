if(NOT DEFINED ZEPHYR_BINARY_DIR)
  message(FATAL_ERROR "A user error has occured.
cmake was invoked with '${CMAKE_CURRENT_LIST_DIR}' specified as the source directory,
but it must be invoked with an application source directory,
such as '${CMAKE_CURRENT_LIST_DIR}/samples/hello_world'.
Debug variables:
CMAKE_CACHEFILE_DIR: ${CMAKE_CACHEFILE_DIR}
")
endif()

project(Zephyr-Kernel VERSION ${PROJECT_VERSION})
enable_language(C CXX ASM)

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

# Verify that the toolchain can compile a dummy file, if it is not we
# won't be able to test for compatiblity with certain C flags.
check_c_compiler_flag("" toolchain_is_ok)
assert(toolchain_is_ok "The toolchain is unable to build a dummy C file. See CMakeError.log.")

set(CMAKE_EXECUTABLE_SUFFIX .elf)

if(NOT PROPERTY_LINKER_SCRIPT_DEFINES)
  set_property(GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__GCC_LINKER_CMD__)
endif()

define_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT BRIEF_DOCS " " FULL_DOCS " ")
set_property(   GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-little${ARCH})

# zephyr_interface is a source-less library that has all the global
# compiler options needed by all source files. All zephyr libraries,
# including the library named "zephyr" link with this library to
# obtain these flags.
add_library(zephyr_interface INTERFACE)

# zephyr is a catchall CMake library for source files that can be
# built purely with the include paths, defines, and other compiler
# flags that come with zephyr_interface.
zephyr_library_named(zephyr)

zephyr_include_directories(
  kernel/include
  arch/${ARCH}/include
  ${SOC_DIR}/${ARCH}/${SOC_PATH}
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/include
  ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/include
  ${BOARD_DIR}
  include
  include/drivers
  ${PROJECT_BINARY_DIR}/include/generated
  ${USERINCLUDE}
  ${STDINCLUDE}
)

zephyr_compile_definitions(
  KERNEL
  __ZEPHYR__=1
)

if(NOT CONFIG_NO_OPTIMIZATIONS)
zephyr_compile_definitions(
  _FORTIFY_SOURCE=2
)
endif()

if(BUILD_VERSION)
  zephyr_compile_definitions(
    BUILD_VERSION=${BUILD_VERSION}
  )
endif()

# We need to set an optimization level.
# Default to -Os
# unless CONFIG_NO_OPTIMIZATIONS is set, then it is -O0
# or unless CONFIG_DEBUG is set, then it is -Og
#
# also, some toolchain's break with -Os, and some toolchain's break
# with -Og so allow them to override what flag to use
#
# Finally, the user can use Kconfig to add compiler options that will
# come after these options and override them
set_ifndef(OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG "-O0")
set_ifndef(OPTIMIZE_FOR_DEBUG_FLAG            "-Og")
set_ifndef(OPTIMIZE_FOR_SIZE_FLAG             "-Os")
set_ifndef(OPTIMIZE_FOR_SPEED_FLAG            "-O2")

if(CONFIG_NO_OPTIMIZATIONS)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG})
elseif(CONFIG_DEBUG_OPTIMIZATIONS)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_DEBUG_FLAG})
elseif(CONFIG_SPEED_OPTIMIZATIONS)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SPEED_FLAG})
elseif(CONFIG_SIZE_OPTIMIZATIONS)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SIZE_FLAG}) # Default
else()
  assert(0 "Unreachable code. Expected optimization level to have been chosen. See misc/Kconfig.")
endif()

zephyr_compile_options(
  ${OPTIMIZATION_FLAG} # Usually -Os
  -g # TODO: build configuration enough?
  -Wall
  -Wformat
  -Wformat-security
  -Wno-format-zero-length
  -imacros ${AUTOCONF_H}
  -ffreestanding
  -Wno-main
  ${NOSTDINC_F}
  ${TOOLCHAIN_C_FLAGS}
)

zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
  $<$<COMPILE_LANGUAGE:CXX>:-fcheck-new>
  $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>
  $<$<COMPILE_LANGUAGE:CXX>:-fdata-sections>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>

  $<$<COMPILE_LANGUAGE:ASM>:-xassembler-with-cpp>
  $<$<COMPILE_LANGUAGE:ASM>:-D_ASMLANGUAGE>
)

if(NOT CONFIG_NATIVE_APPLICATION)
zephyr_ld_options(
  -nostartfiles
  -nodefaultlibs
  -nostdlib
  -static
  -no-pie
)
endif()

# ==========================================================================
#
# cmake -DW=... settings
#
# W=1 - warnings that may be relevant and does not occur too often
# W=2 - warnings that occur quite often but may still be relevant
# W=3 - the more obscure warnings, can most likely be ignored
# ==========================================================================
if(W MATCHES "1")
  zephyr_compile_options(
    -Wextra
    -Wunused
    -Wno-unused-parameter
    -Wmissing-declarations
    -Wmissing-format-attribute
    -Wold-style-definition
    )
  zephyr_cc_option(
    -Wmissing-prototypes
    -Wmissing-include-dirs
    -Wunused-but-set-variable
    -Wno-missing-field-initializers
    )
endif()

if(W MATCHES "2")
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
endif()

if(W MATCHES "3")
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
endif()

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'
include(cmake/extra_flags.cmake)

if(CONFIG_READABLE_ASM)
  zephyr_cc_option(-fno-reorder-blocks)
  zephyr_cc_option(-fno-ipa-cp-clone)
  zephyr_cc_option(-fno-partial-inlining)
endif()

zephyr_cc_option(-fno-asynchronous-unwind-tables)
zephyr_cc_option(-fno-pie)
zephyr_cc_option(-fno-pic)
zephyr_cc_option(-fno-strict-overflow)
zephyr_cc_option(-Wno-pointer-sign)

zephyr_compile_options_ifdef(CONFIG_STACK_CANARIES -fstack-protector-all)

if(CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT)
  if(CONFIG_OMIT_FRAME_POINTER)
    zephyr_cc_option(-fomit-frame-pointer)
  else()
    zephyr_cc_option(-fno-omit-frame-pointer)
  endif()
endif()

separate_arguments(CONFIG_COMPILER_OPT_AS_LIST UNIX_COMMAND ${CONFIG_COMPILER_OPT})
zephyr_compile_options(${CONFIG_COMPILER_OPT_AS_LIST})

# TODO: Include arch compiler options at this point.

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  zephyr_cc_option(
    #FIXME: need to fix all of those
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
else() # GCC assumed
  zephyr_cc_option(
    -Wno-unused-but-set-variable
    -fno-reorder-functions
    )

  if(NOT ${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "xcc")
    zephyr_cc_option(-fno-defer-pop)
  endif()
endif()

zephyr_cc_option_ifdef(CONFIG_STACK_USAGE            -fstack-usage)

zephyr_system_include_directories(${NOSTDINC})

# Force an error when things like SYS_INIT(foo, ...) occur with a missing header.
zephyr_cc_option(-Werror=implicit-int)

# Prohibit date/time macros, which would make the build non-deterministic
# cc-option(-Werror=date-time)

# TODO: Archiver arguments
# ar_option(D)

set_ifndef(LINKERFLAGPREFIX -Wl)

if(NOT CONFIG_NATIVE_APPLICATION)
zephyr_ld_options(
  ${LINKERFLAGPREFIX},-X
  ${LINKERFLAGPREFIX},-N
  )
endif()

zephyr_ld_options(
  ${LINKERFLAGPREFIX},--gc-sections
  ${LINKERFLAGPREFIX},--build-id=none
  )

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
add_subdirectory(misc)
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
add_subdirectory(tests)

set(syscall_macros_h ${ZEPHYR_BINARY_DIR}/include/generated/syscall_macros.h)

add_custom_target(syscall_macros_h_target DEPENDS ${syscall_macros_h})
add_custom_command(                       OUTPUT  ${syscall_macros_h}
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
file(STRINGS ${syscalls_subdirs_txt} PARSE_SYSCALLS_PATHS_DEPENDS)

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

add_custom_target(syscall_list_h_target DEPENDS                ${syscall_list_h})
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
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
add_custom_target(driver_validation_h_target DEPENDS ${DRV_VALIDATION})

include($ENV{ZEPHYR_BASE}/cmake/kobj.cmake)
gen_kobj(KOBJ_INCLUDE_PATH)

# Generate offsets.c.obj from offsets.c
# Generate offsets.h     from offsets.c.obj

set(OFFSETS_C_PATH ${ZEPHYR_BASE}/arch/${ARCH}/core/offsets/offsets.c)
set(OFFSETS_O_PATH ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/offsets.dir/arch/${ARCH}/core/offsets/offsets.c.obj)
set(OFFSETS_H_PATH ${PROJECT_BINARY_DIR}/include/generated/offsets.h)

add_library(          offsets STATIC ${OFFSETS_C_PATH})
target_link_libraries(offsets zephyr_interface)
add_dependencies(     offsets
  syscall_list_h_target
  syscall_macros_h_target
  driver_validation_h_target
  kobj_types_h_target
  )

add_custom_command(
  OUTPUT ${OFFSETS_H_PATH}
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/gen_offset_header.py
  -i ${OFFSETS_O_PATH}
  -o ${OFFSETS_H_PATH}
  DEPENDS offsets
)
add_custom_target(offsets_h DEPENDS ${OFFSETS_H_PATH})

zephyr_include_directories(${TOOLCHAIN_INCLUDES})

zephyr_get_include_directories_for_lang(C ZEPHYR_INCLUDES)

add_subdirectory(kernel)

# Read list content
get_property(ZEPHYR_LIBS_PROPERTY GLOBAL PROPERTY ZEPHYR_LIBS)

foreach(zephyr_lib ${ZEPHYR_LIBS_PROPERTY})
  # TODO: Could this become an INTERFACE property of zephyr_interface?
  add_dependencies(${zephyr_lib} offsets_h)

  # Verify that all (non-imported) libraries have source
  # files. Libraries without source files are not supported because
  # they are an indication that something has been misconfigured.
  get_target_property(lib_imported ${zephyr_lib} IMPORTED)
  get_target_property(lib_sources  ${zephyr_lib} SOURCES)
  if(lib_sources STREQUAL lib_sources-NOTFOUND
      AND (NOT (${zephyr_lib} STREQUAL app))
      AND (NOT lib_imported)
      )
    # app is not checked because it's sources are added to it after
    # this CMakeLists.txt file has been processed
    message(FATAL_ERROR "\
The Zephyr library '${zephyr_lib}' was created without source files. \
Empty (non-imported) libraries are not supported. \
Either make sure that the library has the sources it should have, \
or make sure it is not created when it has no source files.")
  endif()
endforeach()

get_property(OUTPUT_FORMAT        GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)

get_property(LINKER_SCRIPT_DEFINES GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES)

if(CONFIG_APPLICATION_MEMORY)
  # Objects default to being in kernel space, and then we exclude
  # certain items.
  set(kernel_object_file_list
    ${ZEPHYR_LIBS_PROPERTY}
    kernel
    )
  list(
    REMOVE_ITEM
    kernel_object_file_list
    app
    )

  # The zephyr libraries in zephyr/lib/ and zephyr/test/ belong in
  # userspace.

  # NB: The business logic for determing what source files are in
  # kernel space and what source files are in user space is
  # fragile. Fix ASAP.
  #
  # The intended design is that certain directories are designated as
  # containing userspace code and others for kernel space code. The
  # implementation we have however is not working on directories of
  # code, it is working on zephyr libraries. It is exploiting the fact
  # that zephyr libraries follow a naming scheme as described in
  # extensions.cmake:zephyr_library_get_current_dir_lib_name
  #
  # But code from test/ and lib/ that is placed in the "zephyr"
  # library (with zephyr_sources()) will not be in a library that is
  # prefixed with lib__ or test__ and will end up in the wrong address
  # space.
  set(application_space_dirs
    lib
    tests
    )
  foreach(f ${kernel_object_file_list})
    foreach(app_dir ${application_space_dirs})
      if(${f} MATCHES "^${app_dir}__") # Begins with ${app_dir}__, e.g. lib__libc
        list(
          REMOVE_ITEM
          kernel_object_file_list
          ${f}
          )
      endif()
    endforeach()
  endforeach()

  # Create a list ks, with relative paths to kernel space libs.
  foreach(f ${kernel_object_file_list})
    get_target_property(target_name       ${f} NAME)
    get_target_property(target_binary_dir ${f} BINARY_DIR)

    string(REPLACE
      ${PROJECT_BINARY_DIR}
      ""
      fixed_path
      ${target_binary_dir}
      )

    # Append / if not empty
    if(fixed_path)
      set(fixed_path "${fixed_path}/")
    endif()

    # Cut off leading / if present
    if(fixed_path MATCHES "^/.+")
      string(SUBSTRING ${fixed_path} 1 -1 fixed_path)
    endif()

    set(fixed_path "${fixed_path}lib${target_name}.a")

    if(CMAKE_GENERATOR STREQUAL "Ninja")
      # Ninja invokes the linker from the root of the build directory
      # (APPLICATION_BINARY_DIR) instead of from the build/zephyr
      # directory (PROJECT_BINARY_DIR). So for linker-defs.h to get
      # the correct path we need to prefix with zephyr/.
      set(fixed_path "zephyr/${fixed_path}")
    endif()

    list(APPEND ks ${fixed_path})
  endforeach()

  # We are done constructing kernel_object_file_list, now we inject
  # this list into the linker script through the define
  # KERNELSPACE_OBJECT_FILES
  set(def -DKERNELSPACE_OBJECT_FILES=)
  foreach(f ${ks})
    set(def "${def} ${f}")
  endforeach()
  list(APPEND LINKER_SCRIPT_DEFINES ${def})
endif() # CONFIG_APPLICATION_MEMORY

# Declare MPU userspace dependencies before the linker scripts to make
# sure the order of dependencies are met
if(CONFIG_CPU_HAS_MPU AND CONFIG_USERSPACE)
	if(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT AND CONFIG_APP_SHARED_MEM )
    set(APP_SMEM_DEP app_smem_linker)
  endif()
  if(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT AND CONFIG_APPLICATION_MEMORY)
    set(ALIGN_SIZING_DEP app_sizing_prebuilt linker_app_sizing_script)
  endif()
  if(CONFIG_ARM)
  set(PRIV_STACK_DEP priv_stacks_prebuilt)
  endif()
endif()

function(construct_add_custom_command_for_linker_pass linker_output_name output_variable)
  set(linker_cmd_file_name ${linker_output_name}.cmd)

  if (${linker_output_name} MATCHES "^linker_pass_final$")
    set(LINKER_PASS_DEFINE -DLINKER_PASS2)
  else()
    set(LINKER_PASS_DEFINE "")
  endif()

  # Different generators deal with depfiles differently.
  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    # Note that the IMPLICIT_DEPENDS option is currently supported only
    # for Makefile generators and will be ignored by other generators.
    set(LINKER_SCRIPT_DEP IMPLICIT_DEPENDS C ${LINKER_SCRIPT})
  elseif(CMAKE_GENERATOR STREQUAL "Ninja")
    # Using DEPFILE with other generators than Ninja is an error.
    set(LINKER_SCRIPT_DEP DEPFILE ${PROJECT_BINARY_DIR}/${linker_cmd_file_name}.dep)
  else()
    # TODO: How would the linker script dependencies work for non-linker
    # script generators.
    message(STATUS "Warning; this generator is not well supported. The
  Linker script may not be regenerated when it should.")
    set(LINKER_SCRIPT_DEP "")
  endif()

  set(${output_variable}
    OUTPUT ${linker_cmd_file_name}
    DEPENDS ${LINKER_SCRIPT}
    ${LINKER_SCRIPT_DEP}
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    ${NOSTDINC_F}
    -undef
    -MD -MF ${linker_cmd_file_name}.dep -MT ${BASE_NAME}/${linker_cmd_file_name}
    ${ZEPHYR_INCLUDES}
    ${LINKER_SCRIPT_DEFINES}
    ${LINKER_PASS_DEFINE}
    -E ${LINKER_SCRIPT}
    -P # Prevent generation of debug `#line' directives.
    -o ${linker_cmd_file_name}
    VERBATIM
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}

    PARENT_SCOPE
    )
endfunction()

get_filename_component(BASE_NAME ${CMAKE_CURRENT_BINARY_DIR} NAME)
construct_add_custom_command_for_linker_pass(linker custom_command)
add_custom_command(
  ${custom_command}
)

add_custom_target(
  linker_script
  DEPENDS
  ${ALIGN_SIZING_DEP} ${PRIV_STACK_DEP}
  ${APP_SMEM_DEP}
  linker.cmd
  offsets_h
  )

# Give the 'linker_script' target all of the include directories so
# that cmake can successfully find the linker_script's header
# dependencies.
zephyr_get_include_directories_for_lang(C
  ZEPHYR_INCLUDE_DIRS
  STRIP_PREFIX # Don't use a -I prefix
  )
set_property(TARGET
  linker_script
  PROPERTY INCLUDE_DIRECTORIES
  ${ZEPHYR_INCLUDE_DIRS}
  )

set(zephyr_lnk
  ${LINKERFLAGPREFIX},-Map=${PROJECT_BINARY_DIR}/${KERNEL_MAP_NAME}
  -u_OffsetAbsSyms
  -u_ConfigAbsSyms
  ${LINKERFLAGPREFIX},--whole-archive
  ${ZEPHYR_LIBS_PROPERTY}
  ${LINKERFLAGPREFIX},--no-whole-archive
  kernel
  ${OFFSETS_O_PATH}
  ${LIB_INCLUDE_DIR}
  -L${PROJECT_BINARY_DIR}
  ${TOOLCHAIN_LIBS}
  )

if(CONFIG_GEN_ISR_TABLES)
  if(CONFIG_GEN_SW_ISR_TABLE)
    list(APPEND GEN_ISR_TABLE_EXTRA_ARG --sw-isr-table)
  endif()

  if(CONFIG_GEN_IRQ_VECTOR_TABLE)
    list(APPEND GEN_ISR_TABLE_EXTRA_ARG --vector-table)
  endif()

  # isr_tables.c is generated from zephyr_prebuilt by
  # gen_isr_tables.py
  add_custom_command(
    OUTPUT isr_tables.c
    COMMAND ${CMAKE_OBJCOPY}
    -I ${OUTPUT_FORMAT}
    -O binary
    --only-section=.intList
    $<TARGET_FILE:zephyr_prebuilt>
    isrList.bin
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/arch/common/gen_isr_tables.py
    --output-source isr_tables.c
    --kernel $<TARGET_FILE:zephyr_prebuilt>
    --intlist isrList.bin
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--debug>
    ${GEN_ISR_TABLE_EXTRA_ARG}
    DEPENDS zephyr_prebuilt
    )
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES isr_tables.c)
endif()

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
  # (zephyr_prebuilt) DWARF information to produce a table of kernel
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

  # We need precise control of where generated text/data ends up in the final
  # kernel image. Disable function/data sections and use objcopy to move
  # generated data into special section names
  add_library(priv_stacks_output_lib STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_SRC}
    )

  target_link_libraries(priv_stacks_output_lib zephyr_interface)

  # Turn off -ffunction-sections, etc.
  # NB: Using a library instead of target_compile_options(priv_stacks_output_lib
  # [...]) because a library's options have precedence
  add_library(priv_stacks_output_lib_interface INTERFACE)
  target_compile_options(priv_stacks_output_lib_interface INTERFACE
    -fno-function-sections
    -fno-data-sections
    )
  target_link_libraries(priv_stacks_output_lib priv_stacks_output_lib_interface)

  set(PRIV_STACKS_OUTPUT_OBJ_PATH ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/priv_stacks_output_lib.dir/${PRIV_STACKS_OUTPUT_OBJ})

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PRIV_STACKS_OUTPUT_OBJ_RENAMED}
    COMMAND
    ${CMAKE_OBJCOPY}
    --rename-section   .bss=.priv_stacks.noinit
    --rename-section   .data=.priv_stacks.data
    --rename-section   .text=.priv_stacks.text
    --rename-section .rodata=.priv_stacks.rodata
    ${PRIV_STACKS_OUTPUT_OBJ_PATH}
    ${PRIV_STACKS_OUTPUT_OBJ_RENAMED}
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
  # elf file.

  # Use the script GEN_KOBJ_LIST to scan the kernel binary's
  # (zephyr_prebuilt) DWARF information to produce a table of kernel
  # objects (OBJ_LIST) which we will then pass to gperf
  add_custom_command(
    OUTPUT ${OBJ_LIST}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${GEN_KOBJ_LIST}
    --kernel $<TARGET_FILE:zephyr_prebuilt>
    --gperf-output ${OBJ_LIST}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS zephyr_prebuilt
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

  # always compile kobject_hash.c at optimization -Os
  set_source_files_properties(${OUTPUT_SRC} PROPERTIES COMPILE_FLAGS -Os)
  target_link_libraries(output_lib zephyr_interface)

  # Turn off -ffunction-sections, etc.
  # NB: Using a library instead of target_compile_options(output_lib
  # [...]) because a library's options have precedence
  add_library(output_lib_interface INTERFACE)
  target_compile_options(output_lib_interface INTERFACE
    -fno-function-sections
    -fno-data-sections
    )
  target_link_libraries(output_lib output_lib_interface)

  set(OUTPUT_OBJ_PATH ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/output_lib.dir/${OUTPUT_OBJ})

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_OBJ_RENAMED}
    COMMAND
    ${CMAKE_OBJCOPY}
    --rename-section   .data=.kobject_data.data
    --rename-section   .text=.kobject_data.text
    --rename-section .rodata=.kobject_data.rodata
    ${OUTPUT_OBJ_PATH}
    ${OUTPUT_OBJ_RENAMED}
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

get_property(TOPT GLOBAL PROPERTY TOPT)
set_ifndef(  TOPT -T)

get_property(CSTD GLOBAL PROPERTY CSTD)
set_ifndef(CSTD c99)

zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:C>:-std=${CSTD}>
)

configure_file(
     $ENV{ZEPHYR_BASE}/include/arch/arm/cortex_m/scripts/app_data_alignment.ld
     ${PROJECT_BINARY_DIR}/include/generated/app_data_alignment.ld)

configure_file(
     $ENV{ZEPHYR_BASE}/include/arch/arm/cortex_m/scripts/app_smem.ld
     ${PROJECT_BINARY_DIR}/include/generated/app_smem.ld)

if(CONFIG_CPU_HAS_MPU AND CONFIG_USERSPACE)

    if(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT AND CONFIG_APP_SHARED_MEM)
        set(APP_SMEM_LD "${PROJECT_BINARY_DIR}/include/generated/app_smem.ld")
        set(OBJ_FILE_DIR "${PROJECT_BINARY_DIR}/../")

        add_custom_target(
          ${APP_SMEM_DEP} ALL
          DEPENDS app
          DEPENDS ${ALIGN_SIZING_DEP}
         )

        add_custom_command(
          TARGET ${APP_SMEM_DEP}
          COMMAND ${PYTHON_EXECUTABLE}
          ${ZEPHYR_BASE}/scripts/gen_app_partitions.py
          -d ${OBJ_FILE_DIR}
          -o ${APP_SMEM_LD}
          $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
          WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/
          COMMENT "Generating power of 2 aligned app_smem linker section"
         )
  endif()


  if(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT AND CONFIG_APPLICATION_MEMORY)

    construct_add_custom_command_for_linker_pass(linker_app_sizing custom_command)
    add_custom_command(
      ${custom_command}
      )

    add_custom_target(
      linker_app_sizing_script
      DEPENDS
      linker_app_sizing.cmd
      offsets_h
    )

    set_property(TARGET
      linker_app_sizing_script
      PROPERTY INCLUDE_DIRECTORIES
      ${ZEPHYR_INCLUDE_DIRS}
      )

    # For systems with MPUs, the size of the application data section must
    # be determined so that MPU alignment requirements can be met.
    # Create a app_sizing_prebuilt target so we can do this before the
    # other ELF files are built
    set(GEN_APP_ALIGN $ENV{ZEPHYR_BASE}/scripts/gen_alignment_script.py)
    add_executable(       app_sizing_prebuilt misc/empty_file.c)
    target_link_libraries(app_sizing_prebuilt ${TOPT} ${PROJECT_BINARY_DIR}/linker_app_sizing.cmd ${zephyr_lnk})
    set_property(TARGET   app_sizing_prebuilt PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_app_sizing.cmd)
    add_dependencies(     app_sizing_prebuilt linker_app_sizing_script offsets )

    add_custom_command(
      TARGET app_sizing_prebuilt
      POST_BUILD
      COMMAND ${PYTHON_EXECUTABLE} ${GEN_APP_ALIGN}
      --output ./include/generated/app_data_alignment.ld
      --kernel $<TARGET_FILE:app_sizing_prebuilt>
      VERBATIM
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/
   )
  endif()

if(CONFIG_ARM)
  construct_add_custom_command_for_linker_pass(linker_priv_stacks custom_command)
  add_custom_command(
    ${custom_command}
    )

  add_custom_target(
    linker_priv_stacks_script
    DEPENDS
    ${ALIGN_SIZING_DEP} ${APP_SMEM_DEP}
    linker_priv_stacks.cmd
    offsets_h
    )

  set_property(TARGET
    linker_priv_stacks_script
    PROPERTY INCLUDE_DIRECTORIES
    ${ZEPHYR_INCLUDE_DIRS}
    )

  set(PRIV_STACK_LIB priv_stacks_output_obj_renamed_lib)
  add_executable(       priv_stacks_prebuilt misc/empty_file.c)
  target_link_libraries(priv_stacks_prebuilt ${TOPT} ${PROJECT_BINARY_DIR}/linker_priv_stacks.cmd ${zephyr_lnk})
  set_property(TARGET   priv_stacks_prebuilt PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_priv_stacks.cmd)
  add_dependencies(     priv_stacks_prebuilt ${ALIGN_SIZING_DEP} linker_priv_stacks_script offsets)
endif()

endif()

# FIXME: Is there any way to get rid of empty_file.c?
add_executable(       zephyr_prebuilt misc/empty_file.c)
target_link_libraries(zephyr_prebuilt ${TOPT} ${PROJECT_BINARY_DIR}/linker.cmd ${PRIV_STACK_LIB} ${zephyr_lnk})
set_property(TARGET   zephyr_prebuilt PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.cmd)
add_dependencies(     zephyr_prebuilt ${ALIGN_SIZING_DEP} ${PRIV_STACK_DEP} linker_script offsets)


if(NOT CONFIG_NATIVE_APPLICATION)
	set(NOSTDINC_F -nostdinc)
endif()

if(GKOF OR GKSF)
  set(logical_target_for_zephyr_elf kernel_elf)

  # The second linker pass uses the same source linker script of the
  # first pass (LINKER_SCRIPT), but this time with a different output
  # file and preprocessed with the define LINKER_PASS2.
  construct_add_custom_command_for_linker_pass(linker_pass_final custom_command)
  add_custom_command(
    ${custom_command}
    )

  add_custom_target(
    linker_pass_final_script
    DEPENDS
    ${ALIGN_SIZING_DEP} ${PRIV_STACK_DEP}
    zephyr_prebuilt
    linker_pass_final.cmd
    offsets_h
    )
  set_property(TARGET
    linker_pass_final_script
    PROPERTY INCLUDE_DIRECTORIES
    ${ZEPHYR_INCLUDE_DIRS}
  )

  add_executable(       kernel_elf misc/empty_file.c ${GKSF})
  target_link_libraries(kernel_elf ${GKOF} ${TOPT} ${PROJECT_BINARY_DIR}/linker_pass_final.cmd ${zephyr_lnk})
  set_property(TARGET   kernel_elf PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_pass_final.cmd)
  add_dependencies(     kernel_elf ${ALIGN_SIZING_DEP} ${PRIV_STACK_DEP} linker_pass_final_script)
else()
  set(logical_target_for_zephyr_elf zephyr_prebuilt)
  # Use the prebuilt elf as the final elf since we don't have a
  # generation stage.
endif()

# To avoid having the same logical target name for the zephyr lib and
# the zephyr elf, we set the kernel_elf file name to zephyr.elf.
set_target_properties(${logical_target_for_zephyr_elf} PROPERTIES OUTPUT_NAME ${KERNEL_NAME})

set(post_build_commands "")

list_append_ifdef(CONFIG_CHECK_LINK_MAP
  post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/check_link_map.py ${KERNEL_MAP_NAME}
  )

list_append_ifdef(
  CONFIG_BUILD_OUTPUT_HEX
  post_build_commands
  COMMAND ${CMAKE_OBJCOPY} -S -Oihex --gap-fill 0xFF  -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_HEX_NAME}
  )

list_append_ifdef(
  CONFIG_BUILD_OUTPUT_BIN
  post_build_commands
  COMMAND ${CMAKE_OBJCOPY} -S -Obinary --gap-fill 0xFF -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_BIN_NAME}
  )

list_append_ifdef(
  CONFIG_BUILD_OUTPUT_S19
  post_build_commands
  COMMAND ${CMAKE_OBJCOPY} --gap-fill 0xFF --srec-len 1 --output-target=srec ${KERNEL_ELF_NAME} ${KERNEL_S19_NAME}
  )

list_append_ifdef(
  CONFIG_OUTPUT_DISASSEMBLY
  post_build_commands
  COMMAND ${CMAKE_OBJDUMP} -S ${KERNEL_ELF_NAME} >  ${KERNEL_LST_NAME}
  )

list_append_ifdef(
  CONFIG_OUTPUT_STAT
  post_build_commands
  COMMAND ${CMAKE_READELF} -e ${KERNEL_ELF_NAME} >  ${KERNEL_STAT_NAME}
  )

list_append_ifdef(
  CONFIG_BUILD_OUTPUT_STRIPPED
  post_build_commands
  COMMAND ${CMAKE_STRIP}   --strip-all ${KERNEL_ELF_NAME} -o ${KERNEL_STRIP_NAME}
  )

list_append_ifdef(
  CONFIG_BUILD_OUTPUT_EXE
  post_build_commands
  COMMAND ${CMAKE_COMMAND} -E rename ${KERNEL_ELF_NAME}    ${KERNEL_EXE_NAME}
  )

add_custom_command(
  TARGET ${logical_target_for_zephyr_elf}
  POST_BUILD
  ${post_build_commands}
  COMMENT "Generating files from zephyr.elf for board: ${BOARD}"
  # NB: COMMENT only works for some CMake-Generators
)

if(CONFIG_OUTPUT_PRINT_MEMORY_USAGE)
  # Use --print-memory-usage with the first link.
  #
  # Don't use this option with the second link because seeing it twice
  # could confuse users and using it on the second link would suppress
  # it when the first link has a ram/flash-usage issue.
  set(option ${LINKERFLAGPREFIX},--print-memory-usage)
  string(MAKE_C_IDENTIFIER check${option} check)

  set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${option}")
  zephyr_check_compiler_flag(C "" ${check})
  set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})

  target_link_libraries_ifdef(${check} zephyr_prebuilt ${option})
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

if(CONFIG_ASSERT AND (NOT CONFIG_FORCE_NO_ASSERT))
  message(WARNING "
      ------------------------------------------------------------
      --- WARNING:  __ASSERT() statements are globally ENABLED ---
      --- The kernel will run more slowly and use more memory  ---
      ------------------------------------------------------------"
)
endif()

if(CONFIG_BOARD_DEPRECATED)
  message(WARNING "
      WARNING:  The board '${BOARD}' is deprecated and will be
      removed in version ${CONFIG_BOARD_DEPRECATED}"
)
endif()

if(CONFIG_X86 AND CONFIG_USERSPACE AND NOT CONFIG_X86_NO_MELTDOWN)
  message(WARNING "
      WARNING: You have enabled CONFIG_USERSPACE on an x86-based target.
      If your CPU is vulnerable to the Meltdown CPU bug, security of
      supervisor-only memory pages is not guaranteed. This version of Zephyr
      does not contain a fix for this issue."
)
endif()
