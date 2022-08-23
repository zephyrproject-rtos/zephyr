set(COMMON_ZEPHYR_LINKER_DIR ${ZEPHYR_BASE}/cmake/linker_script/common)

set_ifndef(region_min_align CONFIG_CUSTOM_SECTION_MIN_ALIGN_SIZE)

# Set alignment to CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE if not set above
# to make linker section alignment comply with MPU granularity.
set_ifndef(region_min_align CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)

# If building without MPU support, use default 4-byte alignment.. if not set above.
set_ifndef(region_min_align 4)

# Note, the `+ 0` in formulas below avoids errors in cases where a Kconfig
#       variable is undefined and thus expands to nothing.
math(EXPR FLASH_ADDR
     "${CONFIG_FLASH_BASE_ADDRESS} + ${CONFIG_FLASH_LOAD_OFFSET} + 0"
     OUTPUT_FORMAT HEXADECIMAL
)

math(EXPR FLASH_SIZE
     "(${CONFIG_FLASH_SIZE} + 0) * 1024 - (${CONFIG_FLASH_LOAD_OFFSET} + 0)"
     OUTPUT_FORMAT HEXADECIMAL
)

set(RAM_ADDR ${CONFIG_SRAM_BASE_ADDRESS})
math(EXPR RAM_SIZE "(${CONFIG_SRAM_SIZE} + 0) * 1024" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR IDT_ADDR "${RAM_ADDR} + ${RAM_SIZE}" OUTPUT_FORMAT HEXADECIMAL)

# ToDo: decide on the optimal location for this.
# linker/ld/target.cmake based on arch, or directly in arch and scatter_script.cmake can ignore
zephyr_linker(FORMAT "elf32-littlearm")
zephyr_linker(ENTRY ${CONFIG_KERNEL_ENTRY})

zephyr_linker_memory(NAME FLASH    FLAGS rx START ${FLASH_ADDR} SIZE ${FLASH_SIZE})
zephyr_linker_memory(NAME RAM      FLAGS wx START ${RAM_ADDR}   SIZE ${RAM_SIZE})
zephyr_linker_memory(NAME IDT_LIST FLAGS wx START ${IDT_ADDR}   SIZE 2K)

# Only use 'rw' as FLAGS. It's not used anyway.
dt_comp_path(paths COMPATIBLE "zephyr,memory-region")
foreach(path IN LISTS paths)
  zephyr_linker_dts_memory(PATH ${path} FLAGS rw)
endforeach()

if(CONFIG_XIP)
  zephyr_linker_group(NAME ROM_REGION LMA FLASH)
  set(rom_start ${FLASH_ADDR})
  set(XIP_ALIGN_WITH_INPUT ALIGN_WITH_INPUT)
else()
  zephyr_linker_group(NAME ROM_REGION LMA RAM)
  set(rom_start ${RAM_ADDR})
endif()

zephyr_linker_group(NAME RAM_REGION VMA RAM LMA ROM_REGION)
zephyr_linker_group(NAME TEXT_REGION GROUP ROM_REGION SYMBOL SECTION)
zephyr_linker_group(NAME RODATA_REGION GROUP ROM_REGION)
zephyr_linker_group(NAME DATA_REGION GROUP RAM_REGION SYMBOL SECTION)

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

zephyr_linker_section(NAME .rom_start ADDRESS ${rom_start} GROUP ROM_REGION NOINPUT)

zephyr_linker_section(NAME .text         GROUP TEXT_REGION)

zephyr_linker_section_configure(SECTION .rel.plt  INPUT ".rel.iplt")
zephyr_linker_section_configure(SECTION .rela.plt INPUT ".rela.iplt")

zephyr_linker_section_configure(SECTION .text INPUT ".TEXT.*")
zephyr_linker_section_configure(SECTION .text INPUT ".gnu.linkonce.t.*")

zephyr_linker_section_configure(SECTION .text INPUT ".glue_7t")
zephyr_linker_section_configure(SECTION .text INPUT ".glue_7")
zephyr_linker_section_configure(SECTION .text INPUT ".vfp11_veneer")
zephyr_linker_section_configure(SECTION .text INPUT ".v4_bx")
zephyr_linker_section_configure(SECTION .text INPUT ".shared_irq")
zephyr_linker_section_configure(SECTION .text INPUT ".shared_irq_arg")

if(CONFIG_CPP)
  zephyr_linker_section(NAME .ARM.extab GROUP ROM_REGION)
  zephyr_linker_section_configure(SECTION .ARM.extab INPUT ".gnu.linkonce.armextab.*")
endif()

zephyr_linker_section(NAME .ARM.exidx GROUP ROM_REGION)
# Here the original linker would check for __GCC_LINKER_CMD__, need to check toolchain linker ?
#if(__GCC_LINKER_CMD__)
  zephyr_linker_section_configure(SECTION .ARM.exidx INPUT ".gnu.linkonce.armexidx.*")
#endif()


include(${COMMON_ZEPHYR_LINKER_DIR}/common-rom.cmake)
include(${COMMON_ZEPHYR_LINKER_DIR}/thread-local-storage.cmake)

zephyr_linker_section(NAME .rodata GROUP RODATA_REGION)
zephyr_linker_section_configure(SECTION .rodata INPUT ".gnu.linkonce.r.*")
if(CONFIG_USERSPACE AND CONFIG_XIP)
  zephyr_linker_section_configure(SECTION .rodata INPUT ".kobject_data.rodata*")
