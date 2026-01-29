zephyr_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_sources(
  soc.c
  )

if(CONFIG_BT_STM32_IPM)
  if(CONFIG_CMAKE_LINKER_GENERATOR)
    zephyr_linker_section(NAME MAPPING_TABLE TYPE NOLOAD VMA SRAM1 NOINPUT)
    zephyr_linker_section_configure(SECTION MAPPING_TABLE INPUT "MAPPING_TABLE" SYMBOLS _sMAPPING_TABLE _eMAPPING_TABLE)
    zephyr_linker_section(NAME MB_MEM1 TYPE NOLOAD VMA SRAM1 NOINPUT)
    zephyr_linker_section_configure(SECTION MB_MEM1 INPUT "MB_MEM1" SYMBOLS _sMB_MEM1 _eMB_MEM1)
    zephyr_linker_section(NAME MB_MEM2 TYPE NOLOAD VMA SRAM1 NOINPUT)
    zephyr_linker_section_configure(SECTION MB_MEM2 INPUT "MB_MEM2" SYMBOLS _sMB_MEM2 _eMB_MEM2)
  else()
    zephyr_linker_sources(SECTIONS ipm.ld)
  endif()
endif()

zephyr_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_sources_ifdef(CONFIG_POWEROFF poweroff.c)

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
