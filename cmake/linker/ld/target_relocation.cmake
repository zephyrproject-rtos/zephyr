# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_relocation)
  set(MEM_RELOCATAION_LD   "${PROJECT_BINARY_DIR}/include/generated/linker_relocate.ld")
  set(MEM_RELOCATAION_SRAM_DATA_LD
       "${PROJECT_BINARY_DIR}/include/generated/linker_sram_data_relocate.ld")
  set(MEM_RELOCATAION_SRAM_BSS_LD
       "${PROJECT_BINARY_DIR}/include/generated/linker_sram_bss_relocate.ld")
  set(MEM_RELOCATAION_CODE "${PROJECT_BINARY_DIR}/code_relocation.c")

  add_custom_command(
    OUTPUT ${MEM_RELOCATAION_CODE} ${MEM_RELOCATAION_LD}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/scripts/gen_relocate_app.py
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    -d ${APPLICATION_BINARY_DIR}
    -i '$<TARGET_PROPERTY:code_data_relocation_target,COMPILE_DEFINITIONS>'
    -o ${MEM_RELOCATAION_LD}
    -s ${MEM_RELOCATAION_SRAM_DATA_LD}
    -b ${MEM_RELOCATAION_SRAM_BSS_LD}
    -c ${MEM_RELOCATAION_CODE}
    DEPENDS app kernel ${ZEPHYR_LIBS_PROPERTY}
    )

  add_library(code_relocation_source_lib  STATIC ${MEM_RELOCATAION_CODE})
  target_link_libraries(code_relocation_source_lib zephyr_interface)
endmacro()
