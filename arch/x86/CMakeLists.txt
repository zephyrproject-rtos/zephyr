
# Find out if we are optimizing for size
get_target_property(zephyr_COMPILE_OPTIONS zephyr COMPILE_OPTIONS)
if ("-Os" IN_LIST zephyr_COMPILE_OPTIONS)
  target_compile_definitions(zephyr PUBLIC PERF_OPT)
else()
  target_cc_option(zephyr PUBLIC -mpreferred-stack-boundary=2)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  if(CONFIG_X86_IAMCU)
    target_cc_option(zephyr PUBLIC -Qunused-arguments)
  else()
    target_cc_option(zephyr PUBLIC -mmcu)
  endif()

  target_cc_option(zephyr PUBLIC -m32)
  target_cc_option(zephyr PUBLIC -gdwarf-2)
endif()

target_cc_option_ifdef(CONFIG_LTO          zephyr PUBLIC -flto)
target_cc_option_ifndef(CONFIG_SSE_FP_MATH zephyr PUBLIC -mno-sse)

if(CMAKE_VERBOSE_MAKEFILE)
  set(GENIDT_EXTRA_ARGS --verbose)
else()
  set(GENIDT_EXTRA_ARGS "")
endif()

set(GENIDT ${PROJECT_SOURCE_DIR}/scripts/gen_idt.py)

define_property(GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH BRIEF_DOCS " " FULL_DOCS " ")
set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH "i386")

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-i386")

# Use gen_idt.py and objcopy to generate irq_int_vector_map.o and
# staticIdt.o from the elf file zephyr_prebuilt
add_custom_command(
  OUTPUT irq_int_vector_map.bin staticIdt.bin
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${GENIDT}
  --kernel $<TARGET_FILE:zephyr_prebuilt>
  --output-idt staticIdt.bin
  --vector-map irq_int_vector_map.bin
  ${GENIDT_EXTRA_ARGS}
  DEPENDS zephyr_prebuilt
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

add_subdirectory_ifdef(CONFIG_GDB_SERVER debug)
add_subdirectory(soc/${SOC_PATH})
# Must be last so that debug/ or soc/ can override default exception
# handlers
add_subdirectory(core)

get_property(OUTPUT_ARCH   GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH)
get_property(OUTPUT_FORMAT GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/irq_int_vector_map.o
  COMMAND
  ${CMAKE_OBJCOPY}
  -I binary
  -B ${OUTPUT_ARCH}
  -O ${OUTPUT_FORMAT}
  --rename-section .data=irq_int_vector_map
  irq_int_vector_map.bin
  irq_int_vector_map.o
  DEPENDS irq_int_vector_map.bin
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/staticIdt.o
  COMMAND
  ${CMAKE_OBJCOPY}
  -I binary
  -B ${OUTPUT_ARCH}
  -O ${OUTPUT_FORMAT}
  --rename-section .data=staticIdt
  staticIdt.bin
  staticIdt.o
  DEPENDS staticIdt.bin
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )


add_custom_target(irq_int_vector_map_o DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/irq_int_vector_map.o)
add_custom_target(staticIdt_o          DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/staticIdt.o)

add_library(irq_int_vector_map STATIC IMPORTED GLOBAL)
add_library(staticIdt          STATIC IMPORTED GLOBAL)

set_property(TARGET irq_int_vector_map PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/irq_int_vector_map.o)
set_property(TARGET staticIdt          PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/staticIdt.o)

add_dependencies(irq_int_vector_map irq_int_vector_map_o)
add_dependencies(staticIdt          staticIdt_o)

set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_OBJECT_FILES irq_int_vector_map)
set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_OBJECT_FILES staticIdt)

if(CONFIG_X86_MMU)
  # Use gen_mmu.py and objcopy to generate mmu_tables.o from from the
  # elf file zephyr_prebuilt, creating the temp files mmu_tables.bin
  # and mmulist.bin along the way.
  #
  # zephyr_prebuilt.elf -> mmulist.bin -> mmu_tables.bin -> mmu_tables.o
  add_custom_command(
    OUTPUT mmulist.bin
    COMMAND
    ${CMAKE_OBJCOPY}
    -I ${OUTPUT_FORMAT}
    -O binary
    -j mmulist
    $<TARGET_FILE:zephyr_prebuilt>
    mmulist.bin
    DEPENDS zephyr_prebuilt
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_command(
    OUTPUT mmu_tables.bin
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/scripts/gen_mmu.py
    -i mmulist.bin
    -o mmu_tables.bin
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:-v>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS mmulist.bin
    )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mmu_tables.o
    COMMAND
    ${CMAKE_OBJCOPY}
    -I binary
    -B ${OUTPUT_ARCH}
    -O ${OUTPUT_FORMAT}
    --rename-section .data=.mmu_data
    mmu_tables.bin
    mmu_tables.o
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS mmu_tables.bin
    )

  add_custom_target(  mmu_tables_o DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mmu_tables.o)
  add_library(        mmu_tables STATIC IMPORTED GLOBAL)
  set_property(TARGET mmu_tables PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/mmu_tables.o)
  add_dependencies(   mmu_tables mmu_tables_o)
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_OBJECT_FILES mmu_tables)
endif()

if(CONFIG_GDT_DYNAMIC)
  # Use gen_gdt.py and objcopy to generate gdt.o from from the elf
  # file zephyr_prebuilt, creating the temp file gdt.bin along the
  # way.
  #
  # zephyr_prebuilt.elf -> gdt.bin -> gdt.o
  add_custom_command(
    OUTPUT gdt.bin
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${PROJECT_SOURCE_DIR}/scripts/gen_gdt.py
    --kernel $<TARGET_FILE:zephyr_prebuilt>
    --output-gdt gdt.bin
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS zephyr_prebuilt
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gdt.o
    COMMAND
    ${CMAKE_OBJCOPY}
    -I binary
    -B ${OUTPUT_ARCH}
    -O ${OUTPUT_FORMAT}
    --rename-section .data=.gdt_ram_data
    gdt.bin
    gdt.o
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS gdt.bin
    )

  add_custom_target(  gdt_o DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gdt.o)
  add_library(        gdt STATIC IMPORTED GLOBAL)
  set_property(TARGET gdt PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/gdt.o)
  add_dependencies(   gdt gdt_o)
  set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_OBJECT_FILES gdt)
endif()
