zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IF_KCONFIG uart_console.c
            IF_KCONFIG ram_console.c
            IF_KCONFIG rtt_console.c
            IF_KCONFIG ipm_console_receiver.c
            IF_KCONFIG ipm_console_sender.c
            IF_KCONFIG uart_mcumgr.c
            IF_KCONFIG uart_pipe.c
            IF_KCONFIG telnet_console.c
            IF_KCONFIG xtensa_sim_console.c
            IF_KCONFIG native_posix_console.c
            IF_KCONFIG websocket_console.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
