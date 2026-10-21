# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

function(build_relocation)
  execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} --target relocation_output
    COMMAND_ERROR_IS_FATAL ANY
  )
endfunction()

execute_process(
  COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_LIST_DIR}/fixture -B ${BUILD_DIR}
    -G ${GENERATOR}
    -DZEPHYR_BASE=${ZEPHYR_BASE}
    -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
  COMMAND_ERROR_IS_FATAL ANY
)

set(dictionary ${BUILD_DIR}/relocation_dict.txt)
set(source ${BUILD_DIR}/relocated.c)
set(generated ${BUILD_DIR}/include/generated)
set(linker_script ${generated}/linker_relocate.ld)
set(data_script ${generated}/linker_sram_data_relocate.ld)
set(bss_script ${generated}/linker_sram_bss_relocate.ld)
set(relocation_code ${BUILD_DIR}/code_relocation.c)

build_relocation()
file(READ ${data_script} data)
file(READ ${bss_script} bss)
file(READ ${relocation_code} code)
if(NOT data MATCHES "[.]data[.]relocated_data" OR
   NOT "${data}${bss}" MATCHES "[.]bss[.]relocated_bss" OR
   code MATCHES "__ram_(data|bss)")
  message(FATAL_ERROR "Default RAM data/BSS did not use the standard initialization path")
endif()

# Changing only the dictionary must move data and BSS into SRAM2.
file(WRITE ${dictionary} "SRAM2:COPY:${source},\n")
build_relocation()
file(READ ${data_script} data)
file(READ ${bss_script} bss)
file(READ ${linker_script} linker)
file(READ ${relocation_code} code)
if(NOT data STREQUAL "" OR NOT bss STREQUAL "" OR
   NOT linker MATCHES "[.]sram2_data_reloc" OR NOT linker MATCHES "[.]data[.]relocated_data" OR
   NOT linker MATCHES "[.]sram2_bss_reloc" OR NOT linker MATCHES "[.]bss[.]relocated_bss" OR
   NOT code MATCHES "__sram2_data" OR NOT code MATCHES "__sram2_bss")
  message(FATAL_ERROR "Dictionary-only change did not regenerate the SRAM2 outputs")
endif()

# Rebuilding a relocated object must also refresh the generated section list.
file(APPEND ${source} "\nint added_function(void) { return relocated_bss; }\n")
build_relocation()
file(READ ${linker_script} linker)
if(NOT linker MATCHES "[.]text[.]added_function")
  message(FATAL_ERROR "Rebuilt object did not refresh relocated sections")
endif()
