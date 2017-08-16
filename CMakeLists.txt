cmake_minimum_required(VERSION 3.7)

# Copy the PROJECT vars into APPLICATION vars before they are modified
# by project()
set(APPLICATION_SOURCE_DIR ${PROJECT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${PROJECT_BINARY_DIR} CACHE PATH "Application Binary Directory")

project(Zephyr-Kernel VERSION 1.8.99)
enable_language(C CXX ASM)

# Do not generate make install target.
set(CMAKE_SKIP_INSTALL_RULES ON)

set(CMAKE_EXECUTABLE_SUFFIX .elf)

set(AUTOCONF_H ${PROJECT_BINARY_DIR}/include/generated/autoconf.h)
# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})

find_package(PythonInterp 3.4)

if(NOT PREBUILT_HOST_TOOLS)
  set(PREBUILT_HOST_TOOLS $ENV{PREBUILT_HOST_TOOLS} CACHE PATH "")
  if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    set(PREBUILT_HOST_TOOLS ${PROJECT_SOURCE_DIR}/scripts/prebuilt)
  endif()
endif()

if(NOT PREBUILT_HOST_TOOLS)
  message(FATAL_ERROR "Building host tools not yet supported, please use PREBUILT_HOST_TOOLS")
endif()

include(cmake/extensions.cmake)
include(cmake/kconfig.cmake)

# set the variables VERSION_MAJOR, PATCHLEVEL and more
include(cmake/version.cmake)

set(SOC_NAME ${CONFIG_SOC})
set(SOC_SERIES ${CONFIG_SOC_SERIES})
set(SOC_FAMILY ${CONFIG_SOC_FAMILY})

if("${SOC_SERIES}" STREQUAL "")
  set(SOC_PATH ${SOC_NAME})
else()
  set(SOC_PATH ${SOC_FAMILY}/${SOC_SERIES})
endif()

set(ARCH ${CONFIG_ARCH})
set(BOARD_NAME ${CONFIG_BOARD})
set(KERNEL_NAME ${CONFIG_KERNEL_BIN_NAME})

set(KERNEL_ELF_NAME ${KERNEL_NAME}.elf)
set(KERNEL_BIN_NAME ${KERNEL_NAME}.bin)
set(KERNEL_HEX_NAME ${KERNEL_NAME}.hex)
set(KERNEL_STAT_NAME ${KERNEL_NAME}.stat)

include(dts/dts.cmake)

include_directories(
  kernel/include
  arch/${ARCH}/include
  arch/${ARCH}/soc/${SOC_PATH}
  boards/${ARCH}/${BOARD_NAME}
  include
  lib/libc/minimal/include
  ${PROJECT_BINARY_DIR}/include/generated
  ${USERINCLUDE}
  ${STDINCLUDE}
)

add_definitions(
  -DKERNEL
  -D__ZEPHYR__=1
  -include ${AUTOCONF_H}
  -D_FORTIFY_SOURCE=2
)

add_compile_options(
  -Os # TODO: allow config to override this
  -g # TODO: build configuration enough?
  -Wall
  -Wformat
  -Wformat-security
  -Wno-format-zero-length
  -Wno-main
  -ffreestanding
)

add_compile_options(
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

ld_option(-nostartfiles)
ld_option(-nodefaultlibs)
ld_option(-nostdlib)
ld_option(-static)
ld_option(-no-pie)

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'

separate_arguments(EXTRA_CPPFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_CPPFLAGS})
separate_arguments(EXTRA_LD_FLAGS_AS_LIST UNIX_COMMAND ${EXTRA_LD_FLAGS})
separate_arguments(EXTRA_CFLAGS_AS_LIST   UNIX_COMMAND ${EXTRA_CFLAGS})
separate_arguments(EXTRA_CXXFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_CXXFLAGS})
separate_arguments(EXTRA_AFLAGS_AS_LIST   UNIX_COMMAND ${EXTRA_AFLAGS})

if(EXTRA_CPPFLAGS)
  add_definitions(${EXTRA_CPPFLAGS_AS_LIST})
endif()
if(EXTRA_LDFLAGS)
  link_libraries(${EXTRA_LDFLAGS_AS_LIST})
endif()
if(EXTRA_CFLAGS)
  foreach(F ${EXTRA_CFLAGS_AS_LIST})
    add_compile_options($<$<COMPILE_LANGUAGE:C>:${F}>)
  endforeach()
endif()
if(EXTRA_CXXFLAGS)
  foreach(F ${EXTRA_CXXFLAGS_AS_LIST})
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${F}>)
  endforeach()
endif()
if(EXTRA_AFLAGS)
  foreach(F ${EXTRA_AFLAGS_AS_LIST})
    add_compile_options($<$<COMPILE_LANGUAGE:ASM>:${F}>)
  endforeach()
endif()

if(CONFIG_READABLE_ASM)
  cc_option(-fno-reorder-blocks)
  cc_option(-fno-ipa-cp-clone)
  cc_option(-fno-partial-inlining)
