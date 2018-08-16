zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_MODEM_RECEIVER}  modem_receiver.c
            IFDEF:${CONFIG_MODEM_SHELL}     modem_shell.c
            IFDEF:${CONFIG_MODEM_WNCM14A2A} wncm14a2a.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
