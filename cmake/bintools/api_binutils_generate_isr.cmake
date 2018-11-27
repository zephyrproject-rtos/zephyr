macro(toolchain_bintools_generate_isr)

  if(CONFIG_GEN_ISR_TABLES)
    if(CONFIG_GEN_SW_ISR_TABLE)
      list(APPEND GEN_ISR_TABLE_EXTRA_ARG --sw-isr-table)
    endif()

    if(CONFIG_GEN_IRQ_VECTOR_TABLE)
      list(APPEND GEN_ISR_TABLE_EXTRA_ARG --vector-table)
    endif()

    # isr_tables.c is generated from zephyr_prebuilt by
    # gen_isr_tables.py
    add_custom_command(
      OUTPUT isr_tables.c
      COMMAND ${CMAKE_OBJCOPY}
      -I ${OUTPUT_FORMAT}
      -O binary
      --only-section=.intList
      $<TARGET_FILE:zephyr_prebuilt>
      isrList.bin
      COMMAND ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/arch/common/gen_isr_tables.py
      --output-source isr_tables.c
      --kernel $<TARGET_FILE:zephyr_prebuilt>
      --intlist isrList.bin
      $<$<BOOL:${CONFIG_BIG_ENDIAN}>:--big-endian>
      $<$<BOOL:${CMAKE_VERBOSE_MAKEFILE}>:--debug>
      ${GEN_ISR_TABLE_EXTRA_ARG}
      DEPENDS zephyr_prebuilt
    )
    set_property(GLOBAL APPEND PROPERTY GENERATED_KERNEL_SOURCE_FILES isr_tables.c)
  endif()

endmacro()