endif()

cc_option(-fno-asynchronous-unwind-tables)
cc_option(-fno-pie)
cc_option(-fno-pic)

if(CONFIG_STACK_CANARIES)
  cc_option(-fstack-protector-all)
else()
  cc_option(-fno-stack-protector)
endif()

if(CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT)
  if(CONFIG_OMIT_FRAME_POINTER)
    cc_option(-fomit-frame-pointer)
  else()
    cc_option(-fno-omit-frame-pointer)
  endif()
endif()

# TODO: Handle default to Os
add_compile_options(${CONFIG_COMPILER_OPT})

# TODO: Include arch compiler options at this point.

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
   cc_disable_warning(no-unknown-warning-option)
   cc_disable_warning(unused-variable)
   cc_disable_warning(format-invalid-specifier)
   cc_disable_warning(gnu)
   # comparison of unsigned expression < 0 is always false
   cc_disable_warning(tautological-compare)
else() # GCC assumed
  cc_disable_warning(unused-but-set-variable)
  cc_option(-fno-reorder-functions)
  if(NOT ${ZEPHYR_GCC_VARIANT} STREQUAL "xcc")
    cc_option(-fno-defer-pop)
  endif()
endif()

cc_option_ifdef(CONFIG_DEBUG_SECTION_MISMATCH -fno-inline-functions-called-once)

add_compile_options(-nostdinc)
include_directories(SYSTEM ${NOSTDINC})

cc_disable_warning(pointer-sign)
cc_option(-fno-strict-overflow)

cc_option_ifdef(CONFIG_STACK_USAGE -fstack-usage)

# Force an error when things like SYS_INIT(foo, ...) occur with a missing header.
cc_option(-Werror=implicit-int)

# Prohibit date/time macros, which would make the build non-deterministic
# cc-option(-Werror=date-time)

# TODO: Archiver arguments
# ar_option(D)

set_ifndef(LINKERFLAGPREFIX -Wl)
ld_option(${LINKERFLAGPREFIX},-X)
ld_option(${LINKERFLAGPREFIX},-N)
ld_option(${LINKERFLAGPREFIX},--gc-sections)
ld_option(${LINKERFLAGPREFIX},--build-id=none)

if(CONFIG_HAVE_CUSTOM_LINKER_SCRIPT)
  set(LINKER_SCRIPT ${PROJECT_BASE}/${CONFIG_CUSTOM_LINKER_SCRIPT})
  if(NOT EXISTS LINKER_SCRIPT)
    set(LINKER_SCRIPT ${CONFIG_CUSTOM_LINKER_SCRIPT})
  endif()
else()
  # Try a board specific linker file
  set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/boards/${ARCH}/${BOARD_NAME}/linker.ld)
  if(NOT EXISTS ${LINKER_SCRIPT})
    # If not available, try an SoC specific linker file
    set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/arch/${ARCH}/soc/${SOC_PATH}/linker.ld)
  endif()
endif()

if(NOT EXISTS ${LINKER_SCRIPT})
  message(FATAL_ERROR "Could not find linker script ${LINKER_SCRIPT}")
endif()

configure_file(version.h.in ${PROJECT_BINARY_DIR}/include/generated/version.h)

define_property(GLOBAL PROPERTY ZEPHYR_LIBS
    BRIEF_DOCS "Global list of all Zephyr CMake libs that should be linked in"
    FULL_DOCS "Global list of all Zephyr CMake libs that should be linked in. zephyr_library() appends libs to this list.")
