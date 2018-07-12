if(CONFIG_XTENSA_RESET_VECTOR)
  zephyr_list(SOURCES
              OUTPUT PRIVATE_SOURCES
              reset-vector.S
              memerror-vector.S
              memctl_default.S
  )
  zephyr_sources_cc_option( SOURCES ${PRIVATE_SOURCES}
                            OPTIONS -c -mtext-section-literals -mlongcalls
  )

  target_sources(arch_xtensa PRIVATE ${PRIVATE_SOURCES})
endif()
