# Copyright (c) 2019 Intel Corp.
# SPDX-License-Identifier: Apache-2.0

# Find out if we are optimizing for size
get_target_property(zephyr_COMPILE_OPTIONS zephyr_interface INTERFACE_COMPILE_OPTIONS)
if ("-Os" IN_LIST zephyr_COMPILE_OPTIONS)
  zephyr_cc_option(-mpreferred-stack-boundary=2)
else()
  zephyr_compile_definitions(PERF_OPT)
endif()

set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH "i386")
set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-i386")

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  zephyr_compile_options(-Qunused-arguments)

  zephyr_cc_option(
    -m32
    -gdwarf-2
    )
endif()

zephyr_cc_option_ifndef(CONFIG_SSE_FP_MATH -mno-sse)

if(CMAKE_VERBOSE_MAKEFILE)
  set(GENIDT_EXTRA_ARGS --verbose)
else()
  set(GENIDT_EXTRA_ARGS "")
endif()

set(GENIDT ${ZEPHYR_BASE}/arch/x86/gen_idt.py)

define_property(GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH BRIEF_DOCS " " FULL_DOCS " ")

# Use gen_idt.py and objcopy to generate irq_int_vector_map.o,
# irq_vectors_alloc.o, and staticIdt.o from the elf file ${ZEPHYR_PREBUILT_EXECUTABLE}
set(gen_idt_output_files
  ${CMAKE_CURRENT_BINARY_DIR}/irq_int_vector_map.bin
  ${CMAKE_CURRENT_BINARY_DIR}/staticIdt.bin
  ${CMAKE_CURRENT_BINARY_DIR}/irq_vectors_alloc.bin
  )
add_custom_target(
  gen_idt_output
  DEPENDS
  ${gen_idt_output_files}
  )
add_custom_command(
  OUTPUT irq_int_vector_map.bin staticIdt.bin irq_vectors_alloc.bin
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${GENIDT}
  --kernel $<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
  --output-idt staticIdt.bin
  --vector-map irq_int_vector_map.bin
  --output-vectors-alloc irq_vectors_alloc.bin
  ${GENIDT_EXTRA_ARGS}
  DEPENDS ${ZEPHYR_PREBUILT_EXECUTABLE}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

# Must be last so that soc/ can override default exception handlers
add_subdirectory(core)

get_property(OUTPUT_ARCH   GLOBAL PROPERTY PROPERTY_OUTPUT_ARCH)
get_property(OUTPUT_FORMAT GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT)



add_bin_file_to_the_next_link(gen_idt_output staticIdt)
add_bin_file_to_the_next_link(gen_idt_output irq_int_vector_map)
add_bin_file_to_the_next_link(gen_idt_output irq_vectors_alloc)

if(CONFIG_GDT_DYNAMIC)
  # Use gen_gdt.py and objcopy to generate gdt.o from from the elf
  # file ${ZEPHYR_PREBUILT_EXECUTABLE}, creating the temp file gdt.bin along the
  # way.
  #
  # ${ZEPHYR_PREBUILT_EXECUTABLE}.elf -> gdt.bin -> gdt.o
  add_custom_target(
    gdt_bin_target
    DEPENDS
    gdt.bin
  )
  add_custom_command(
    OUTPUT gdt.bin
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/arch/x86/gen_gdt.py
    --kernel $<TARGET_FILE:${ZEPHYR_PREBUILT_EXECUTABLE}>
    --output-gdt gdt.bin
    $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--verbose>
    DEPENDS ${ZEPHYR_PREBUILT_EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

  add_bin_file_to_the_next_link(gdt_bin_target gdt)
endif()