endif()
zephyr_linker_section_configure(SECTION .rodata ALIGN 4)

# ToDo - . = ALIGN(_region_min_align);
# Symbol to add _image_ram_start = .;

# This comes from ramfunc.ls, via snippets-ram-sections.ld
zephyr_linker_section(NAME .ramfunc GROUP RAM_REGION SUBALIGN 8)
# Todo: handle MPU_ALIGN(_ramfunc_size);

# ToDo - handle if(CONFIG_USERSPACE)

zephyr_linker_section(NAME .data GROUP DATA_REGION)
zephyr_linker_section_configure(SECTION .data INPUT ".kernel.*")

include(${COMMON_ZEPHYR_LINKER_DIR}/common-ram.cmake)
#include(kobject.ld)

if(NOT CONFIG_USERSPACE)
  zephyr_linker_section(NAME .bss VMA RAM LMA FLASH TYPE BSS)
  zephyr_linker_section_configure(SECTION .bss INPUT COMMON)
  zephyr_linker_section_configure(SECTION .bss INPUT ".kernel_bss.*")
  # As memory is cleared in words only, it is simpler to ensure the BSS
  # section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
  zephyr_linker_section_configure(SECTION .bss ALIGN 4)

  zephyr_linker_section(NAME .noinit GROUP RAM_REGION TYPE NOLOAD NOINIT)
  # This section is used for non-initialized objects that
  # will not be cleared during the boot process.
  zephyr_linker_section_configure(SECTION .noinit INPUT ".kernel_noinit.*")
endif()

zephyr_linker_symbol(SYMBOL __kernel_ram_start EXPR "(@__bss_start@)")
zephyr_linker_symbol(SYMBOL __kernel_ram_end  EXPR "(${RAM_ADDR} + ${RAM_SIZE})")
zephyr_linker_symbol(SYMBOL __kernel_ram_size EXPR "(@__kernel_ram_end@ - @__bss_start@)")
zephyr_linker_symbol(SYMBOL _image_ram_start  EXPR "(${RAM_ADDR})" SUBALIGN 32) # ToDo calculate 32 correctly
zephyr_linker_symbol(SYMBOL ARM_LIB_STACKHEAP EXPR "(${RAM_ADDR} + ${RAM_SIZE})" SIZE -0x1000)

set(VECTOR_ALIGN 4)
if(CONFIG_CPU_CORTEX_M_HAS_VTOR)
  math(EXPR VECTOR_ALIGN "4 * (16 + ${CONFIG_NUM_IRQS})")
  if(${VECTOR_ALIGN} LESS 128)
    set(VECTOR_ALIGN 128)
  else()
    pow2round(VECTOR_ALIGN)
  endif()
endif()

zephyr_linker_section_configure(
  SECTION .rom_start
  INPUT ".exc_vector_table*"
        ".gnu.linkonce.irq_vector_table*"
        ".vectors"
  OFFSET ${CONFIG_ROM_START_OFFSET}
  KEEP FIRST
  SYMBOLS _vector_start _vector_end
  ALIGN ${VECTOR_ALIGN}
  PRIO 50
)

dt_chosen(chosen_itcm PROPERTY "zephyr,itcm")
if(DEFINED chosen_itcm)
  dt_node_has_status(status_result PATH ${chosen_itcm} STATUS okay)
  if(${status_result})
    zephyr_linker_group(NAME ITCM_REGION VMA ITCM LMA ROM_REGION)

    zephyr_linker_section(NAME .itcm GROUP ITCM_REGION SUBALIGN 4)
  endif()
endif()

dt_chosen(chosen_dtcm PROPERTY "zephyr,dtcm")
if(DEFINED chosen_dtcm)
  dt_node_has_status(status_result PATH ${chosen_dtcm} STATUS okay)
  if(${status_result})
    zephyr_linker_group(NAME DTCM_REGION VMA DTCM LMA ROM_REGION)

    zephyr_linker_section(NAME .dtcm_bss GROUP DTCM_REGION SUBALIGN 4 TYPE BSS)
    zephyr_linker_section(NAME .dtcm_noinit GROUP DTCM_REGION SUBALIGN 4 TYPE NOLOAD NOINIT)
    zephyr_linker_section(NAME .dtcm_data GROUP DTCM_REGION SUBALIGN 4)
  endif()
endif()

zephyr_linker_section(NAME .ARM.attributes ADDRESS 0 NOINPUT)
zephyr_linker_section_configure(SECTION .ARM.attributes INPUT ".ARM.attributes" KEEP)
zephyr_linker_section_configure(SECTION .ARM.attributes INPUT ".gnu.attributes" KEEP)

# armlink specific flags
zephyr_linker_section_configure(SECTION .text ANY FLAGS "+RO" "+XO")
zephyr_linker_section_configure(SECTION .data ANY FLAGS "+RW")
zephyr_linker_section_configure(SECTION .bss ANY FLAGS "+ZI")

include(${COMMON_ZEPHYR_LINKER_DIR}/debug-sections.cmake)

dt_comp_path(paths COMPATIBLE "zephyr,memory-region")
foreach(path IN LISTS paths)
  zephyr_linker_dts_section(PATH ${path})
endforeach()
