# SPDX-License-Identifier: Apache-2.0

set_property(TARGET linker PROPERTY devices_start_symbol "Image$$device$$Base")

find_program(CMAKE_LINKER ${CROSS_COMPILE}armlink PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

add_custom_target(armlink)

function(toolchain_ld_force_undefined_symbols)
  foreach(symbol ${ARGN})
    zephyr_link_libraries(--undefined=${symbol})
  endforeach()
endfunction()

macro(configure_linker_script linker_script_gen linker_pass_define)
  get_filename_component(script_path "${linker_script_gen}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}"
  )
  string(SHA256 script_id "${script_path}")
  get_filename_component(script_name ${linker_script_gen} NAME_WE)
  set(steering_file ${CMAKE_CURRENT_BINARY_DIR}/${script_name}.steer)
  set(steering_c ${CMAKE_CURRENT_BINARY_DIR}/${script_name}_steering.c)
  set(keep_file ${CMAKE_CURRENT_BINARY_DIR}/${script_name}.keep)
  set(steering_target armlink_steering_${script_id})
  set(cmake_linker_script_settings
    ${PROJECT_BINARY_DIR}/include/generated/ld_script_settings_${script_id}.cmake
  )

  zephyr_linker_generate_linker_settings_file(${cmake_linker_script_settings})

  add_custom_command(
    OUTPUT ${linker_script_gen}
           ${steering_file}
           ${steering_c}
           ${keep_file}
    COMMAND ${CMAKE_COMMAND}
      -C ${cmake_linker_script_settings}
      "-DPASS=${linker_pass_define}"
      -DSTEERING_FILE=${steering_file}
      -DSTEERING_C=${steering_c}
      -DKEEP_FILE=${keep_file}
      -DOUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${linker_script_gen}
      -P ${ZEPHYR_BASE}/cmake/linker/armlink/scatter_script.cmake
    DEPENDS ${ZEPHYR_BASE}/cmake/linker/armlink/scatter_script.cmake
            ${ZEPHYR_BASE}/cmake/linker/linker_script_common.cmake
            ${DEVICE_API_LD_TARGET}
            ${cmake_linker_script_settings}
            ${ARGN}
    VERBATIM
  )

  add_library(${steering_target} OBJECT ${steering_c})
  target_link_libraries(${steering_target} PRIVATE zephyr_interface)
  set_property(GLOBAL PROPERTY ARMLINK_STEERING_${script_id} ${steering_target})
  set_property(GLOBAL PROPERTY ARMLINK_EDIT_${script_id} ${steering_file})
  set_property(GLOBAL PROPERTY ARMLINK_KEEP_${script_id} ${keep_file})
endmacro()

function(toolchain_ld_link_elf)
  cmake_parse_arguments(
    TOOLCHAIN_LD_LINK_ELF                                     # prefix of output variables
    ""                                                        # list of names of the boolean arguments
    "TARGET_ELF;OUTPUT_MAP;LINKER_SCRIPT"                     # list of names of scalar arguments
    "LIBRARIES_PRE_SCRIPT;LIBRARIES_POST_SCRIPT;DEPENDENCIES" # list of names of list arguments
    ${ARGN}                                                   # input args to parse
  )

  get_filename_component(script_path "${TOOLCHAIN_LD_LINK_ELF_LINKER_SCRIPT}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}"
  )
  string(SHA256 script_id "${script_path}")
  get_property(steering_target GLOBAL PROPERTY ARMLINK_STEERING_${script_id})
  get_property(steering_file GLOBAL PROPERTY ARMLINK_EDIT_${script_id})
  get_property(keep_file GLOBAL PROPERTY ARMLINK_KEEP_${script_id})
  if(NOT steering_target)
    message(FATAL_ERROR "Missing steering files for ${TOOLCHAIN_LD_LINK_ELF_LINKER_SCRIPT}")
  endif()

  foreach(lib ${ZEPHYR_LIBS_PROPERTY})
    if(NOT ${lib} STREQUAL arch__arm__core__cortex_m)
      list(APPEND ZEPHYR_LIBS_OBJECTS $<TARGET_OBJECTS:${lib}>)
      list(APPEND ZEPHYR_LIBS_OBJECTS $<TARGET_PROPERTY:${lib},LINK_LIBRARIES>)
    endif()
  endforeach()

  target_link_libraries(
    ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_PRE_SCRIPT}
    --scatter=${TOOLCHAIN_LD_LINK_ELF_LINKER_SCRIPT}
    ${TOOLCHAIN_LD_LINK_ELF_LIBRARIES_POST_SCRIPT}
    $<TARGET_OBJECTS:arch__arm__core__cortex_m>
    --map --list=${TOOLCHAIN_LD_LINK_ELF_OUTPUT_MAP}
    ${ZEPHYR_LIBS_OBJECTS}
    kernel
    $<TARGET_OBJECTS:${OFFSETS_LIB}>
    --library_type=microlib
    --entry=$<TARGET_PROPERTY:linker,ENTRY>
    "--keep=\"*.o(.init_*)\""
    "--keep=\"*.o(.device_*)\""
    $<TARGET_OBJECTS:${steering_target}>
    --edit=${steering_file}
    --via=${keep_file}
    # Resolving symbols using generated steering files will emit the warnings 6331 and 6332.
    # Steering files are used because we want to be able to use `__device_end` instead of `Image$$device$$Limit`.
    # Thus silence those two warnings.
    --diag_suppress=6331,6332
    # The scatter file is generated, and thus sometimes input sections are specified
    # even though there will be no such sections found in the libraries linked.
    --diag_suppress=6314
    # We use empty execution sections in order to define custom symbols, such as
    # __kernel_ram_x symbols, but nothing will go in those section, so silence
    # the warning. Note, marking the section EMPTY causes armlink to reserve the
    # address which in some cases leads to overlapping section errors.
    --diag_suppress=6312
    # Use of '.gnu.linkonce' sections. Those are used by ld, and # supported by armlink, albeit
    # deprecated there. For current ARMClang support phase, we accept this warning, but we should
    # look into changing to COMDAT groups.
    --diag_suppress=6092
    # Wildcard matching of keep sections, Those are needed for gnu ld, and thus we inherit the same
    # keep flags and apply them to armlink. Consider adjusting keep flags per linker in future.
    --diag_suppress=6319
    # Match pattern for an unused section that is being removed.
    --diag_suppress=6329
    ${TOOLCHAIN_LIBS_OBJECTS}

    ${TOOLCHAIN_LD_LINK_ELF_DEPENDENCIES}
  )
  if(CONFIG_LTO)
    target_link_libraries(${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF} --lto)
  endif()

  # RESOLVE removes undefined public names from armlink's symbol table.
  # Restore their same-link values before generators and ELF checks consume it.
  set(symbol_export ${ZEPHYR_BASE}/scripts/build/armlink_symbol_export.py)
  set_property(TARGET ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF} APPEND PROPERTY LINK_DEPENDS
    ${symbol_export} ${steering_file})
  add_custom_command(TARGET ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF} POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE} ${symbol_export}
      --elf $<TARGET_FILE:${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}>
      --steering ${steering_file}
      --report $<TARGET_FILE:${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}>.armlink-symbols.json
    VERBATIM
  )
  if(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
    target_link_libraries(${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF} --emit_relocs)
    set(finalizer ${ZEPHYR_BASE}/scripts/build/isr_metadata_finalize.py)
    set_property(TARGET ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF} APPEND PROPERTY LINK_DEPENDS
      ${finalizer} ${ZEPHYR_BASE}/scripts/build/gen_isr_tables_parser_local.py)
    add_custom_command(TARGET ${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF} POST_BUILD
      COMMAND ${PYTHON_EXECUTABLE} ${finalizer}
        --elf $<TARGET_FILE:${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}>
        --report $<TARGET_FILE:${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}>.isr-metadata.json
      VERBATIM
    )
  endif()
