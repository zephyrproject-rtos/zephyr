# TODO: Get rid of this if statement by introducing
# CONFIG_HAS_INTERRUPT_CONTROLLER

if(CONFIG_LOAPIC OR MVIC OR ARCV2_INTERRUPT_UNIT OR PLIC_FE310 OR EXTI_STM32)
  zephyr_library()

  zephyr_library_sources_ifdef(CONFIG_ARCV2_INTERRUPT_UNIT    arcv2_irq_unit.c)
  zephyr_library_sources_ifdef(CONFIG_IOAPIC                  ioapic_intr.c)
  zephyr_library_sources_ifdef(CONFIG_LOAPIC                  loapic_intr.c system_apic.c)
  zephyr_library_sources_ifdef(CONFIG_LOAPIC_SPURIOUS_VECTOR  loapic_spurious.c)
  zephyr_library_sources_ifdef(CONFIG_MVIC                    mvic.c)
  zephyr_library_sources_ifdef(CONFIG_PIC_DISABLE             i8259.c)
  zephyr_library_sources_ifdef(CONFIG_PLIC_FE310              plic_fe310.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_STM32        exti_stm32.c)
endif()
