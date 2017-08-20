zephyr_library()
zephyr_library_include_directories(PRIVATE ${PROJECT_SOURCE_DIR}/drivers)

if(COMPILER STREQUAL "clang")
else()
  target_cc_option(zephyr PUBLIC -march=lakemont -mtune=lakemont)
endif()

target_cc_option(zephyr PUBLIC -msoft-float -march=pentium)

if(CONFIG_X86_IAMCU)
  # TODO
  # target_cc_option(zephyr PUBLIC -miamcu)
  # set_property(GLOBAL APPEND PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES -D__IAMCU)

  # # OUTPUT_{FORMAT,ARCH} need to be global variables as well, can't set like this
  # set(OUTPUT_FORMAT elf32-iamcu)
  # set(OUTPUT_ARCH iamcu:intel)
else()
  target_cc_option(zephyr PUBLIC -mno-iamcu)
endif()

target_compile_definitions(zephyr PUBLIC QM_LAKEMONT=1)

set(SOC_SERIES quark_se) # PARENT_SCOPE or global property?

# ccflags-y +=-I$(srctree)/arch/x86
# ccflags-y +=-I$(srctree)/include/drivers
# ccflags-y +=-I$(srctree)/drivers
# asflags-y := ${ccflags-y}

zephyr_library_sources_ifdef(CONFIG_SOC_QUARK_SE_C1000
  soc.c
  soc_config.c
  eoi.c
  power.c
  soc_power.S
  )
