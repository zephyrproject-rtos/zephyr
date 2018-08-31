zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            buf.c
            smp.c
            IFDEF:${CONFIG_MCUMGR_SMP_BT} smp_bt.c
            IFDEF:${CONFIG_MCUMGR_SMP_SHELL} smp_shell.c
            IFDEF:${CONFIG_MCUMGR_SMP_UART} smp_uart.c
)

if (CONFIG_MCUMGR_SMP_SHELL OR CONFIG_MCUMGR_SMP_UART)
  zephyr_list(SOURCES APPEND
              OUTPUT PRIVATE_SOURCES
              serial_util.c
  )
endif ()

add_library(subsys_mgmt STATIC ${PRIVATE_SOURCES})
target_link_libraries(subsys_mgmt PUBLIC MCUMGR)
target_link_libraries(zephyr_app_linking INTERFACE subsys_mgmt)
zephyr_set_property(TARGET subsys_mgmt MEMORY_SPACE kernel)
