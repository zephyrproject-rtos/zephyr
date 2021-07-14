set(COMMON_ZEPHYR_LINKER_DIR ${ZEPHYR_BASE}/cmake/linker_script/common)

set_ifndef(region_min_align CONFIG_CUSTOM_SECTION_MIN_ALIGN_SIZE)

# Set alignment to CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE if not set above
# to make linker section alignment comply with MPU granularity.
set_ifndef(region_min_align CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)

# If building without MPU support, use default 4-byte alignment.. if not set abve.
set_ifndef(region_min_align 4)

math(EXPR FLASH_ADDR "${CONFIG_FLASH_BASE_ADDRESS} + ${CONFIG_FLASH_LOAD_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR FLASH_SIZE "${CONFIG_FLASH_SIZE} * 1024 - ${CONFIG_FLASH_LOAD_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
set(RAM_ADDR ${CONFIG_SRAM_BASE_ADDRESS})
math(EXPR RAM_SIZE "${CONFIG_SRAM_SIZE} * 1024" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR IDT_ADDR "${RAM_ADDR} + ${RAM_SIZE}" OUTPUT_FORMAT HEXADECIMAL)

# ToDo: decide on the optimal location for this.
# linker/ld/target.cmake based on arch, or directly in arch and scatter_script.cmake can ignore
zephyr_linker(FORMAT "elf32-littlearm")

zephyr_linker_memory(NAME FLASH    FLAGS rx START ${FLASH_ADDR} SIZE ${FLASH_SIZE})
zephyr_linker_memory(NAME RAM      FLAGS wx START ${RAM_ADDR}   SIZE ${RAM_SIZE})
zephyr_linker_memory(NAME IDT_LIST FLAGS wx START ${IDT_ADDR}   SIZE 2K)

#zephyr_region(NAME FLASH ALIGN ${region_min_align})
#zephyr_region(NAME RAM ALIGN ${region_min_align})

# should go to a relocation.cmake - from include/linker/rel-sections.ld - start
zephyr_linker_section(NAME  .rel.plt  HIDDEN)
zephyr_linker_section(NAME  .rela.plt HIDDEN)
zephyr_linker_section(NAME  .rel.dyn)
zephyr_linker_section(NAME  .rela.dyn)
# should go to a relocation.cmake - from include/linker/rel-sections.ld - end

# Discard sections for GNU ld.
zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".plt")
zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".iplt")
zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".got.plt")
zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".igot.plt")
zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".got")
zephyr_linker_section_configure(SECTION /DISCARD/ INPUT ".igot")


if(DEFINED CONFIG_ROM_START_OFFSET
   AND (DEFINED CONFIG_ARM OR DEFINED CONFIG_X86 OR DEFINED CONFIG_SOC_OPENISA_RV32M1_RISCV32)
)
  zephyr_linker_section(NAME .rom_start ADDRESS ${CONFIG_ROM_START_OFFSET} VMA FLASH NOINPUT)
else()
  zephyr_linker_section(NAME .rom_start VMA FLASH NOINPUT)
endif()

zephyr_linker_section(NAME .text         VMA FLASH)

# ToDo: Find out again where section '.extra' originated before re-activating.
#zephyr_linker_section(NAME .extra        VMA RAM LMA FLASH SUBALIGN 8)
#zephyr_linker_section(NAME .bss          VMA RAM LMA RAM TYPE NOLOAD)
#zephyr_linker_section_ifdef(CONFIG_DEBUG_THREAD_INFO NAME zephyr_dbg_info VMA FLASH)

#zephyr_linker_section(NAME .ARM.exidx VMA RAM TYPE NOLOAD)
#zephyr_linker_section_configure_ifdef(GNU SECTION .ARM.exidx INPUT ".ARM.exidx* gnu.linkonce.armexidx.*")

zephyr_linker_section_configure(SECTION .rel.plt  INPUT ".rel.iplt")
zephyr_linker_section_configure(SECTION .rela.plt INPUT ".rela.iplt")

zephyr_linker_section_configure(SECTION text INPUT ".TEXT.*")
zephyr_linker_section_configure(SECTION text INPUT ".gnu.linkonce.t.*")

zephyr_linker_section_configure(SECTION text INPUT ".glue_7t")
zephyr_linker_section_configure(SECTION text INPUT ".glue_7")
zephyr_linker_section_configure(SECTION text INPUT ".vfp11_veneer")
zephyr_linker_section_configure(SECTION text INPUT ".v4_bx")

if(CONFIG_CPLUSPLUS)
  zephyr_linker_section(NAME .ARM.extab VMA FLASH)
  zephyr_linker_section_configure(SECTION .ARM.extab INPUT ".gnu.linkonce.armextab.*")
endif()

zephyr_linker_section(NAME .ARM.exidx VMA FLASH)
# Here the original linker would check for __GCC_LINKER_CMD__, need to check toolchain linker ?
#if(__GCC_LINKER_CMD__)
  zephyr_linker_section_configure(SECTION .ARM.exidx INPUT ".gnu.linkonce.armexidx.*")
#endif()


include(${COMMON_ZEPHYR_LINKER_DIR}/common-rom.cmake)
include(${COMMON_ZEPHYR_LINKER_DIR}/thread-local-storage.cmake)

zephyr_linker_section(NAME .rodata LMA FLASH)
zephyr_linker_section_configure(SECTION .rodata INPUT ".gnu.linkonce.r.*")
if(CONFIG_USERSPACE AND CONFIG_XIP)
  zephyr_linker_section_configure(SECTION .rodata INPUT ".kobject_data.rodata*")
