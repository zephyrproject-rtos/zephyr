zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_ARCV2_INTERRUPT_UNIT}   arcv2_irq_unit.c
            IFDEF:${CONFIG_IOAPIC}                 ioapic_intr.c
            IFDEF:${CONFIG_LOAPIC}                 loapic_intr.c
                                                   system_apic.c
            IFDEF:${CONFIG_LOAPIC_SPURIOUS_VECTOR} loapic_spurious.S
            IFDEF:${CONFIG_MVIC}                   mvic.c
            IFDEF:${CONFIG_PIC_DISABLE}            i8259.c
            IFDEF:${CONFIG_PLIC}                   plic.c
            IFDEF:${CONFIG_SHARED_IRQ}             shared_irq.c
            IFDEF:${CONFIG_SOC_FAMILY_STM32}       exti_stm32.c
            IFDEF:${CONFIG_CAVS_ICTL}              cavs_ictl.c
            IFDEF:${CONFIG_DW_ICTL}                dw_ictl.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
