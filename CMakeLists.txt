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

if(NOT PROPERTY_LINKER_SCRIPT_DEFINES)
  set_property(GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__GCC_LINKER_CMD__)
endif()

# zephyr is a catchall CMake library for source files that can be
# built purely with the include paths, defines, and other compiler
# flags that all zephyr source files use.
zephyr_library_named(zephyr)

include(dts/dts.cmake)

target_include_directories(zephyr PUBLIC
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

target_compile_definitions(zephyr PUBLIC
  KERNEL
  __ZEPHYR__=1
  _FORTIFY_SOURCE=2
)

target_compile_options(zephyr PUBLIC
  -Os # TODO: allow config to override this
  -g # TODO: build configuration enough?
  -Wall
  -Wformat
  -Wformat-security
  -Wno-format-zero-length
  -Wno-main
  -ffreestanding
  -include ${AUTOCONF_H}
)

target_compile_options(zephyr PUBLIC
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

target_link_libraries(zephyr PUBLIC
  -nostartfiles
  -nodefaultlibs
  -nostdlib
  -static
  -no-pie
  )

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'

separate_arguments(EXTRA_CPPFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_CPPFLAGS})
separate_arguments(EXTRA_LD_FLAGS_AS_LIST UNIX_COMMAND ${EXTRA_LD_FLAGS})
separate_arguments(EXTRA_CFLAGS_AS_LIST   UNIX_COMMAND ${EXTRA_CFLAGS})
separate_arguments(EXTRA_CXXFLAGS_AS_LIST UNIX_COMMAND ${EXTRA_CXXFLAGS})
separate_arguments(EXTRA_AFLAGS_AS_LIST   UNIX_COMMAND ${EXTRA_AFLAGS})

if(EXTRA_CPPFLAGS)
  target_compile_definitions(zephyr PUBLIC ${EXTRA_CPPFLAGS_AS_LIST})
endif()
if(EXTRA_LDFLAGS)
  target_link_libraries(zephyr PUBLIC ${EXTRA_LDFLAGS_AS_LIST})
endif()
if(EXTRA_CFLAGS)
  foreach(F ${EXTRA_CFLAGS_AS_LIST})
    target_compile_options(zephyr PUBLIC $<$<COMPILE_LANGUAGE:C>:${F}>)
  endforeach()
endif()
if(EXTRA_CXXFLAGS)
  foreach(F ${EXTRA_CXXFLAGS_AS_LIST})
    target_compile_options(zephyr PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${F}>)
  endforeach()
endif()
if(EXTRA_AFLAGS)
  foreach(F ${EXTRA_AFLAGS_AS_LIST})
    target_compile_options(zephyr PUBLIC $<$<COMPILE_LANGUAGE:ASM>:${F}>)
  endforeach()
endif()

if(CONFIG_READABLE_ASM)
  target_cc_option(zephyr PUBLIC -fno-reorder-blocks)
  target_cc_option(zephyr PUBLIC -fno-ipa-cp-clone)
  target_cc_option(zephyr PUBLIC -fno-partial-inlining)
endif()

target_cc_option(zephyr PUBLIC -fno-asynchronous-unwind-tables)
target_cc_option(zephyr PUBLIC -fno-pie)
target_cc_option(zephyr PUBLIC -fno-pic)
target_cc_option(zephyr PUBLIC -fno-strict-overflow)
target_cc_option(zephyr PUBLIC -Wno-pointer-sign)

if(CONFIG_STACK_CANARIES)
  target_cc_option(zephyr PUBLIC -fstack-protector-all)
else()
  target_cc_option(zephyr PUBLIC -fno-stack-protector)
endif()

if(CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT)
  if(CONFIG_OMIT_FRAME_POINTER)
    target_cc_option(zephyr PUBLIC -fomit-frame-pointer)
  else()
    target_cc_option(zephyr PUBLIC -fno-omit-frame-pointer)
  endif()
endif()

# TODO: Handle default to Os
target_compile_options(zephyr PUBLIC ${CONFIG_COMPILER_OPT})

# TODO: Include arch compiler options at this point.

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  target_cc_option(zephyr PUBLIC -Wno-unknown-warning-option)
  target_cc_option(zephyr PUBLIC -Wno-unused-variable)
  target_cc_option(zephyr PUBLIC -Wno-format-invalid-specifier)
  target_cc_option(zephyr PUBLIC -Wno-gnu)
  # comparison of unsigned expression < 0 is always false
  target_cc_option(zephyr PUBLIC -Wtautological-compare)
else() # GCC assumed
  target_cc_option(zephyr PUBLIC -Wno-unused-but-set-variable)
  target_cc_option(zephyr PUBLIC -fno-reorder-functions)
  if(NOT ${ZEPHYR_GCC_VARIANT} STREQUAL "xcc")
    target_cc_option(zephyr PUBLIC -fno-defer-pop)
  endif()
endif()

target_cc_option_ifdef(CONFIG_DEBUG_SECTION_MISMATCH zephyr PUBLIC -fno-inline-functions-called-once)
target_cc_option_ifdef(CONFIG_STACK_USAGE zephyr PUBLIC -fstack-usage)

target_compile_options(zephyr PUBLIC -nostdinc)
target_include_directories(zephyr SYSTEM PUBLIC ${NOSTDINC})

# Force an error when things like SYS_INIT(foo, ...) occur with a missing header.
target_cc_option(zephyr PUBLIC -Werror=implicit-int)

# Prohibit date/time macros, which would make the build non-deterministic
# cc-option(-Werror=date-time)

# TODO: Archiver arguments
# ar_option(D)

set_ifndef(LINKERFLAGPREFIX -Wl)
target_link_libraries(zephyr PUBLIC ${LINKERFLAGPREFIX},-X)
target_link_libraries(zephyr PUBLIC ${LINKERFLAGPREFIX},-N)
target_link_libraries(zephyr PUBLIC ${LINKERFLAGPREFIX},--gc-sections)
target_link_libraries(zephyr PUBLIC ${LINKERFLAGPREFIX},--build-id=none)

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

# Generate offsets.h from offsets.o

set(OFFSETS_C_PATH ${PROJECT_SOURCE_DIR}/arch/${ARCH}/core/offsets/offsets.c)
add_library(offsets OBJECT ${OFFSETS_C_PATH})
target_object_link_libraries(offsets zephyr)

set(OFFSETS_H_PATH ${PROJECT_BINARY_DIR}/include/generated/offsets.h)
add_custom_command(
  OUTPUT ${OFFSETS_H_PATH}
  COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/gen_offset_header.py
  -i ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/offsets.dir/arch/${ARCH}/core/offsets/offsets.c.obj
  -o ${OFFSETS_H_PATH}
  DEPENDS offsets
)
add_custom_target(offsets_h DEPENDS ${OFFSETS_H_PATH})

add_dependencies(zephyr offsets_h)

target_sources(zephyr PRIVATE ${OFFSETS_C_PATH})

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

get_property(LINKER_SCRIPT_DEFINES GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES)

get_filename_component(BASE_NAME ${CMAKE_CURRENT_BINARY_DIR} NAME)
add_custom_command(
  OUTPUT linker.cmd
  DEPENDS ${LINKER_SCRIPT}
  ${LINKER_SCRIPT_DEP}
  COMMAND ${CMAKE_C_COMPILER}
  -x assembler-with-cpp
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
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES isr_tables.c)

endif()

# Read global variables into local variables
get_property(GKOF GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES)
get_property(GKSF GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES)

add_executable(       prebuilt_kernel dummy.c)
target_link_libraries(prebuilt_kernel -T${PROJECT_BINARY_DIR}/linker.cmd ${zephyr_lnk})
set_property(TARGET   prebuilt_kernel PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.cmd)
add_dependencies(     prebuilt_kernel linker_script)

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

  add_executable(       kernel_elf dummy.c ${GKSF})
  target_link_libraries(kernel_elf ${GKOF} -T${PROJECT_BINARY_DIR}/linker_pass2.cmd ${zephyr_lnk})
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
