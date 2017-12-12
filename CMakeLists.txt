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

# Do not generate make install target.
set(CMAKE_SKIP_INSTALL_RULES ON)

set(CMAKE_EXECUTABLE_SUFFIX .elf)

set(SOC_NAME ${CONFIG_SOC})
set(SOC_SERIES ${CONFIG_SOC_SERIES})
set(SOC_FAMILY ${CONFIG_SOC_FAMILY})

if("${SOC_SERIES}" STREQUAL "")
  set(SOC_PATH ${SOC_NAME})
else()
  set(SOC_PATH ${SOC_FAMILY}/${SOC_SERIES})
endif()

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
  arch/${ARCH}/soc/${SOC_PATH}
  arch/${ARCH}/soc/${SOC_PATH}/include
  arch/${ARCH}/soc/${SOC_FAMILY}/include
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
  _FORTIFY_SOURCE=2
)

# We need to set an optimization level.
# Default to -Os
# unless CONFIG_DEBUG is set, then it is -Og
#
# also, some toolchain's break with -Os, and some toolchain's break
# with -Og so allow them to override what flag to use
#
# Finally, the user can use Kconfig to add compiler options that will
# come after these options and override them
set_ifndef(OPTIMIZE_FOR_SIZE_FLAG  "-Os")
set_ifndef(OPTIMIZE_FOR_DEBUG_FLAG "-Og")
if(CONFIG_DEBUG)
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_DEBUG_FLAG})
else()
  set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SIZE_FLAG}) # Default
endif()

zephyr_compile_options(
  ${OPTIMIZATION_FLAG} # Usually -Os
  -g # TODO: build configuration enough?
  -Wall
  -Wformat
  -Wformat-security
  -Wno-format-zero-length
  -Wno-main
  -ffreestanding
  -include ${AUTOCONF_H}
)

zephyr_compile_options(
  $<$<COMPILE_LANGUAGE:C>:-std=c99>

  $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
  $<$<COMPILE_LANGUAGE:CXX>:-fcheck-new>
  $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>
  $<$<COMPILE_LANGUAGE:CXX>:-fdata-sections>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>

  $<$<COMPILE_LANGUAGE:ASM>:-xassembler-with-cpp>
  $<$<COMPILE_LANGUAGE:ASM>:-D_ASMLANGUAGE>
)

zephyr_ld_options(
  -nostartfiles
  -nodefaultlibs
  -nostdlib
  -static
  -no-pie
  )

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

zephyr_compile_options(${CONFIG_COMPILER_OPT})

# TODO: Include arch compiler options at this point.

