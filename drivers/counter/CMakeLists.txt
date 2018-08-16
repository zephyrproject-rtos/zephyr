zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_AON_COUNTER_QMSI}        counter_qmsi_aon.c
            IFDEF:${CONFIG_AON_TIMER_QMSI}          counter_qmsi_aonpt.c
            IFDEF:${CONFIG_COUNTER_TMR_CMSDK_APB}   counter_tmr_cmsdk_apb.c
            IFDEF:${CONFIG_TIMER_TMR_CMSDK_APB}     timer_tmr_cmsdk_apb.c
            IFDEF:${CONFIG_COUNTER_DTMR_CMSDK_APB}  counter_dtmr_cmsdk_apb.c
            IFDEF:${CONFIG_TIMER_DTMR_CMSDK_APB}    timer_dtmr_cmsdk_apb.c
            IFDEF:${CONFIG_USERSPACE}               counter_handlers.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
