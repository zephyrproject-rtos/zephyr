cmake_minimum_required(VERSION 3.7)

project(Zephyr-Kernel VERSION ${PROJECT_VERSION})
enable_language(C CXX ASM)

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

set(ARCH ${CONFIG_ARCH})
set(BOARD_NAME ${CONFIG_BOARD})
set(KERNEL_NAME ${CONFIG_KERNEL_BIN_NAME})

set(KERNEL_ELF_NAME   ${KERNEL_NAME}.elf)
set(KERNEL_BIN_NAME   ${KERNEL_NAME}.bin)
set(KERNEL_HEX_NAME   ${KERNEL_NAME}.hex)
set(KERNEL_MAP_NAME   ${KERNEL_NAME}.map)
set(KERNEL_LST_NAME   ${KERNEL_NAME}.lst)
set(KERNEL_S19_NAME   ${KERNEL_NAME}.s19)
set(KERNEL_STAT_NAME  ${KERNEL_NAME}.stat) # TODO: Port this
set(KERNEL_STRIP_NAME ${KERNEL_NAME}.strip)

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

zephyr_compile_options(
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

zephyr_link_libraries(
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
  zephyr_compile_definitions(${EXTRA_CPPFLAGS_AS_LIST})
endif()
if(EXTRA_LDFLAGS)
  zephyr_link_libraries(${EXTRA_LDFLAGS_AS_LIST})
endif()
if(EXTRA_CFLAGS)
  foreach(F ${EXTRA_CFLAGS_AS_LIST})
    zephyr_compile_options($<$<COMPILE_LANGUAGE:C>:${F}>)
  endforeach()
endif()
if(EXTRA_CXXFLAGS)
  foreach(F ${EXTRA_CXXFLAGS_AS_LIST})
    zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${F}>)
  endforeach()
endif()
if(EXTRA_AFLAGS)
  foreach(F ${EXTRA_AFLAGS_AS_LIST})
    zephyr_compile_options($<$<COMPILE_LANGUAGE:ASM>:${F}>)
  endforeach()
endif()

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

if(CONFIG_STACK_CANARIES)
  zephyr_cc_option(-fstack-protector-all)
else()
  zephyr_cc_option(-fno-stack-protector)
endif()

if(CONFIG_OVERRIDE_FRAME_POINTER_DEFAULT)
  if(CONFIG_OMIT_FRAME_POINTER)
    zephyr_cc_option(-fomit-frame-pointer)
  else()
    zephyr_cc_option(-fno-omit-frame-pointer)
  endif()
endif()

# TODO: Handle default to Os
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
zephyr_link_libraries(
  ${LINKERFLAGPREFIX},-X
  ${LINKERFLAGPREFIX},-N
  ${LINKERFLAGPREFIX},--gc-sections
  ${LINKERFLAGPREFIX},--build-id=none
  )

if(CONFIG_HAVE_CUSTOM_LINKER_SCRIPT)
  set(LINKER_SCRIPT ${PROJECT_BASE}/${CONFIG_CUSTOM_LINKER_SCRIPT})
  if(NOT EXISTS LINKER_SCRIPT)
    set(LINKER_SCRIPT ${CONFIG_CUSTOM_LINKER_SCRIPT})
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
add_subdirectory(tests)

# DTS is placed behind ext/ because
# /home/sebo/zephyr/dts/arm/ti/cc32xx.dtsi depends on include
# directories from ext/hal/ti/simplelink
include(dts/dts.cmake)

# Generate offsets.c.obj from offsets.c
# Generate offsets.h     from offsets.c.obj

set(OFFSETS_C_PATH $ENV{ZEPHYR_BASE}/arch/${ARCH}/core/offsets/offsets.c)
set(OFFSETS_O_PATH ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/offsets.dir/arch/${ARCH}/core/offsets/offsets.c.obj)
set(OFFSETS_H_PATH ${PROJECT_BINARY_DIR}/include/generated/offsets.h)

add_library(offsets STATIC ${OFFSETS_C_PATH})
target_link_libraries(offsets zephyr_interface)

add_custom_command(
  OUTPUT ${OFFSETS_H_PATH}
  COMMAND ${PYTHON_EXECUTABLE} $ENV{ZEPHYR_BASE}/scripts/gen_offset_header.py
  -i ${OFFSETS_O_PATH}
  -o ${OFFSETS_H_PATH}
  DEPENDS offsets
)
add_custom_target(offsets_h DEPENDS ${OFFSETS_H_PATH})

zephyr_include_directories(${TOOLCHAIN_INCLUDES})

zephyr_get_include_directories(ZEPHYR_INCLUDES)

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

add_subdirectory(kernel)

# Read list content
get_property(ZEPHYR_LIBS_PROPERTY GLOBAL PROPERTY ZEPHYR_LIBS)

foreach(zephyr_lib ${ZEPHYR_LIBS_PROPERTY})
  # TODO: Could this become an INTERFACE property of zephyr_interface?
  add_dependencies(${zephyr_lib} offsets_h)
endforeach()

get_property(OUTPUT_FORMAT        GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)

set(zephyr_lnk
  ${LINKERFLAGPREFIX},-Map=${PROJECT_BINARY_DIR}/${KERNEL_MAP_NAME}
  -u_OffsetAbsSyms
  -u_ConfigAbsSyms
  -e__start
  ${LINKERFLAGPREFIX},--start-group
  ${LINKERFLAGPREFIX},--whole-archive
  ${ZEPHYR_LIBS_PROPERTY}
  ${LINKERFLAGPREFIX},--no-whole-archive
  kernel
  ${OFFSETS_O_PATH}
  ${LINKERFLAGPREFIX},--end-group
  ${LIB_INCLUDE_DIR}
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

# Read global variables into local variables
get_property(GKOF GLOBAL PROPERTY GENERATED_KERNEL_OBJECT_FILES)
get_property(GKSF GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES)

# FIXME: Is there any way to get rid of dummy.c?
add_executable(       zephyr_prebuilt dummy.c)
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

  add_executable(       kernel_elf dummy.c ${GKSF})
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
set_target_properties(${logical_target_for_zephyr_elf} PROPERTIES OUTPUT_NAME "zephyr")

add_custom_command(
  TARGET ${logical_target_for_zephyr_elf}
  POST_BUILD
  COMMAND ${PYTHON_EXECUTABLE} $ENV{ZEPHYR_BASE}/scripts/check_link_map.py ${KERNEL_MAP_NAME}
  COMMAND ${CMAKE_OBJCOPY} -S -Oihex   -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_HEX_NAME}
  COMMAND ${CMAKE_OBJCOPY} -S -Obinary -R .comment -R COMMON -R .eh_frame  ${KERNEL_ELF_NAME}    ${KERNEL_BIN_NAME}
  COMMAND ${CMAKE_OBJCOPY} --srec-len 1 --output-target=srec               ${KERNEL_ELF_NAME}    ${KERNEL_S19_NAME}
  COMMAND ${CMAKE_OBJDUMP} -S                                              ${KERNEL_ELF_NAME} >  ${KERNEL_LST_NAME}
  COMMAND ${CMAKE_STRIP}   --strip-all                                     ${KERNEL_ELF_NAME} -o ${KERNEL_STRIP_NAME}
  COMMENT "Generating zephyr.{hex,bin,lst,strip} from zephyr.elf for board: ${BOARD}"
  # NB: COMMENT only works for some CMake-Generators
)

if(EMU_PLATFORM)
  # TODO: Support more emulators
  add_subdirectory(cmake/qemu)
else()
  add_custom_target(run COMMENT "\
    ===================================================\
	Emulation/Simulation not supported with this board.\
	===================================================\
    ")
endif()

include(cmake/ccache.cmake)
