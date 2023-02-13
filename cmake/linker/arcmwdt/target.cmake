# SPDX-License-Identifier: Apache-2.0
set_property(TARGET linker PROPERTY devices_start_symbol "__device_start")

find_program(CMAKE_LINKER ${CROSS_COMPILE}lldac PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

# the prefix to transfer linker options from compiler
set_ifndef(LINKERFLAGPREFIX -Wl,)

# Run $LINKER_SCRIPT file through the C preprocessor, producing ${linker_script_gen}
# NOTE: ${linker_script_gen} will be produced at build-time; not at configure-time
macro(configure_linker_script linker_script_gen linker_pass_define)
  set(extra_dependencies ${ARGN})
  set(template_script_defines ${linker_pass_define})
  list(TRANSFORM template_script_defines PREPEND "-D")

  # Different generators deal with depfiles differently.
  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    # Note that the IMPLICIT_DEPENDS option is currently supported only
    # for Makefile generators and will be ignored by other generators.
    set(linker_script_dep IMPLICIT_DEPENDS C ${LINKER_SCRIPT})
  elseif(CMAKE_GENERATOR STREQUAL "Ninja")
    # Using DEPFILE with other generators than Ninja is an error.
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

# the command to generate linker file from template
  add_custom_command(
    OUTPUT ${linker_script_gen}
    DEPENDS
    ${LINKER_SCRIPT}
    ${AUTOCONF_H}
    ${extra_dependencies}
    # NB: 'linker_script_dep' will use a keyword that ends 'DEPENDS'
    ${linker_script_dep}
    COMMAND ${CMAKE_C_COMPILER}
    -x c
    ${NOSYSDEF_CFLAG}
    -Hnocopyr
    -MD -MF ${linker_script_gen}.dep -MT ${linker_script_gen}
    -D_LINKER
    -D_ASMLANGUAGE
    -imacros ${AUTOCONF_H}
    ${current_includes}
    ${current_defines}
    ${template_script_defines}
    ${LINKER_SCRIPT}
    -E
    -o ${linker_script_gen}
    VERBATIM
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND_EXPAND_LISTS
)
endmacro()

# Force symbols to be entered in the output file as undefined symbols
function(toolchain_ld_force_undefined_symbols)
  foreach(symbol ${ARGN})
    zephyr_link_libraries(${LINKERFLAGPREFIX}-u${symbol})
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
    ${LINKERFLAGPREFIX}-T${TOOLCHAIN_LD_LINK_ELF_LINKER_SCRIPT}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_POST_SCRIPT}
    ${LINKERFLAGPREFIX}--gc-sections
    ${LINKERFLAGPREFIX}--entry=__start
    ${LINKERFLAGPREFIX}--Map=${TOOLCHAIN_LD_LINK_ELF_OUTPUT_MAP}
    ${LINKERFLAGPREFIX}--whole-archive
    ${ZEPHYR_LIBS_PROPERTY}
    ${LINKERFLAGPREFIX}--no-whole-archive
    kernel
    $<TARGET_OBJECTS:${OFFSETS_LIB}>
    ${LIB_INCLUDE_DIR}
    -L${PROJECT_BINARY_DIR}
    ${TOOLCHAIN_LIBS}

    ${TOOLCHAIN_LD_LINK_ELF_DEPENDENCIES}
  )
endfunction(toolchain_ld_link_elf)

