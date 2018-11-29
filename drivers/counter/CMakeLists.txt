zephyr_library()

zephyr_library_sources_ifdef(CONFIG_AON_COUNTER_QMSI		counter_qmsi_aon.c)
zephyr_library_sources_ifdef(CONFIG_AON_TIMER_QMSI		counter_qmsi_aonpt.c)
zephyr_library_sources_ifdef(CONFIG_TIMER_TMR_CMSDK_APB		timer_tmr_cmsdk_apb.c)
zephyr_library_sources_ifdef(CONFIG_TIMER_DTMR_CMSDK_APB	timer_dtmr_cmsdk_apb.c)
zephyr_library_sources_ifdef(CONFIG_COUNTER_IMX_EPIT		counter_imx_epit.c)
zephyr_library_sources_ifdef(CONFIG_COUNTER_MCUX_RTC            counter_mcux_rtc.c)
zephyr_library_sources_ifdef(CONFIG_COUNTER_NRF_TIMER           counter_nrfx_timer.c)
zephyr_library_sources_ifdef(CONFIG_COUNTER_NRF_RTC             counter_nrfx_rtc.c)
zephyr_library_sources_ifdef(CONFIG_RTC_QMSI 			counter_rtc_qmsi.c)
zephyr_library_sources_ifdef(CONFIG_COUNTER_RTC_STM32           counter_ll_stm32_rtc.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE   counter_handlers.c)
