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
  set(STEERING_FILE)
  set(STEERING_C)
  set(STEERING_FILE_ARG)
  set(STEERING_C_ARG)
  set(linker_pass_define_list ${linker_pass_define})
  set(cmake_linker_script_settings
      ${PROJECT_BINARY_DIR}/include/generated/ld_script_settings_${linker_pass_define}.cmake
  )

  if("LINKER_ZEPHYR_FINAL" IN_LIST linker_pass_define_list)
    set(STEERING_FILE ${CMAKE_CURRENT_BINARY_DIR}/armlink_symbol_steering.steer)
    set(STEERING_C ${CMAKE_CURRENT_BINARY_DIR}/armlink_symbol_steering.c)
    set(STEERING_FILE_ARG "-DSTEERING_FILE=${STEERING_FILE}")
    set(STEERING_C_ARG "-DSTEERING_C=${STEERING_C}")
  endif()

  zephyr_linker_generate_linker_settings_file(${cmake_linker_script_settings})

  add_custom_command(
    OUTPUT ${linker_script_gen}
           ${STEERING_FILE}
           ${STEERING_C}
    COMMAND ${CMAKE_COMMAND}
      -C ${cmake_linker_script_settings}
      -DPASS="${linker_pass_define}"
      ${STEERING_FILE_ARG}
      ${STEERING_C_ARG}
      -DOUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${linker_script_gen}
      -P ${ZEPHYR_BASE}/cmake/linker/armlink/scatter_script.cmake
    DEPENDS ${DEVICE_API_LD_TARGET}
            ${cmake_linker_script_settings}
  )

  if("LINKER_ZEPHYR_FINAL" IN_LIST linker_pass_define_list)
    add_library(armlink_steering OBJECT ${STEERING_C})
    target_link_libraries(armlink_steering PRIVATE zephyr_interface)
  endif()
endmacro()

function(toolchain_ld_link_elf)
  cmake_parse_arguments(
    TOOLCHAIN_LD_LINK_ELF                                     # prefix of output variables
    ""                                                        # list of names of the boolean arguments
    "TARGET_ELF;OUTPUT_MAP;LINKER_SCRIPT"                     # list of names of scalar arguments
    "LIBRARIES_PRE_SCRIPT;LIBRARIES_POST_SCRIPT;DEPENDENCIES" # list of names of list arguments
    ${ARGN}                                                   # input args to parse
  )

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
    $<TARGET_OBJECTS:armlink_steering>
    --edit=${CMAKE_CURRENT_BINARY_DIR}/armlink_symbol_steering.steer
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