# TODO: This Clang check is broken
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  zephyr_cc_option(
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
  if(NOT ${ZEPHYR_GCC_VARIANT} STREQUAL "xcc")
    zephyr_cc_option(-fno-defer-pop)
  endif()
endif()

zephyr_cc_option_ifdef(CONFIG_DEBUG_SECTION_MISMATCH -fno-inline-functions-called-once)
zephyr_cc_option_ifdef(CONFIG_STACK_USAGE            -fstack-usage)

zephyr_compile_options(-nostdinc)
zephyr_system_include_directories(${NOSTDINC})

# Force an error when things like SYS_INIT(foo, ...) occur with a missing header.
zephyr_cc_option(-Werror=implicit-int)

# Prohibit date/time macros, which would make the build non-deterministic
# cc-option(-Werror=date-time)

# TODO: Archiver arguments
# ar_option(D)

if(IS_TEST)
  add_subdirectory(cmake/test)
endif()

set_ifndef(LINKERFLAGPREFIX -Wl)
zephyr_ld_options(
  ${LINKERFLAGPREFIX},-X
  ${LINKERFLAGPREFIX},-N
  ${LINKERFLAGPREFIX},--gc-sections
  ${LINKERFLAGPREFIX},--build-id=none
  )

if(CONFIG_HAVE_CUSTOM_LINKER_SCRIPT)
  set(LINKER_SCRIPT ${APPLICATION_SOURCE_DIR}/${CONFIG_CUSTOM_LINKER_SCRIPT})
  if(NOT EXISTS LINKER_SCRIPT)
    set(LINKER_SCRIPT ${CONFIG_CUSTOM_LINKER_SCRIPT})
    if(NOT EXISTS LINKER_SCRIPT)
      message(FATAL_ERROR "CONFIG_HAVE_CUSTOM_LINKER_SCRIPT was set, but no linker script was found at '${CONFIG_CUSTOM_LINKER_SCRIPT}'")
    endif()
  endif()
else()
  # Try a board specific linker file
  set(LINKER_SCRIPT ${BOARD_DIR}/linker.ld)
  if(NOT EXISTS ${LINKER_SCRIPT})
    # If not available, try an SoC specific linker file
    set(LINKER_SCRIPT $ENV{ZEPHYR_BASE}/arch/${ARCH}/soc/${SOC_PATH}/linker.ld)
  endif()
endif()

if(NOT EXISTS ${LINKER_SCRIPT})
  message(FATAL_ERROR "Could not find linker script: '${LINKER_SCRIPT}'. Corrupted configuration?")
endif()

configure_file(version.h.in ${PROJECT_BINARY_DIR}/include/generated/version.h)

add_subdirectory(lib)
add_subdirectory(misc)
# We use include instead of add_subdirectory to avoid creating a new directory scope.
# This is because source file properties are directory scoped, including the GENERATED
# property which is set implicitly for custom command outputs
include(misc/generated/CMakeLists.txt)
add_subdirectory(boards)
add_subdirectory(ext)
add_subdirectory(subsys)
add_subdirectory(arch)
add_subdirectory(drivers)
add_subdirectory(tests)

set(syscall_macros_h ${ZEPHYR_BINARY_DIR}/include/generated/syscall_macros.h)

add_custom_target(syscall_macros_h_target DEPENDS ${syscall_macros_h})
add_custom_command(                       OUTPUT  ${syscall_macros_h}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${PROJECT_SOURCE_DIR}/scripts/gen_syscall_header.py
  > ${syscall_macros_h}
  DEPENDS ${PROJECT_SOURCE_DIR}/scripts/gen_syscall_header.py
  )

# This command is a hack to support commands that are always run. Any
# target that depends on always_rebuild will always be rebuilt.
add_custom_command(OUTPUT always_rebuild COMMAND cmake -E echo Building for board ${BOARD})

set(syscall_list_h ${CMAKE_CURRENT_BINARY_DIR}/include/generated/syscall_list.h)
set(syscalls_json  ${CMAKE_CURRENT_BINARY_DIR}/misc/generated/syscalls.json)

add_custom_command(
  OUTPUT
  ${syscalls_json}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${PROJECT_SOURCE_DIR}/scripts/parse_syscalls.py
  --include          ${PROJECT_SOURCE_DIR}/include        # Read files from this dir
  --json-file        ${syscalls_json}                     # Write this file
  DEPENDS always_rebuild
  )

add_custom_target(syscall_list_h_target DEPENDS                ${syscall_list_h})
add_custom_command(OUTPUT include/generated/syscall_dispatch.c ${syscall_list_h}
  # Also, some files are written to include/generated/syscalls/
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${PROJECT_SOURCE_DIR}/scripts/gen_syscalls.py
  --json-file        ${syscalls_json}                     # Read this file
  --base-output      include/generated/syscalls           # Write to this dir
  --syscall-dispatch include/generated/syscall_dispatch.c # Write this file
  > ${syscall_list_h}                                     # Write stdout to this file
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${syscalls_json}
  )

# Generate offsets.c.obj from offsets.c
# Generate offsets.h     from offsets.c.obj

set(OFFSETS_C_PATH $ENV{ZEPHYR_BASE}/arch/${ARCH}/core/offsets/offsets.c)
set(OFFSETS_O_PATH ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/offsets.dir/arch/${ARCH}/core/offsets/offsets.c.obj)
set(OFFSETS_H_PATH ${PROJECT_BINARY_DIR}/include/generated/offsets.h)

add_library(          offsets STATIC ${OFFSETS_C_PATH})
target_link_libraries(offsets zephyr_interface)
add_dependencies(     offsets
  syscall_list_h_target
  syscall_macros_h_target
  )

add_custom_command(
  OUTPUT ${OFFSETS_H_PATH}
  COMMAND ${PYTHON_EXECUTABLE} $ENV{ZEPHYR_BASE}/scripts/gen_offset_header.py
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

  # Verify that all libraries have source files. Libraries without
  # source files are not supported because they are an indication that
  # something has been misconfigured.
  get_target_property(lib_sources ${zephyr_lib} SOURCES)
  if(lib_sources STREQUAL lib_sources-NOTFOUND
      AND (NOT (${zephyr_lib} STREQUAL app))
      )
    # app is not checked because it's sources are added to it after
    # this CMakeLists.txt file has been processed
    message(FATAL_ERROR "\
The Zephyr library '${zephyr_lib}' was created without source files. \
Empty libraries are not supported. \
Either make sure that the library has the sources it should have, \
or make sure it is not created when it has no source files.")
  endif()
endforeach()

get_property(OUTPUT_FORMAT        GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)

# Run the pre-processor on the linker script
#
# Deal with the un-preprocessed linker scripts differently with
# different generators.
if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
  # Note that the IMPLICIT_DEPENDS option is currently supported only
  # for Makefile generators and will be ignored by other generators.
  set(LINKER_SCRIPT_DEP IMPLICIT_DEPENDS C ${LINKER_SCRIPT})
elseif(CMAKE_GENERATOR STREQUAL "Ninja")
  # Using DEPFILE with other generators than Ninja is an error.
  set(LINKER_SCRIPT_DEP DEPFILE ${PROJECT_BINARY_DIR}/linker.cmd.dep)
else()
  # TODO: How would the linker script dependencies work for non-linker
  # script generators.
  message(STATUS "Warning; this generator is not well supported. The
  Linker script may not be regenerated when it should.")
  set(LINKER_SCRIPT_DEP "")
endif()

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

    list(APPEND ks "${fixed_path}lib${target_name}.a")
  endforeach()

  # We are done constructing kernel_object_file_list, now we inject this
  # information into the linker script through -D's
  list(LENGTH kernel_object_file_list NUM_KERNEL_OBJECT_FILES)
  list(APPEND LINKER_SCRIPT_DEFINES -DNUM_KERNEL_OBJECT_FILES=${NUM_KERNEL_OBJECT_FILES})
  set(i 0)
  foreach(f ${ks})
    list(APPEND LINKER_SCRIPT_DEFINES -DKERNEL_OBJECT_FILE_${i}=${f})
    math(EXPR i "${i}+1")
  endforeach()
endif() # CONFIG_APPLICATION_MEMORY

get_filename_component(BASE_NAME ${CMAKE_CURRENT_BINARY_DIR} NAME)
add_custom_command(
  OUTPUT linker.cmd
  DEPENDS ${LINKER_SCRIPT}
  ${LINKER_SCRIPT_DEP}
  # NB: This COMMAND is copy-pasted to generate linker_pass2.cmd
  # TODO: Remove duplication
  COMMAND ${CMAKE_C_COMPILER}
  -x assembler-with-cpp
  -nostdinc
  -undef
  -MD -MF linker.cmd.dep -MT ${BASE_NAME}/linker.cmd
  ${ZEPHYR_INCLUDES}
  ${LINKER_SCRIPT_DEFINES}
  -E ${LINKER_SCRIPT} -P
  -o linker.cmd
  VERBATIM
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)
add_custom_target(
  linker_script
  DEPENDS
  linker.cmd
  offsets_h
)

set(zephyr_lnk
  ${LINKERFLAGPREFIX},-Map=${PROJECT_BINARY_DIR}/${KERNEL_MAP_NAME}
  -u_OffsetAbsSyms
  -u_ConfigAbsSyms
  -e${CONFIG_KERNEL_ENTRY}
  ${LINKERFLAGPREFIX},--start-group
  ${LINKERFLAGPREFIX},--whole-archive
  ${ZEPHYR_LIBS_PROPERTY}
  ${LINKERFLAGPREFIX},--no-whole-archive
  kernel
  ${OFFSETS_O_PATH}
  ${LINKERFLAGPREFIX},--end-group
  ${LIB_INCLUDE_DIR}
  -L${PROJECT_BINARY_DIR}
  ${TOOLCHAIN_LIBS}
  )

if(CONFIG_GEN_ISR_TABLES)
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
    $ENV{ZEPHYR_BASE}/arch/common/gen_isr_tables.py
    --output-source isr_tables.c
    --intlist isrList.bin
    --debug
    --sw-isr-table
    --vector-table
    DEPENDS zephyr_prebuilt
    )
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES isr_tables.c)
endif()

if(CONFIG_USERSPACE)
  set(GEN_KOBJ_LIST $ENV{ZEPHYR_BASE}/scripts/gen_kobject_list.py)
  set(PROCESS_GPERF $ENV{ZEPHYR_BASE}/scripts/process_gperf.py)

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
    --output ${OBJ_LIST}
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
    DEPENDS obj_list
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
    ${PROCESS_GPERF}
    -i ${OUTPUT_SRC_PRE}
    -o ${OUTPUT_SRC}
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS output_src_pre
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_target(output_src DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_SRC})

  # We need precise control of where generated text/data ends up in the final
  # kernel image. Disable function/data sections and use objcopy to move
  # generated data into special section names
  add_library(output_lib STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_SRC}
    )

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

