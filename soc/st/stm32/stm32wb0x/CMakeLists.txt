# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_PM power.c s2ram_marking.S)
zephyr_include_directories(.)

if(CONFIG_CMAKE_LINKER_GENERATOR)
  # The first word of SRAM (at 0x2000'0000) is reserved and followed
  # by the RAM_VR structure. By using 0x2000'0004 as start address
  # for the section, we obtain a similar result as the linker snippet
  # which creates the section at 0x2000'0000 then skips 4 bytes using
  # the GNU ld-specific construct `. += 0x4;` (c.f. ram_sections.ld)
  if(CONFIG_BT)
    # When CONFIG_BT is enabled, the BLUE_RAM section is reserved by `soc.c`.
    # However, this section does NOT start immediately after RAM_VR. The
    # little gap is ignored by GNU ld but NOT by the IAR linker which tries
    # to be clever and places INITIALIZED data there. This results in an
    # image where __data_region_end < __data_region_start which causes a
    # catastrophic failure in arch_data_copy(). Prevent this by setting
    # a MIN_SIZE on the section in such case to ensure there is no gap
    # between the end of RAM_VR and start of BLUE_RAM.
    #
    # The minimum size is (<BLUE_RAM start> - <RAM_VR start>) = 0xBC.
    #                      = 0x2000'00C0      = 0x2000'0004
    set(ram_vr_minsize MIN_SIZE 0xBC)
  endif()
  zephyr_linker_section(NAME stm32wb0_RAM_VR TYPE NOLOAD GROUP RAM_REGION ADDRESS 0x20000004 ${ram_vr_minsize} NOINIT)
  zephyr_linker_section(NAME stm32wb0_BLUE_RAM TYPE NOLOAD GROUP RAM_REGION ADDRESS 0x200000C0 NOINIT)
else()
  zephyr_linker_sources(RAM_SECTIONS ram_sections.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
