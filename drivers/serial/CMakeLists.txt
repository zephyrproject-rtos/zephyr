zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_UART_ALTERA_JTAG}     uart_altera_jtag_hal.c
            IFDEF:${CONFIG_NRF_UART_PERIPHERAL}  uart_nrfx_uart.c
            IFDEF:${CONFIG_NRF_UARTE_PERIPHERAL} uart_nrfx_uarte.c
            IFDEF:${CONFIG_USERSPACE}            uart_handlers.c
            IFDEF:${CONFIG_UART_NATIVE_POSIX}    uart_native_posix.c
            IF_KCONFIG uart_imx.c
            IF_KCONFIG uart_cc32xx.c
            IF_KCONFIG uart_cmsdk_apb.c
            IF_KCONFIG uart_esp32.c
            IF_KCONFIG uart_sifive.c
            IF_KCONFIG uart_gecko.c
            IF_KCONFIG leuart_gecko.c
            IF_KCONFIG uart_mcux.c
            IF_KCONFIG uart_mcux_lpuart.c
            IF_KCONFIG uart_mcux_lpsci.c
            IF_KCONFIG uart_miv.c
            IF_KCONFIG uart_msp432p4xx.c
            IF_KCONFIG uart_ns16550.c
            IF_KCONFIG uart_nsim.c
            IF_KCONFIG uart_qmsi.c
            IF_KCONFIG uart_sam.c
            IF_KCONFIG usart_sam.c
            IF_KCONFIG uart_stellaris.c
            IF_KCONFIG uart_stm32.c
            IF_KCONFIG uart_sam0.c
            IF_KCONFIG usart_mcux_lpc.c
            IF_KCONFIG uart_psoc6.c
)

if(CONFIG_UART_NATIVE_POSIX)
  SET_SOURCE_FILES_PROPERTIES(${PRIVATE_SOURCES} PROPERTIES COMPILE_DEFINITIONS NO_POSIX_CHEATS)
endif()

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
