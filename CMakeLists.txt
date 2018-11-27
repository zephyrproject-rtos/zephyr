# *DOCUMENTATION*
#
# Note that this is *NOT* the top-level CMakeLists.txt. That's in the
# application. See the Application Development Primer documentation
# for details.
#
# To see a list of typical targets execute "make usage"
# More info can be located in ./README.rst
#
# Comments in this file are targeted only to the developer, do not
# expect to learn how to build the kernel reading this file.

project(Zephyr-Kernel VERSION ${PROJECT_VERSION})
enable_language(C CXX ASM)

if(NOT DEFINED ZEPHYR_BINARY_DIR)
  message(FATAL_ERROR "
    A user error has occurred.
    cmake was invoked with '${CMAKE_CURRENT_LIST_DIR}' specified as the source directory,
    but it must be invoked with an application source directory,
    such as '${CMAKE_CURRENT_LIST_DIR}/samples/hello_world'.
    Debug variables:
    CMAKE_CACHEFILE_DIR: ${CMAKE_CACHEFILE_DIR}"
  )
endif()

# Verify that the toolchain can compile a dummy file, if it cannot we
# won't be able to test for compatiblity with certain C flags.
check_c_compiler_flag("" toolchain_is_ok)
assert(toolchain_is_ok "The toolchain is unable to build a dummy C file. See CMakeError.log.")

# TODO: Clean up
set(CMAKE_EXECUTABLE_SUFFIX .elf)

# TODO: Clean up
if(NOT PROPERTY_LINKER_SCRIPT_DEFINES)
  set_property(GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES ${ZEPHYR_DEFINE_FLAG_C}__GCC_LINKER_CMD__)
endif()

# TODO: Clean up
define_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT BRIEF_DOCS " " FULL_DOCS " ")
set_property(   GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-little${ARCH}) # BFD format

# 'zephyr_interface' is a source-less "library" that encapsulates all the global
# compiler options needed by all source files. All zephyr libraries,
# including the library named 'zephyr', link with zephyr_interface to
# obtain these flags.
# https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#interface-libraries
add_library(zephyr_interface INTERFACE)

# 'zephyr' is a catch-all CMake library for source files that can be
# built purely with the include paths, defines, and other compiler
# flags that come with zephyr_interface.
zephyr_library_named(zephyr)

zephyr_include_directories(
  kernel/include
  arch/${ARCH}/include
  ${SOC_DIR}/${ARCH}/${SOC_PATH}
  ${SOC_DIR}/${ARCH}/${SOC_PATH}/include
  ${SOC_DIR}/${ARCH}/${SOC_FAMILY}/include
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

if(BUILD_VERSION)
  zephyr_compile_definitions(
    BUILD_VERSION=${BUILD_VERSION}
  )
endif()


# Discover if the default C standard has been overridden
# So far, only POSIX-based boards does this
get_property(CSTD GLOBAL PROPERTY CSTD)
# Set the default C standard to ISO/IEC 9899:1999
set_ifndef(CSTD c99)

toolchain_cc()
toolchain_ld()

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'
include(cmake/extra_flags.cmake)

# Permit verbatim Kconfig-specified additional options
separate_arguments(CONFIG_COMPILER_OPT_AS_LIST UNIX_COMMAND ${CONFIG_COMPILER_OPT})
zephyr_compile_options(${CONFIG_COMPILER_OPT_AS_LIST})




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
   --include          ${ZEPHYR_BASE}/include       # Read files from this dir
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


# CONFIG_APPLICATION_MEMORY: Split kernel and application memory
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
  set(def "${ZEPHYR_DEFINE_FLAG_C}KERNELSPACE_OBJECT_FILES=")
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

# Obtain ZEPHYR_INCLUDE_DIRS: the list of include directories
zephyr_get_include_directories_for_lang(C
  ZEPHYR_INCLUDE_DIRS
  STRIP_PREFIX  # Strip the CMAKE_INCLUDE_FLAG_C (e.g. "-I") prefix
)


toolchain_cc_preprocess_linker_pass(linker)
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
set_property(TARGET
  linker_script
  PROPERTY INCLUDE_DIRECTORIES
  ${ZEPHYR_INCLUDE_DIRS}
  )

toolchain_ld_link_step_0_init()
toolchain_bintools_generate_isr()
toolchain_bintools_usermode()

# FIXME: Is there any way to get rid of empty_file.c?
add_executable(       zephyr_prebuilt misc/empty_file.c)
set_property(TARGET   zephyr_prebuilt PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.cmd)
add_dependencies(     zephyr_prebuilt ${ALIGN_SIZING_DEP} ${PRIV_STACK_DEP} linker_script offsets)
toolchain_ld_link_step_1_base()

# Read global variables into local variables
get_property(GKOF GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES)
get_property(GKSF GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES)

if(GKOF OR GKSF)
  # The second linker pass uses the same source linker script of the
  # first pass (LINKER_SCRIPT), but this time with a different output
  # file and preprocessed with the define LINKER_PASS2.
  toolchain_cc_preprocess_linker_pass(linker_pass_final)

  add_custom_target(
    linker_pass_final_script
    DEPENDS
    ${ALIGN_SIZING_DEP} ${PRIV_STACK_DEP}
    zephyr_prebuilt
    linker_pass_final.cmd
    offsets_h
    )
  # Give the 'linker_pass_final_script' target all of the include directories so
  # that cmake can successfully find the linker_script's header
  # dependencies.
  set_property(TARGET
    linker_pass_final_script
    PROPERTY INCLUDE_DIRECTORIES
    ${ZEPHYR_INCLUDE_DIRS}
  )

  add_executable(       kernel_elf misc/empty_file.c ${GKSF})
  set_property(TARGET   kernel_elf PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_pass_final.cmd)
  add_dependencies(     kernel_elf ${ALIGN_SIZING_DEP} ${PRIV_STACK_DEP} linker_pass_final_script)
  toolchain_ld_link_step_2_generated()

  set(logical_target_for_zephyr_elf kernel_elf)
else()
  # Use the prebuilt elf as the final elf since we didn't have a
  # generation stage.
  set(logical_target_for_zephyr_elf zephyr_prebuilt)
endif()

# Export the variable to the application's scope to allow the
# application to know what the name of the final elf target is.
set(logical_target_for_zephyr_elf ${logical_target_for_zephyr_elf} PARENT_SCOPE)

# To avoid having the same logical target name for the zephyr lib and
# the zephyr elf, we set the kernel_elf file name to zephyr.elf.
set_target_properties(${logical_target_for_zephyr_elf} PROPERTIES OUTPUT_NAME ${KERNEL_NAME})

# Run some commands now that we have made a final elf
set(post_link_commands "")
toolchain_bintools_post_link()
add_custom_command(
  TARGET ${logical_target_for_zephyr_elf}
  POST_BUILD
  ${post_link_commands}
  COMMENT "Generating files from ${logical_target_for_zephyr_elf} for board: ${BOARD}"
  # NB: COMMENT only works for some CMake-Generators
)


##
## Post-build stuff
##

toolchain_bintools_print_size()

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