# FIXME: Is there any way to get rid of empty_file.c?
add_executable(       zephyr_prebuilt misc/empty_file.c)
target_link_libraries(zephyr_prebuilt -T${PROJECT_BINARY_DIR}/linker.cmd ${zephyr_lnk})
set_property(TARGET   zephyr_prebuilt PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.cmd)
add_dependencies(     zephyr_prebuilt linker_script offsets)

if(GKOF OR GKSF)
  set(logical_target_for_zephyr_elf kernel_elf)

  # The second linker pass uses the same source linker script of the
  # first pass (LINKER_SCRIPT), but this time preprocessed with the
  # define LINKER_PASS2.
  add_custom_command(
    OUTPUT linker_pass2.cmd
    DEPENDS ${LINKER_SCRIPT}
    ${LINKER_SCRIPT_DEP}
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    -nostdinc
    -undef
    -MD -MF linker_pass2.cmd.dep -MT ${BASE_NAME}/linker_pass2.cmd
    ${ZEPHYR_INCLUDES}
    ${LINKER_SCRIPT_DEFINES}
    -DLINKER_PASS2
    -E ${LINKER_SCRIPT} -P
    -o linker_pass2.cmd
    VERBATIM
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
  add_custom_target(
    linker_pass2_script
    DEPENDS
    linker_pass2.cmd
    offsets_h
    )

  add_executable(       kernel_elf misc/empty_file.c ${GKSF})
  target_link_libraries(kernel_elf ${GKOF} -T${PROJECT_BINARY_DIR}/linker_pass2.cmd ${zephyr_lnk})
  set_property(TARGET   kernel_elf PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_pass2.cmd)
  add_dependencies(     kernel_elf linker_pass2_script)
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
  COMMAND ${PYTHON_EXECUTABLE} $ENV{ZEPHYR_BASE}/scripts/check_link_map.py ${KERNEL_MAP_NAME}
  )

