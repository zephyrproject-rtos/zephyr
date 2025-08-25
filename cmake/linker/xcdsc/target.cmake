# Set Linker Symbol Properties
set_property(TARGET linker PROPERTY devices_start_symbol "_device_list_start")
# Locate linker binary
set_ifndef(LINKERFLAGPREFIX -Wl)
set(XCDSC_BIN_PREFIX xc-dsc-)
find_program(CMAKE_LINKER NAMES ${XCDSC_BIN_PREFIX}ld PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
# Generate linker script using preprocessor
macro(configure_linker_script linker_script_gen linker_pass_define)
    set(template_script_defines ${linker_pass_define})
    list(TRANSFORM template_script_defines PREPEND "-D")
    set(extra_dependencies ${ARGN})
    if(DEFINED SOC_LINKER_SCRIPT)
    cmake_path(GET SOC_LINKER_SCRIPT PARENT_PATH soc_linker_script_includes)
    set(soc_linker_script_includes -I${soc_linker_script_includes})
    endif()
    add_custom_command( OUTPUT
                        ${linker_script_gen}
                        DEPENDS
                        ${LINKER_SCRIPT}
                        ${AUTOCONF_H}
                        ${extra_dependencies}
                        ${linker_script_dep}
                        COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -MD -MF ${linker_script_gen}.dep -MT ${linker_script_gen}
                        -D_LINKER
                        -D_ASM_LANGUAGE
                        -D__XCDSC_LINKER_CMD__
                        -mdfp="${DFP_ROOT}/xc16"
                        -imacros ${AUTOCONF_H}
                        -I${ZEPHYR_BASE}/include
                        -imacros${ZEPHYR_BASE}/include/zephyr/linker/sections.h
                        -imacros${ZEPHYR_BASE}/include/zephyr/linker/linker-defs.h
                        -imacros${ZEPHYR_BASE}/include/zephyr/linker/linker-tool-gcc.h
                        -I${PROJECT_BINARY_DIR}/include/generated
                        -I${XCDSC_TOOLCHAIN_PATH}/include/
                        ${soc_linker_script_includes}
                        ${template_script_defines}
                        -E ${LINKER_SCRIPT} # Preprocess only do not compile
                        -P # Prevent generation of debug `#line' directives.
                        -o ${linker_script_gen}
                        VERBATIM
                        WORKING_DIRECTORY
                        ${PROJECT_BINARY_DIR}
                        COMMAND_EXPAND_LISTS )
endmacro()
# Force Inclusion of Undefined Symbols
# Force symbols to be entered in the output file as undefined symbols
function(toolchain_ld_force_undefined_symbols)
  foreach(symbol ${ARGN})
    zephyr_link_libraries(${LINKERFLAGPREFIX},-u,${symbol})
  endforeach()
endfunction()

# Define final ELF Linking Rule
function(toolchain_ld_link_elf)
cmake_parse_arguments(
    TOOLCHAIN_LD_LINK_ELF                                     # prefix of output variables
    ""                                                        # list of names of the boolean arguments
    "TARGET_ELF;OUTPUT_MAP;LINKER_SCRIPT"                     # list of names of scalar arguments
    "LIBRARIES_PRE_SCRIPT;LIBRARIES_POST_SCRIPT;DEPENDENCIES" # list of names of list arguments
    ${ARGN}                                                   # input args to parse
  )
    file(TOUCH ${PROJECT_BINARY_DIR}/memoryfile.xml)
    target_link_libraries(
    ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT}
    ${TOPT}
    ${TOOLCHAIN_LD_LINK_ELF_LINKER_SCRIPT}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_POST_SCRIPT}
    ${LINKERFLAGPREFIX},-Map=${TOOLCHAIN_LD_LINK_ELF_OUTPUT_MAP}
    ${LINKERFLAGPREFIX},--start-group
    c99-pic30-elf
    ${LINKERFLAGPREFIX},--end-group
    ${LINKERFLAGPREFIX},--report-mem
    ${LINKERFLAGPREFIX},--memorysummary ${PROJECT_BINARY_DIR}/memoryfile.xml
    ${LINKERFLAGPREFIX},--whole-archive
    ${WHOLE_ARCHIVE_LIBS}
    ${LINKERFLAGPREFIX},--no-whole-archive
    ${NO_WHOLE_ARCHIVE_LIBS}
    ${LINKERFLAGPREFIX},-L${PROJECT_BINARY_DIR}
    ${TOOLCHAIN_LD_LINK_ELF_DEPENDENCIES} )
endfunction(toolchain_ld_link_elf)
# Finalize Link Execution Behaviour
macro(toolchain_linker_finalize)
    get_property(link_order TARGET linker PROPERTY link_order_library)
    foreach(lib ${link_order})
        get_property(link_flag TARGET linker PROPERTY ${lib}_library)
        list(APPEND zephyr_std_libs "${link_flag}")
    endforeach()
    string(REPLACE ";" " " zephyr_std_libs "${zephyr_std_libs}")
    set(link_libraries "<OBJECTS> -o <TARGET> <LINK_LIBRARIES> ${zephyr_std_libs}")
    set(common_link "${link_libraries}")
    set(CMAKE_ASM_LINK_EXECUTABLE "${common_link}")
    set(CMAKE_C_LINK_EXECUTABLE   "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> ${common_link}")
endmacro()
# Load toolchain_ld-family macros
include(${ZEPHYR_BASE}/cmake/linker/${LINKER}/target_configure.cmake)