set_property(GLOBAL PROPERTY ZEPHYR_LIBS "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES
  BRIEF_DOCS "Object files that are generated after Zephyr has been linked once."
  FULL_DOCS "\
Object files that are generated after Zephyr has been linked once.\
May include isr tables, mmu tables, etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES "")

# zephyr is a catchall CMake library for source files that don't need
# to be built with include paths, defines, or other compiler flags
# above what all zephyr source files use.
add_library(zephyr STATIC "")
zephyr_library(zephyr)

# main is a "CMake OBJECT library"[0] containing all the application
# code and is modified by the entry point
# ${APPLICATION_SOURCE_DIR}/CMakeLists.txt that was specified when
# cmake was called.  [0]
# https://cmake.org/Wiki/CMake/Tutorials/Object_Library
add_library(main OBJECT "")

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

# Get ZEPHYR_INCLUDE for preprocessing, with cmake 3.8 this should be possible with generator expressions.
get_property(ZEPHYR_INCLUDES_LIST TARGET zephyr PROPERTY INCLUDE_DIRECTORIES)
foreach(INCLUDE ${ZEPHYR_INCLUDES_LIST})
  list(APPEND ZEPHYR_INCLUDES -I${INCLUDE})
endforeach()

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

get_filename_component(BASE_NAME ${CMAKE_CURRENT_BINARY_DIR} NAME)
add_custom_command(
  OUTPUT linker.cmd
  DEPENDS ${LINKER_SCRIPT}
  ${LINKER_SCRIPT_DEP}
  COMMAND ${CMAKE_C_COMPILER}
  -x assembler-with-cpp
  -MD -MF linker.cmd.dep -MT ${BASE_NAME}/linker.cmd
  ${ZEPHYR_INCLUDES}
  -D__GCC_LINKER_CMD__
  -E ${LINKER_SCRIPT} -P
  -o linker.cmd
  VERBATIM
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)
add_custom_target(
  linker_script
  DEPENDS linker.cmd
)

if(TOOLCHAIN_DO_PASS2)
  include(${CMAKE_TOOLCHAIN_FILE})
endif()

add_subdirectory(kernel)

# Read list content
get_property(ZEPHYR_LIBS_PROPERTY GLOBAL PROPERTY ZEPHYR_LIBS)

set(zephyr_lnk
  ${LINKERFLAGPREFIX},-Map=${PROJECT_BINARY_DIR}/${KERNEL_NAME}.map
  -u_OffsetAbsSyms
  -u_ConfigAbsSyms
  -e__start
  ${LINKERFLAGPREFIX},--start-group
  ${LINKERFLAGPREFIX},--whole-archive
  ${ZEPHYR_LIBS_PROPERTY}
  ${LINKERFLAGPREFIX},--end-group
  ${LINKERFLAGPREFIX},--start-group
  ${LINKERFLAGPREFIX},--no-whole-archive
  kernel
  ${LINKERFLAGPREFIX},--end-group
  ${LIB_INCLUDE_DIR}
  ${TOOLCHAIN_LIBS}
)

target_object_link_libraries(main zephyr)

if(CONFIG_GEN_ISR_TABLES)
  if("${ARCH}" STREQUAL "riscv32")
    set(OUTPUT_FORMAT elf32-littleriscv)
  else()
    set(OUTPUT_FORMAT elf32-little${ARCH})
  endif()

  # isr_tables.c is generated from prebuilt_kernel by
  # gen_isr_tables.py
  add_custom_command(
    OUTPUT isr_tables.c
    COMMAND ${CMAKE_OBJCOPY}
    -I ${OUTPUT_FORMAT}
    -O binary
    --only-section=.intList
    $<TARGET_FILE:prebuilt_kernel>
    isrList.bin
    COMMAND ${PYTHON_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/arch/common/gen_isr_tables.py
    --output-source isr_tables.c
    --intlist isrList.bin
    --debug
    --sw-isr-table
    --vector-table
    DEPENDS prebuilt_kernel
    )
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_OBJECT_FILES isr_tables.c)
endif()

# Read the global variable into the local variable GKOF
get_property(GKOF GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES)

add_executable(       prebuilt_kernel $<TARGET_OBJECTS:main> $<TARGET_OBJECTS:offsets>)
target_link_libraries(prebuilt_kernel -T${PROJECT_BINARY_DIR}/linker.cmd ${zephyr_lnk})
set_property(TARGET   prebuilt_kernel PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.cmd)
add_dependencies(     prebuilt_kernel linker_script)
if(GKOF)
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
    -MD -MF linker_pass2.cmd.dep -MT ${BASE_NAME}/linker_pass2.cmd
    ${ZEPHYR_INCLUDES}
    -D__GCC_LINKER_CMD__
    -DLINKER_PASS2
    -E ${LINKER_SCRIPT} -P
    -o linker_pass2.cmd
    VERBATIM
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
  add_custom_target(linker_pass2_script DEPENDS linker_pass2.cmd)

  add_executable(       kernel_elf $<TARGET_OBJECTS:main> $<TARGET_OBJECTS:offsets> ${GKOF})
  target_link_libraries(kernel_elf -T${PROJECT_BINARY_DIR}/linker_pass2.cmd ${zephyr_lnk})
  set_property(TARGET   kernel_elf PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker_pass2.cmd)
  add_dependencies(     kernel_elf linker_pass2_script)
else()
  set(logical_target_for_zephyr_elf prebuilt_kernel)
  # Use the prebuilt elf as the final elf since we don't have a
  # generation stage.
endif()

# To avoid having the same logical target name for the zephyr lib and
# the zephyr elf, we set the kernel_elf file name to zephyr.elf.
set_target_properties(${logical_target_for_zephyr_elf} PROPERTIES OUTPUT_NAME "zephyr")

add_custom_command(
  TARGET ${logical_target_for_zephyr_elf}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -S -Oihex   -R .comment -R COMMON -R .eh_frame ${KERNEL_ELF_NAME} ${KERNEL_HEX_NAME}
  COMMAND ${CMAKE_OBJCOPY} -S -Obinary -R .comment -R COMMON -R .eh_frame ${KERNEL_ELF_NAME} ${KERNEL_BIN_NAME}
  COMMAND ${CMAKE_OBJDUMP} -D                                             ${KERNEL_ELF_NAME} > ${KERNEL_NAME}.lst
  COMMENT "Generating zephyr.{hex,bin,lst} from zephyr.elf"
  # NB: COMMENT only works for some CMake-Generators
)
