zephyr_library()

zephyr_library_sources(sys_clock_init.c)

target_sources_ifdef(CONFIG_HPET_TIMER  drivers__timer PRIVATE hpet.c)
target_sources_ifdef(CONFIG_ARCV2_TIMER drivers__timer PRIVATE arcv2_timer0.c)

target_sources_if_kconfig(              drivers__timer PRIVATE loapic_timer.c)
target_sources_if_kconfig(              drivers__timer PRIVATE altera_avalon_timer.c)
target_sources_if_kconfig(              drivers__timer PRIVATE nrf_rtc_timer.c)
target_sources_if_kconfig(              drivers__timer PRIVATE pulpino_timer.c)
target_sources_if_kconfig(              drivers__timer PRIVATE riscv_machine_timer.c)
target_sources_if_kconfig(              drivers__timer PRIVATE cortex_m_systick.c)

target_link_libraries(drivers__timer drivers)
