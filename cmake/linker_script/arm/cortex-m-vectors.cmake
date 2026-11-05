# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ISR_TABLES_LOCAL_DECLARATION AND CONFIG_GEN_IRQ_VECTOR_TABLE)
  # vector_table.S emits sixteen 32-bit exception entries on every Cortex-M ABI.
  set(cortex_m_exception_count 16)
  set(cortex_m_vector_bytes 4)
  math(EXPR cortex_m_exception_bytes "${cortex_m_exception_count} * ${cortex_m_vector_bytes}")
  math(EXPR cortex_m_vector_offset
    "(${CONFIG_ROM_START_OFFSET} + ${VECTOR_ALIGN} - 1) & ~(${VECTOR_ALIGN} - 1)"
  )
  math(EXPR cortex_m_irq_offset "${cortex_m_vector_offset} + ${cortex_m_exception_bytes}")
  math(EXPR cortex_m_irq_bytes
    "(${CONFIG_NUM_IRQS} - ${CONFIG_GEN_IRQ_START_VECTOR}) * ${cortex_m_vector_bytes}"
  )
  math(EXPR cortex_m_vector_end "${cortex_m_irq_offset} + ${cortex_m_irq_bytes}")

  zephyr_linker_section_configure(SECTION .rom_start
    INPUT ".exc_vector_table*" OFFSET ${cortex_m_vector_offset}
    ALIGN ${VECTOR_ALIGN} KEEP FIRST SYMBOLS _vector_start
    MIN_SIZE ${cortex_m_exception_bytes} MAX_SIZE ${cortex_m_exception_bytes} PRIO 50
  )
  zephyr_linker_section_configure(SECTION .rom_start
    INPUT ".isr_pre0.vectors" OFFSET ${cortex_m_irq_offset}
    ALIGN ${cortex_m_vector_bytes} KEEP
    MIN_SIZE ${cortex_m_irq_bytes} MAX_SIZE ${cortex_m_irq_bytes}
    PASS NOT LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL PRIO 60
  )
  # Generated IRQ records use priorities >= 100 and offsets from .rom_start.
  # The vector end precedes any null-pointer guard padding, as in the GNU script.
  set(cortex_m_vector_padding)
  if(DEFINED VECTOR_TABLE_MIN_SIZE)
    math(EXPR cortex_m_vector_padding_bytes
      "${VECTOR_TABLE_MIN_SIZE} - (${cortex_m_vector_end} - ${cortex_m_vector_offset})"
    )
    if(cortex_m_vector_padding_bytes GREATER 0)
      set(cortex_m_vector_padding MIN_SIZE ${cortex_m_vector_padding_bytes})
    endif()
  endif()
  zephyr_linker_section_configure(SECTION .rom_start
    OFFSET ${cortex_m_vector_end} SYMBOLS _vector_end PRIO 2147483647
    ${cortex_m_vector_padding}
  )
else()
  if(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
    set(cortex_m_manual_vectors ".isr_vector_table*")
  else()
    set(cortex_m_manual_vectors ".gnu.linkonce.irq_vector_table*")
  endif()
  zephyr_linker_section_configure(SECTION .rom_start
    INPUT ".exc_vector_table*" ${cortex_m_manual_vectors} ".vectors"
    OFFSET ${CONFIG_ROM_START_OFFSET}
    KEEP FIRST SYMBOLS _vector_start _vector_end ALIGN ${VECTOR_ALIGN} PRIO 50
    ${vector_table_min_size_arg}
  )
endif()