list_append_ifdef(
  CONFIG_BUILD_OUTPUT_HEX
  post_build_commands
  COMMAND ${CMAKE_OBJCOPY} -S -Oihex   -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_HEX_NAME}
  )

list_append_ifdef(
  CONFIG_BUILD_OUTPUT_BIN
  post_build_commands
  COMMAND ${CMAKE_OBJCOPY} -S -Obinary -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_BIN_NAME}
  )
list_append_ifdef(
  CONFIG_BUILD_OUTPUT_S19
  post_build_commands
  COMMAND ${CMAKE_OBJCOPY} --srec-len 1 --output-target=srec ${KERNEL_ELF_NAME} ${KERNEL_S19_NAME}
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

add_custom_command(
  TARGET ${logical_target_for_zephyr_elf}
  POST_BUILD
  ${post_build_commands}
  COMMENT "Generating files from zephyr.elf for board: ${BOARD}"
  # NB: COMMENT only works for some CMake-Generators
)

add_subdirectory(cmake/qemu)
add_subdirectory(cmake/flash)

add_subdirectory(cmake/usage)
add_subdirectory(cmake/reports)

include(cmake/ccache.cmake)

if(CONFIG_ASSERT)
  message(WARNING "
      ------------------------------------------------------------
      --- WARNING:  __ASSERT() statements are globally ENABLED ---
      --- The kernel will run more slowly and uses more memory ---
      ------------------------------------------------------------"
)
endif()

if(CONFIG_BOARD_DEPRECATED)
  message(WARNING "
      WARNING:  The board '${BOARD}' is deprecated and will be
      removed in version ${CONFIG_BOARD_DEPRECATED}"
)
endif()