# linker options of temporary linkage for code generation
macro(toolchain_ld_baremetal)
  zephyr_ld_options(
    -Hlld
    -Hnosdata
    -Xtimer0 # to suppress the warning message
    -Hnoxcheck_obj
    -Hnocplus
    -Hhostlib=
    -Hheap=0
    -Hnoivt
    -Hnocrt
  )

  # There are two options:
  # - We have full MWDT libc support and we link MWDT libc - this is default
  #   behavior and we don't need to do something for that.
  # - We use minimal libc provided by Zephyr itself. In that case we must not
  #   link MWDT libc, but we still need to link libmw
  if(CONFIG_MINIMAL_LIBC)
    zephyr_ld_options(
      -Hnolib
      -Hldopt=-lmw
    )
  endif()

  # Funny thing is if this is set to =error, some architectures will
  # skip this flag even though the compiler flag check passes
  # (e.g. ARC and Xtensa). So warning should be the default for now.
  #
  # Skip this for native application as Zephyr only provides
  # additions to the host toolchain linker script. The relocation
  # sections (.rel*) requires us to override those provided
  # by host toolchain. As we can't account for all possible
  # combination of compiler and linker on all machines used
  # for development, it is better to turn this off.
  #
  # CONFIG_LINKER_ORPHAN_SECTION_PLACE is to place the orphan sections
  # without any warnings or errors, which is the default behavior.
  # So there is no need to explicitly set a linker flag.
  if(CONFIG_LINKER_ORPHAN_SECTION_WARN)
    message(WARNING "MWDT toolchain does not support
           CONFIG_LINKER_ORPHAN_SECTION_WARN")
  elseif(CONFIG_LINKER_ORPHAN_SECTION_ERROR)
    zephyr_ld_options(
      ${LINKERFLAGPREFIX}--orphan-handling=error)
  endif()
endmacro()

# base linker options
macro(toolchain_ld_base)
  if(NOT PROPERTY_LINKER_SCRIPT_DEFINES)
    set_property(GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__MWDT_LINKER_CMD__)
  endif()

  # Sort the common symbols and each input section by alignment
  # in descending order to minimize padding between these symbols.
  zephyr_ld_option_ifdef(
    CONFIG_LINKER_SORT_BY_ALIGNMENT
    ${LINKERFLAGPREFIX}--sort-section=alignment
  )
endmacro()

# generate linker script snippets from configure files
macro(toolchain_ld_configure_files)
  configure_file(
       $ENV{ZEPHYR_BASE}/include/zephyr/arch/common/app_data_alignment.ld
       ${PROJECT_BINARY_DIR}/include/generated/app_data_alignment.ld)

  configure_file(
       $ENV{ZEPHYR_BASE}/include/zephyr/linker/app_smem.ld
       ${PROJECT_BINARY_DIR}/include/generated/app_smem.ld)

  configure_file(
       $ENV{ZEPHYR_BASE}/include/zephyr/linker/app_smem_aligned.ld
       ${PROJECT_BINARY_DIR}/include/generated/app_smem_aligned.ld)

  configure_file(
       $ENV{ZEPHYR_BASE}/include/zephyr/linker/app_smem_unaligned.ld
       ${PROJECT_BINARY_DIR}/include/generated/app_smem_unaligned.ld)
endmacro()

# link C++ libraries
macro(toolchain_ld_cpp)
  zephyr_link_libraries(
    -Hcplus
  )
endmacro()

# use linker for relocation
macro(toolchain_ld_relocation)
  set(MEM_RELOCATION_LD   "${PROJECT_BINARY_DIR}/include/generated/linker_relocate.ld")
  set(MEM_RELOCATION_SRAM_DATA_LD
       "${PROJECT_BINARY_DIR}/include/generated/linker_sram_data_relocate.ld")
  set(MEM_RELOCATION_SRAM_BSS_LD
       "${PROJECT_BINARY_DIR}/include/generated/linker_sram_bss_relocate.ld")
  set(MEM_RELOCATION_CODE "${PROJECT_BINARY_DIR}/code_relocation.c")

  add_custom_command(
    OUTPUT ${MEM_RELOCATION_CODE} ${MEM_RELOCATION_LD}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/build/gen_relocate_app.py
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    -d ${APPLICATION_BINARY_DIR}
    -i \"$<TARGET_PROPERTY:code_data_relocation_target,COMPILE_DEFINITIONS>\"
    -o ${MEM_RELOCATION_LD}
    -s ${MEM_RELOCATION_SRAM_DATA_LD}
    -b ${MEM_RELOCATION_SRAM_BSS_LD}
    -c ${MEM_RELOCATION_CODE}
    DEPENDS app kernel ${ZEPHYR_LIBS_PROPERTY}
    )

  add_library(code_relocation_source_lib  STATIC ${MEM_RELOCATION_CODE})
  target_include_directories(code_relocation_source_lib PRIVATE
	${ZEPHYR_BASE}/kernel/include ${ARCH_DIR}/${ARCH}/include)
  target_link_libraries(code_relocation_source_lib zephyr_interface)
endmacro()