endif()
zephyr_linker_section_configure(SECTION .rodata ALIGN 4)

# ToDo - . = ALIGN(_region_min_align);
# Symbol to add _image_ram_start = .;

# This comes from ramfunc.ls, via snippets-ram-sections.ld
zephyr_linker_section(NAME .ramfunc VMA RAM LMA FLASH SUBALIGN 8)
# MPU_ALIGN(_ramfunc_ram_size);
# } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#_ramfunc_ram_size = _ramfunc_ram_end - _ramfunc_ram_start;
#_ramfunc_rom_start = LOADADDR(.ramfunc);

# ToDo - handle if(CONFIG_USERSPACE)

zephyr_linker_section(NAME .data VMA RAM LMA FLASH)
#zephyr_linker_section_configure(SECTION .data SYMBOLS __data_ram_start)
zephyr_linker_section_configure(SECTION .data INPUT ".kernel.*")
#zephyr_linker_section_configure(SECTION .data SYMBOLS __data_ram_end)

include(${COMMON_ZEPHYR_LINKER_DIR}/common-ram.cmake)
#include(kobject.ld)

if(NOT CONFIG_USERSPACE)
  zephyr_linker_section(NAME .bss VMA RAM LMA FLASH TYPE BSS)
#  zephyr_linker_section(NAME .bss VMA RAM LMA RAM TYPE NOLOAD)
  # For performance, BSS section is assumed to be 4 byte aligned and
  # a multiple of 4 bytes
#        . = ALIGN(4);
#	__kernel_ram_start = .;
  zephyr_linker_section_configure(SECTION .bss INPUT COMMON)
  zephyr_linker_section_configure(SECTION .bss INPUT ".kernel_bss.*")
  # As memory is cleared in words only, it is simpler to ensure the BSS
  # section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
  zephyr_linker_section_configure(SECTION .bss ALIGN 4)
  # GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

  zephyr_linker_section(NAME .noinit VMA RAM LMA FLASH TYPE NOLOAD NOINIT)
  # This section is used for non-initialized objects that
  # will not be cleared during the boot process.
  zephyr_linker_section_configure(SECTION .noinit INPUT ".kernel_noinit.*")
  # GROUP_LINK_IN(RAMABLE_REGION)
endif()

# _image_ram_end = .;
# _end = .; /* end of image */
#
# __kernel_ram_end = RAM_ADDR + RAM_SIZE;
# __kernel_ram_size = __kernel_ram_end - __kernel_ram_start;
#zephyr_linker_symbol(SYMBOL __kernel_ram_end  EXPR "${RAM_ADDR} + ${RAM_SIZE}")
zephyr_linker_symbol(SYMBOL __kernel_ram_end  EXPR "(${RAM_ADDR} + ${RAM_SIZE})")
zephyr_linker_symbol(SYMBOL __kernel_ram_size EXPR "(%__kernel_ram_end% - %__bss_start%)")
zephyr_linker_symbol(SYMBOL _image_ram_start  EXPR "(${RAM_ADDR})" SUBALIGN 32) # ToDo calculate 32 correctly
zephyr_linker_symbol(SYMBOL ARM_LIB_STACKHEAP EXPR "(${RAM_ADDR} + ${RAM_SIZE})" SIZE -0x1000) # ToDo calculate 32 correctly

set(VECTOR_ALIGN 4)
if(CONFIG_CPU_CORTEX_M_HAS_VTOR)
  math(EXPR VECTOR_ALIGN "4 * (16 + ${CONFIG_NUM_IRQS})")
  if(${VECTOR_ALIGN} LESS 128)
    set(VECTOR_ALIGN 128)
  else()
    pow2round(VECTOR_ALIGN)
  endif()
endif()

#zephyr_linker_section_configure(SECTION rom_start INPUT ".exc_vector_table" KEEP FIRST)
# To be moved to: ./arch/arm/core/aarch32/CMakeLists.txt or similar - start
zephyr_linker_section_configure(
  SECTION .rom_start
  INPUT ".exc_vector_table*"
        ".gnu.linkonce.irq_vector_table*"
        ".vectors"
  KEEP FIRST
  SYMBOLS _vector_start _vector_end
  ALIGN ${VECTOR_ALIGN}
  PRIO 0
)


zephyr_linker_section(NAME .ARM.attributes ADDRESS 0 NOINPUT)
zephyr_linker_section_configure(SECTION .ARM.attributes INPUT ".ARM.attributes" KEEP)
zephyr_linker_section_configure(SECTION .ARM.attributes INPUT ".gnu.attributes" KEEP)

# Should this be GNU only ?
# Same symbol is used in code as _IRQ_VECTOR_TABLE_SECTION_NAME, see sections.h
#zephyr_linker_section_configure(SECTION .rom_start INPUT ".gnu.linkonce.irq_vector_table*" KEEP PRIO 1)
#zephyr_linker_section_configure(SECTION .rom_start INPUT ".vectors" KEEP PRIO 2)
#zephyr_linker_section_configure(SECTION .rom_start SYMBOLS _vector_end PRIO 3)
# To be moved to: ./arch/arm/core/aarch32/CMakeLists.txt or similar - end

# armlink specific flags
zephyr_linker_section_configure(SECTION .text ANY FLAGS "+RO" "+XO")
zephyr_linker_section_configure(SECTION .data ANY FLAGS "+RW")
zephyr_linker_section_configure(SECTION .bss ANY FLAGS "+ZI")


include(${COMMON_ZEPHYR_LINKER_DIR}/debug-sections.cmake)
