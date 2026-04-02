# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SRAM_VECTOR_TABLE)
  # The vector table must be placed at the start of SRAM.
  # Reserve the first sizeof(<vector table>) bytes for this purpose.
  if(CONFIG_CMAKE_LINKER_GENERATOR)
    # We use MIN_SIZE to perform the equivalent of GNU ld `. += <XXX>;`
    # but we have to compute the size of the vector table manually as
    # it must be a bytes count (can't be `_vector_end - _vector_start`).
    # Fortunately, the vector table size can be easily computed:
    # 16 words for the architectural exceptions plus one word per IRQ
    # (each word is 32 bits = 4 bytes).
    math(EXPR vect_tbl_size "(16 + ${CONFIG_NUM_IRQS}) * 4")
    zephyr_linker_section(NAME .st_stm32f0x_vt TYPE NOLOAD GROUP RAM_REGION ADDRESS 0x20000000 NOINPUT NOINIT)
    zephyr_linker_section_configure(SECTION .st_stm32f0x_vt MIN_SIZE ${vect_tbl_size} SYMBOLS _ram_vector_start _ram_vector_end)
  else()
    zephyr_linker_sources(
      RAM_SECTIONS
      SORT_KEY 0
      sram_vector_table.ld
    )
  endif()
endif()

zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(soc.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