endfunction(toolchain_ld_link_elf)

# This function will generate the correct CMAKE_C_LINK_EXECUTABLE / CMAKE_CXX_LINK_EXECUTABLE
# rule to ensure that standard c and runtime libraries are correctly placed
# and the end of link invocation and doesn't appear in the middle of the link
# command invocation.
macro(toolchain_linker_finalize)
  set(zephyr_std_libs)
  get_property(link_order TARGET linker PROPERTY link_order_library)
  foreach(lib ${link_order})
    get_property(link_flag TARGET linker PROPERTY ${lib}_library)
    set(zephyr_std_libs "${zephyr_std_libs} ${link_flag}")
  endforeach()

  set(common_link "<LINK_FLAGS> <LINK_LIBRARIES> <OBJECTS> ${zephyr_std_libs} -o <TARGET>")
  set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_C_LINK_FLAGS> ${common_link}")
  set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_CXX_LINK_FLAGS> ${common_link}")
  set(CMAKE_ASM_LINK_EXECUTABLE "<CMAKE_LINKER> <CMAKE_ASM_LINK_FLAGS> ${common_link}")
endmacro()

include(${ZEPHYR_BASE}/cmake/linker/ld/target_relocation.cmake)
include(${ZEPHYR_BASE}/cmake/linker/ld/target_configure.cmake)
