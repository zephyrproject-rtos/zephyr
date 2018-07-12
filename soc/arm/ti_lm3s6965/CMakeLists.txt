zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
  soc.c
  soc_config.c
  reboot.S
  sys_arch_reboot.c
  )

target_sources(soc_arm PRIVATE ${PRIVATE_SOURCES})
