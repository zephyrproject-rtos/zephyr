# SPDX-License-Identifier: Apache-2.0
set_property(TARGET linker PROPERTY devices_start_symbol "__device_start")

if(DEFINED TOOLCHAIN_HOME)
  # When Toolchain home is defined, then we are cross-compiling, so only look
  # for linker in that path, else we are using host tools.
  set(LD_SEARCH_PATH PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_LINKER ${CROSS_COMPILE}ld.bfd ${LD_SEARCH_PATH})
if(NOT CMAKE_LINKER)
  find_program(CMAKE_LINKER ${CROSS_COMPILE}ld ${LD_SEARCH_PATH})
endif()

set_ifndef(LINKERFLAGPREFIX -Wl)

if(NOT "${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "host")
  if(CONFIG_EXCEPTIONS)
    # When building with C++ Exceptions, it is important that crtbegin and crtend
    # are linked at specific locations.
    # The location is so important that we cannot let this be controlled by normal
    # link libraries, instead we must control the link command specifically as
    # part of toolchain.
    set(CMAKE_CXX_LINK_EXECUTABLE
        "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> ${LIBGCC_DIR}/crtbegin.o <OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${LIBGCC_DIR}/crtend.o")
  endif()
endif()

# Run $LINKER_SCRIPT file through the C preprocessor, producing ${linker_script_gen}
# NOTE: ${linker_script_gen} will be produced at build-time; not at configure-time
macro(configure_linker_script linker_script_gen linker_pass_define)
  set(extra_dependencies ${ARGN})

  if(CONFIG_CMAKE_LINKER_GENERATOR)
    add_custom_command(
      OUTPUT ${linker_script_gen}
      COMMAND ${CMAKE_COMMAND}
        -DPASS="${linker_pass_define}"
        -DFORMAT="$<TARGET_PROPERTY:linker,FORMAT>"
        -DENTRY="$<TARGET_PROPERTY:linker,ENTRY>"
        -DMEMORY_REGIONS="$<TARGET_PROPERTY:linker,MEMORY_REGIONS>"
        -DGROUPS="$<TARGET_PROPERTY:linker,GROUPS>"
        -DSECTIONS="$<TARGET_PROPERTY:linker,SECTIONS>"
        -DSECTION_SETTINGS="$<TARGET_PROPERTY:linker,SECTION_SETTINGS>"
        -DSYMBOLS="$<TARGET_PROPERTY:linker,SYMBOLS>"
        -DOUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${linker_script_gen}
        -P ${ZEPHYR_BASE}/cmake/linker/ld/ld_script.cmake
      )
  else()
    set(template_script_defines ${linker_pass_define})
    list(TRANSFORM template_script_defines PREPEND "-D")

    # Only Ninja and Makefile generators support DEPFILE.
    if((CMAKE_GENERATOR STREQUAL "Ninja")
       OR (CMAKE_GENERATOR MATCHES "Makefiles")
    )
      set(linker_script_dep DEPFILE ${PROJECT_BINARY_DIR}/${linker_script_gen}.dep)
    else()
      # TODO: How would the linker script dependencies work for non-linker
      # script generators.
      message(STATUS "Warning; this generator is not well supported. The
    Linker script may not be regenerated when it should.")
      set(linker_script_dep "")
    endif()

    zephyr_get_include_directories_for_lang(C current_includes)
    get_property(current_defines GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES)

    if("${SPARSE}" STREQUAL "y")
      set(ld_command ${REAL_CC})
    else()
      set(ld_command ${CMAKE_C_COMPILER})
    endif()

    add_custom_command(
      OUTPUT ${linker_script_gen}
      DEPENDS
      ${LINKER_SCRIPT}
      ${AUTOCONF_H}
      ${extra_dependencies}
      # NB: 'linker_script_dep' will use a keyword that ends 'DEPENDS'
      ${linker_script_dep}
      COMMAND ${ld_command}
      -x assembler-with-cpp
      ${NOSYSDEF_CFLAG}
      -MD -MF ${linker_script_gen}.dep -MT ${linker_script_gen}
      -D_LINKER
      -D_ASMLANGUAGE
      -imacros ${AUTOCONF_H}
      ${current_includes}
      ${current_defines}
      ${template_script_defines}
      -E ${LINKER_SCRIPT}
      -P # Prevent generation of debug `#line' directives.
      -o ${linker_script_gen}
      VERBATIM
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
      COMMAND_EXPAND_LISTS
    )
  endif()
endmacro()

# Force symbols to be entered in the output file as undefined symbols
function(toolchain_ld_force_undefined_symbols)
  foreach(symbol ${ARGN})
    zephyr_link_libraries(${LINKERFLAGPREFIX},-u,${symbol})
  endforeach()
endfunction()

# Link a target to given libraries with toolchain-specific argument order
#
# Usage:
#   toolchain_ld_link_elf(
#     TARGET_ELF             <target_elf>
#     OUTPUT_MAP             <output_map_file_of_target>
#     LIBRARIES_PRE_SCRIPT   [libraries_pre_script]
#     LINKER_SCRIPT          <linker_script>
#     LIBRARIES_POST_SCRIPT  [libraries_post_script]
#     DEPENDENCIES           [dependencies]
#   )
function(toolchain_ld_link_elf)
  cmake_parse_arguments(
    TOOLCHAIN_LD_LINK_ELF                                     # prefix of output variables
    ""                                                        # list of names of the boolean arguments
    "TARGET_ELF;OUTPUT_MAP;LINKER_SCRIPT"                     # list of names of scalar arguments
    "LIBRARIES_PRE_SCRIPT;LIBRARIES_POST_SCRIPT;DEPENDENCIES" # list of names of list arguments
    ${ARGN}                                                   # input args to parse
  )

  if(${CMAKE_LINKER} STREQUAL "${CROSS_COMPILE}ld.bfd")
    # ld.bfd was found so let's explicitly use that for linking, see #32237
    set(use_linker "-fuse-ld=bfd")
  endif()

  target_link_libraries(
    ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT}
    ${use_linker}
    ${TOPT}
    ${TOOLCHAIN_LD_LINK_ELF_LINKER_SCRIPT}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_POST_SCRIPT}

    ${LINKERFLAGPREFIX},-Map=${TOOLCHAIN_LD_LINK_ELF_OUTPUT_MAP}
    ${LINKERFLAGPREFIX},--whole-archive
    ${ZEPHYR_LIBS_PROPERTY}
    ${LINKERFLAGPREFIX},--no-whole-archive
    kernel
    $<TARGET_OBJECTS:${OFFSETS_LIB}>
    ${LIB_INCLUDE_DIR}
    -L${PROJECT_BINARY_DIR}
    ${TOOLCHAIN_LIBS}

    ${TOOLCHAIN_LD_LINK_ELF_DEPENDENCIES}
  )
endfunction(toolchain_ld_link_elf)

# Load toolchain_ld-family macros
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/target_base.cmake)
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/target_baremetal.cmake)
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/target_cpp.cmake)
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/target_relocation.cmake)
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/target_configure.cmake)
