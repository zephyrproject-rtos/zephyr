# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

# Register GNU-format script generation from CMake linker settings.
function(zephyr_linker_generate_cmake_script linker_script_gen linker_pass_define)
  cmake_parse_arguments(SCRIPT "LLD" "" "DEPENDS" ${ARGN})
  string(SHA256 linker_pass_id "${linker_pass_define}")
  set(settings
    ${PROJECT_BINARY_DIR}/include/generated/ld_script_settings_${linker_pass_id}.cmake
  )
  zephyr_linker_generate_linker_settings_file(${settings})

  set(backend_args "")
  if(SCRIPT_LLD)
    set(backend_args -DLINKER_IS_LLD=TRUE)
  endif()
  add_custom_command(
    OUTPUT ${linker_script_gen}
    DEPENDS
      ${SCRIPT_DEPENDS}
      ${settings}
      ${DEVICE_API_LD_TARGET}
      ${ZEPHYR_BASE}/cmake/linker/ld/ld_script.cmake
      ${ZEPHYR_BASE}/cmake/linker/linker_script_common.cmake
    COMMAND ${CMAKE_COMMAND}
      -C ${settings}
      ${backend_args}
      "-DPASS=${linker_pass_define}"
      -DOUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/${linker_script_gen}
      -P ${ZEPHYR_BASE}/cmake/linker/ld/ld_script.cmake
    VERBATIM
  )
endfunction()
