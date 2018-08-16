zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            sys_clock_init.c
            IFDEF:${CONFIG_HPET_TIMER}          hpet.c
            IFDEF:${CONFIG_ARCV2_TIMER}         arcv2_timer0.c
            IFDEF:${CONFIG_ALTERA_AVALON_TIMER} altera_avalon_timer_hal.c
            IFDEF:${CONFIG_XTENSA_TIMER}        xtensa_sys_timer.c
            IF_KCONFIG loapic_timer.c
            IF_KCONFIG nrf_rtc_timer.c
            IF_KCONFIG pulpino_timer.c
            IF_KCONFIG riscv_machine_timer.c
            IF_KCONFIG cortex_m_systick.c
            IF_KCONFIG native_posix_timer.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
