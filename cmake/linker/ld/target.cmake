# SPDX-License-Identifier: Apache-2.0
set_property(TARGET linker PROPERTY devices_start_symbol "_device_list_start")

find_package(GnuLd REQUIRED)
set(CMAKE_LINKER ${GNULD_LINKER})

set_ifndef(LINKERFLAGPREFIX -Wl)

if((${CMAKE_LINKER} STREQUAL "${CROSS_COMPILE}ld.bfd") OR
   ${GNULD_LINKER_IS_BFD})
  # ld.bfd was found so let's explicitly use that for linking, see #32237
  list(APPEND TOOLCHAIN_LD_FLAGS -fuse-ld=bfd)
  list(APPEND CMAKE_REQUIRED_FLAGS -fuse-ld=bfd)
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
endif()

# Run $LINKER_SCRIPT file through the C preprocessor, producing ${linker_script_gen}
# NOTE: ${linker_script_gen} will be produced at build-time; not at configure-time
macro(configure_linker_script linker_script_gen linker_pass_define)
  set(extra_dependencies ${ARGN})
  set(cmake_linker_script_settings
      ${PROJECT_BINARY_DIR}/include/generated/ld_script_settings_${linker_pass_define}.cmake
  )

  if(CONFIG_CMAKE_LINKER_GENERATOR)

    zephyr_linker_generate_linker_settings_file(${cmake_linker_script_settings})

    add_custom_command(
      OUTPUT ${linker_script_gen}
      DEPENDS
        ${extra_dependencies}
        ${cmake_linker_script_settings}
        ${DEVICE_API_LD_TARGET}
      COMMAND ${CMAKE_COMMAND}
        -C ${cmake_linker_script_settings}
        -DPASS="${linker_pass_define}"
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
    if(DEFINED SOC_LINKER_SCRIPT)
      cmake_path(GET SOC_LINKER_SCRIPT PARENT_PATH soc_linker_script_includes)
      set(soc_linker_script_includes -I${soc_linker_script_includes})
    endif()

    add_custom_command(
      OUTPUT ${linker_script_gen}
      DEPENDS
      ${LINKER_SCRIPT}
      ${AUTOCONF_H}
      ${extra_dependencies}
      # NB: 'linker_script_dep' will use a keyword that ends 'DEPENDS'
      ${linker_script_dep}
      COMMAND ${CMAKE_C_COMPILER}
      -x assembler-with-cpp
      ${NOSYSDEF_CFLAG}
      -MD -MF ${linker_script_gen}.dep -MT ${linker_script_gen}
      -D_LINKER
      -D_ASMLANGUAGE
      -D__GCC_LINKER_CMD__
      -imacros ${AUTOCONF_H}
      ${current_includes}
      ${soc_linker_script_includes}
      ${template_script_defines}
      -DUSE_PARTITION_MANAGER=$<BOOL:${CONFIG_PARTITION_MANAGER_ENABLED}>
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

  target_link_libraries(
    ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT}
    ${TOPT}
    ${TOOLCHAIN_LD_LINK_ELF_LINKER_SCRIPT}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_POST_SCRIPT}

    ${LINKERFLAGPREFIX},-Map=${TOOLCHAIN_LD_LINK_ELF_OUTPUT_MAP}
    ${LINKERFLAGPREFIX},--whole-archive
    ${WHOLE_ARCHIVE_LIBS}
    ${LINKERFLAGPREFIX},--no-whole-archive
    ${NO_WHOLE_ARCHIVE_LIBS}
    $<TARGET_OBJECTS:${OFFSETS_LIB}>
    -L${PROJECT_BINARY_DIR}

    ${TOOLCHAIN_LD_LINK_ELF_DEPENDENCIES}
  )
endfunction(toolchain_ld_link_elf)

# Function for finalizing link setup after Zephyr configuration has completed.
#
# This function will generate the correct CMAKE_C_LINK_EXECUTABLE / CMAKE_CXX_LINK_EXECUTABLE
# rule to ensure that standard c and runtime libraries are correctly placed
# and the end of link invocation and doesn't appear in the middle of the link
# command invocation.
macro(toolchain_linker_finalize)
  get_property(zephyr_std_libs TARGET linker PROPERTY lib_include_dir)
  get_property(link_order TARGET linker PROPERTY link_order_library)
  foreach(lib ${link_order})
    get_property(link_flag TARGET linker PROPERTY ${lib}_library)
    list(APPEND zephyr_std_libs "${link_flag}")
  endforeach()
  string(REPLACE ";" " " zephyr_std_libs "${zephyr_std_libs}")

  set(link_libraries "<OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${zephyr_std_libs}")
  set(common_link "<LINK_FLAGS> ${link_libraries}")

  set(CMAKE_ASM_LINK_EXECUTABLE "<CMAKE_ASM_COMPILER> <FLAGS> <CMAKE_ASM_LINK_FLAGS> ${common_link}")
  set(CMAKE_C_LINK_EXECUTABLE   "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> ${common_link}")

  set(cpp_link "${common_link}")
  if(NOT "${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "host")
    if(CONFIG_CPP_EXCEPTIONS AND LIBGCC_DIR)
      # When building with C++ Exceptions, it is important that crtbegin and crtend
      # are linked at specific locations.
      set(cpp_link "<LINK_FLAGS> ${LIBGCC_DIR}/crtbegin.o ${link_libraries} ${LIBGCC_DIR}/crtend.o")
    endif()
  endif()
  set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> <CMAKE_CXX_LINK_FLAGS> ${cpp_link}")
endmacro()

# Load toolchain_ld-family macros
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/target_relocation.cmake)
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/target_configure.cmake)
